/*
 * 2048 Game Desk Accessory
 *
 * Classic 2048 sliding tile puzzle game.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "widgets_common.h"

#define GRID_SIZE 4
#define TILE_WIDTH 7
#define TILE_HEIGHT 3

typedef struct {
    ITuiDeskApp Interface;
    WIDGET_STATE State;
    CHAR8 Title[64];

    /* Game state */
    INT32 Grid[GRID_SIZE][GRID_SIZE];
    INT32 Score;
    INT32 BestScore;
    BOOLEAN GameOver;
    BOOLEAN Won;

    /* Display dimensions */
    UINT32 Width;
    UINT32 Height;

    /* Animation */
    INT32 LastMoveRow;
    INT32 LastMoveCol;

} Game2048Impl;

/* Get tile color based on value */
static TUI_COLOR GetTileColor(INT32 value)
{
    switch (value) {
        case 2:    return TuiColorWhite;
        case 4:    return TuiColorYellow;
        case 8:    return TuiColorCyan;
        case 16:   return TuiColorGreen;
        case 32:   return TuiColorMagenta;
        case 64:   return TuiColorRed;
        case 128:  return TuiColorBrightYellow;
        case 256:  return TuiColorBrightCyan;
        case 512:  return TuiColorBrightGreen;
        case 1024: return TuiColorBrightMagenta;
        case 2048: return TuiColorBrightRed;
        default:   return TuiColorBrightWhite;
    }
}

/* Add random tile (2 or 4) to empty cell */
static VOID AddRandomTile(Game2048Impl *impl)
{
    /* Count empty cells */
    INT32 emptyCells[GRID_SIZE * GRID_SIZE][2];
    INT32 emptyCount = 0;

    for (INT32 i = 0; i < GRID_SIZE; i++) {
        for (INT32 j = 0; j < GRID_SIZE; j++) {
            if (impl->Grid[i][j] == 0) {
                emptyCells[emptyCount][0] = i;
                emptyCells[emptyCount][1] = j;
                emptyCount++;
            }
        }
    }

    if (emptyCount == 0) return;

    /* Pick random empty cell */
    INT32 index = rand() % emptyCount;
    INT32 row = emptyCells[index][0];
    INT32 col = emptyCells[index][1];

    /* 90% chance of 2, 10% chance of 4 */
    impl->Grid[row][col] = (rand() % 10 == 0) ? 4 : 2;

    impl->LastMoveRow = row;
    impl->LastMoveCol = col;
}

/* Check if any moves are possible */
static BOOLEAN CanMove(Game2048Impl *impl)
{
    /* Check for empty cells */
    for (INT32 i = 0; i < GRID_SIZE; i++) {
        for (INT32 j = 0; j < GRID_SIZE; j++) {
            if (impl->Grid[i][j] == 0) {
                return TRUE;
            }
        }
    }

    /* Check for adjacent equal tiles */
    for (INT32 i = 0; i < GRID_SIZE; i++) {
        for (INT32 j = 0; j < GRID_SIZE; j++) {
            INT32 current = impl->Grid[i][j];

            /* Check right */
            if (j < GRID_SIZE - 1 && impl->Grid[i][j + 1] == current) {
                return TRUE;
            }

            /* Check down */
            if (i < GRID_SIZE - 1 && impl->Grid[i + 1][j] == current) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

/* Move tiles in specified direction */
static BOOLEAN MoveTiles(Game2048Impl *impl, INT32 dx, INT32 dy)
{
    BOOLEAN moved = FALSE;
    INT32 startRow = (dy == 1) ? GRID_SIZE - 1 : 0;
    INT32 startCol = (dx == 1) ? GRID_SIZE - 1 : 0;
    INT32 rowStep = (dy == 1) ? -1 : 1;
    INT32 colStep = (dx == 1) ? -1 : 1;

    /* Create merged flags to prevent double merging */
    BOOLEAN merged[GRID_SIZE][GRID_SIZE] = {{FALSE}};

    /* Process tiles in direction of movement */
    for (INT32 i = startRow; i >= 0 && i < GRID_SIZE; i += rowStep) {
        for (INT32 j = startCol; j >= 0 && j < GRID_SIZE; j += colStep) {
            if (impl->Grid[i][j] == 0) continue;

            INT32 currentRow = i;
            INT32 currentCol = j;
            INT32 value = impl->Grid[i][j];

            /* Move tile as far as possible */
            while (TRUE) {
                INT32 newRow = currentRow + dy;
                INT32 newCol = currentCol + dx;

                /* Check bounds */
                if (newRow < 0 || newRow >= GRID_SIZE ||
                    newCol < 0 || newCol >= GRID_SIZE) {
                    break;
                }

                /* Check if target is empty */
                if (impl->Grid[newRow][newCol] == 0) {
                    impl->Grid[newRow][newCol] = value;
                    impl->Grid[currentRow][currentCol] = 0;
                    currentRow = newRow;
                    currentCol = newCol;
                    moved = TRUE;
                    continue;
                }

                /* Check if can merge */
                if (impl->Grid[newRow][newCol] == value &&
                    !merged[newRow][newCol]) {
                    impl->Grid[newRow][newCol] *= 2;
                    impl->Grid[currentRow][currentCol] = 0;
                    merged[newRow][newCol] = TRUE;
                    impl->Score += impl->Grid[newRow][newCol];
                    moved = TRUE;

                    /* Check for win */
                    if (impl->Grid[newRow][newCol] == 2048 && !impl->Won) {
                        impl->Won = TRUE;
                    }
                }

                break;
            }
        }
    }

    return moved;
}

/* Initialize new game */
static VOID InitGame(Game2048Impl *impl)
{
    /* Clear grid */
    memset(impl->Grid, 0, sizeof(impl->Grid));

    impl->Score = 0;
    impl->GameOver = FALSE;
    impl->Won = FALSE;
    impl->LastMoveRow = -1;
    impl->LastMoveCol = -1;

    /* Add two starting tiles */
    AddRandomTile(impl);
    AddRandomTile(impl);
}

/* IUnknown methods */
static HRESULT ANXAPI Game2048_QueryInterface(
    ITuiDeskApp *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI Game2048_AddRef(ITuiDeskApp *This)
{
    Game2048Impl *impl = (Game2048Impl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Game2048_Release(ITuiDeskApp *This)
{
    Game2048Impl *impl = (Game2048Impl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* Render game */
static HRESULT ANXAPI Game2048_Render(
    ITuiDeskApp *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    Game2048Impl *impl = (Game2048Impl *)This;
    CHAR8 buffer[128];

    if (!impl->State.Visible) return S_OK;

    /* Draw title and score */
    snprintf(buffer, sizeof(buffer), " 2048 Game ");
    Screen->Vtbl->WriteText(Screen, X + 2, Y, buffer,
                           TuiColorBlack, TuiColorYellow);

    snprintf(buffer, sizeof(buffer), "Score: %d", impl->Score);
    Screen->Vtbl->WriteText(Screen, X + 16, Y, buffer,
                           TuiColorWhite, TuiColorBlack);

    snprintf(buffer, sizeof(buffer), "Best: %d", impl->BestScore);
    Screen->Vtbl->WriteText(Screen, X + 30, Y, buffer,
                           TuiColorYellow, TuiColorBlack);

    Y += 2;

    /* Draw grid */
    for (INT32 i = 0; i < GRID_SIZE; i++) {
        for (INT32 j = 0; j < GRID_SIZE; j++) {
            INT32 tileX = X + j * (TILE_WIDTH + 1);
            INT32 tileY = Y + i * (TILE_HEIGHT + 1);

            INT32 value = impl->Grid[i][j];

            /* Draw tile border */
            for (INT32 tx = 0; tx < TILE_WIDTH; tx++) {
                Screen->Vtbl->WriteChar(Screen, tileX + tx, tileY,
                                       gBoxChars.Horizontal,
                                       TuiColorBrightBlack, TuiColorBlack);
                Screen->Vtbl->WriteChar(Screen, tileX + tx, tileY + TILE_HEIGHT - 1,
                                       gBoxChars.Horizontal,
                                       TuiColorBrightBlack, TuiColorBlack);
            }

            for (INT32 ty = 0; ty < TILE_HEIGHT; ty++) {
                Screen->Vtbl->WriteChar(Screen, tileX, tileY + ty,
                                       gBoxChars.Vertical,
                                       TuiColorBrightBlack, TuiColorBlack);
                Screen->Vtbl->WriteChar(Screen, tileX + TILE_WIDTH - 1, tileY + ty,
                                       gBoxChars.Vertical,
                                       TuiColorBrightBlack, TuiColorBlack);
            }

            /* Draw tile value */
            if (value > 0) {
                snprintf(buffer, sizeof(buffer), "%d", value);
                INT32 textX = tileX + (TILE_WIDTH - strlen(buffer)) / 2;
                INT32 textY = tileY + TILE_HEIGHT / 2;

                TUI_COLOR fg = TuiColorBlack;
                TUI_COLOR bg = GetTileColor(value);

                /* Fill tile background */
                for (INT32 ty = 1; ty < TILE_HEIGHT - 1; ty++) {
                    for (INT32 tx = 1; tx < TILE_WIDTH - 1; tx++) {
                        Screen->Vtbl->WriteChar(Screen, tileX + tx, tileY + ty,
                                               ' ', fg, bg);
                    }
                }

                /* Draw value */
                Screen->Vtbl->WriteText(Screen, textX, textY, buffer, fg, bg);

                /* Highlight newly added tile */
                if (i == impl->LastMoveRow && j == impl->LastMoveCol) {
                    Screen->Vtbl->WriteChar(Screen, tileX + 1, tileY + 1,
                                           gBoxChars.Diamond,
                                           TuiColorBrightYellow, bg);
                }
            }
        }
    }

    Y += GRID_SIZE * (TILE_HEIGHT + 1) + 1;

    /* Draw status */
    if (impl->GameOver) {
        Screen->Vtbl->WriteText(Screen, X + 8, Y,
                               "GAME OVER! Press N for new game",
                               TuiColorRed, TuiColorBlack);
    } else if (impl->Won) {
        Screen->Vtbl->WriteText(Screen, X + 8, Y,
                               "YOU WIN! Press C to continue or N for new game",
                               TuiColorGreen, TuiColorBlack);
    } else {
        Screen->Vtbl->WriteText(Screen, X + 2, Y,
                               "Use arrow keys to move tiles. N:New game  Q:Quit",
                               TuiColorBrightBlack, TuiColorBlack);
    }

    return S_OK;
}

/* Handle input */
static HRESULT ANXAPI Game2048_HandleKey(
    ITuiDeskApp *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    Game2048Impl *impl = (Game2048Impl *)This;

    *Handled = FALSE;

    if (!impl->State.Enabled) return S_OK;

    /* Reset last move highlight */
    impl->LastMoveRow = -1;
    impl->LastMoveCol = -1;

    /* Handle game over */
    if (impl->GameOver) {
        if (Key == 'n' || Key == 'N') {
            InitGame(impl);
            *Handled = TRUE;
        }
        return S_OK;
    }

    /* Handle win state */
    if (impl->Won) {
        if (Key == 'n' || Key == 'N') {
            InitGame(impl);
            *Handled = TRUE;
            return S_OK;
        } else if (Key == 'c' || Key == 'C') {
            impl->Won = FALSE;  /* Continue playing */
            *Handled = TRUE;
            return S_OK;
        }
    }

    BOOLEAN moved = FALSE;

    switch (Key) {
        case TuiKeyUp:
            moved = MoveTiles(impl, 0, -1);
            *Handled = TRUE;
            break;

        case TuiKeyDown:
            moved = MoveTiles(impl, 0, 1);
            *Handled = TRUE;
            break;

        case TuiKeyLeft:
            moved = MoveTiles(impl, -1, 0);
            *Handled = TRUE;
            break;

        case TuiKeyRight:
            moved = MoveTiles(impl, 1, 0);
            *Handled = TRUE;
            break;

        case 'n':
        case 'N':
            InitGame(impl);
            *Handled = TRUE;
            return S_OK;
    }

    /* If tiles moved, add new tile and check game over */
    if (moved) {
        AddRandomTile(impl);

        /* Update best score */
        if (impl->Score > impl->BestScore) {
            impl->BestScore = impl->Score;
        }

        /* Check if game is over */
        if (!CanMove(impl)) {
            impl->GameOver = TRUE;
        }
    }

    return S_OK;
}

/* Get title */
static HRESULT ANXAPI Game2048_GetTitle(
    ITuiDeskApp *This,
    CONST CHAR8 **OutTitle
)
{
    Game2048Impl *impl = (Game2048Impl *)This;
    *OutTitle = impl->Title;
    return S_OK;
}

/* VTable */
static CONST ITuiDeskApp_Vtbl Game2048Vtbl = {
    Game2048_QueryInterface,
    Game2048_AddRef,
    Game2048_Release,
    Game2048_Render,
    Game2048_HandleKey,
    Game2048_GetTitle
};

/* Factory function */
HRESULT AnxTuiCreateGame2048(ITuiDeskApp **OutGame)
{
    Game2048Impl *impl;

    if (!OutGame) return E_POINTER;

    impl = (Game2048Impl *)calloc(1, sizeof(Game2048Impl));
    if (!impl) {
        *OutGame = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &Game2048Vtbl;
    InitWidgetState(&impl->State);

    strcpy(impl->Title, "2048");

    impl->Width = 40;
    impl->Height = 25;
    impl->BestScore = 0;

    /* Initialize random seed */
    srand((unsigned int)time(NULL));

    /* Start new game */
    InitGame(impl);

    *OutGame = &impl->Interface;
    return S_OK;
}
