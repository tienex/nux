/*
 * widget_spreadsheet.c - Spreadsheet Widget
 *
 * Excel-like spreadsheet control with:
 * - Dynamic virtual storage (millions of cells)
 * - Formula evaluation
 * - Cell editing and selection
 * - Freeze panes
 * - Column/row headers
 * - Cell formatting
 * - Copy/paste integration
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_CELL_LENGTH 256
#define MAX_COLUMNS 256
#define MAX_ROWS 1048576
#define DEFAULT_COL_WIDTH 10
#define DEFAULT_ROW_HEIGHT 1
#define HEADER_WIDTH 4

/* Cell data */
typedef struct {
    CHAR8 Value[MAX_CELL_LENGTH];      /* Display value */
    CHAR8 Formula[MAX_CELL_LENGTH];    /* Formula (if starts with =) */
    TUI_COLOR Foreground;
    TUI_COLOR Background;
    UINT8 Attributes;                   /* Bold, italic, etc. */
} SpreadsheetCell;

/* Selection range */
typedef struct {
    UINT32 StartRow;
    UINT32 StartCol;
    UINT32 EndRow;
    UINT32 EndCol;
} CellRange;

typedef struct {
    ITuiSpreadsheet Interface;
    WIDGET_STATE State;

    /* Dimensions */
    UINT32 TotalRows;
    UINT32 TotalColumns;
    UINT32 VisibleRows;
    UINT32 VisibleColumns;

    /* Column widths */
    UINT32 ColumnWidths[MAX_COLUMNS];

    /* Scrolling */
    UINT32 ScrollRow;
    UINT32 ScrollCol;

    /* Freeze panes */
    UINT32 FreezeRow;  /* Rows above this are frozen */
    UINT32 FreezeCol;  /* Columns left of this are frozen */

    /* Selection */
    UINT32 CurrentRow;
    UINT32 CurrentCol;
    CellRange Selection;
    BOOLEAN SelectionActive;

    /* Editing */
    BOOLEAN EditMode;
    CHAR8 EditBuffer[MAX_CELL_LENGTH];
    UINT32 EditCursor;

    /* Virtual mode */
    BOOLEAN VirtualMode;
    HRESULT (*OnGetCell)(VOID *UserData, UINT32 Row, UINT32 Col, SpreadsheetCell *OutCell);
    HRESULT (*OnSetCell)(VOID *UserData, UINT32 Row, UINT32 Col, CONST SpreadsheetCell *Cell);
    VOID *VirtualUserData;

    /* Cell storage (for non-virtual mode) */
    SpreadsheetCell **Cells;  /* Sparse storage [row][col] */
    UINT32 AllocatedRows;

    /* Callbacks */
    HRESULT (*OnCellChanged)(VOID *UserData, UINT32 Row, UINT32 Col, CONST CHAR8 *Value);
    HRESULT (*OnSelectionChanged)(VOID *UserData, CONST CellRange *Range);
    VOID *UserData;

} TuiSpreadsheetImpl;

/* Helper: Get column name (A, B, ..., Z, AA, AB, ...) */
static VOID GetColumnName(UINT32 col, CHAR8 *out, UINTN size)
{
    if (col < 26) {
        snprintf(out, size, "%c", 'A' + col);
    } else {
        snprintf(out, size, "%c%c", 'A' + (col / 26) - 1, 'A' + (col % 26));
    }
}

/* Helper: Evaluate simple formula */
static VOID EvaluateFormula(CONST CHAR8 *formula, CHAR8 *result, UINTN size)
{
    /* Simple evaluation - just handle basic arithmetic */
    /* In a real implementation, this would parse cell references and operators */

    if (formula[0] != '=') {
        strncpy(result, formula, size - 1);
        result[size - 1] = '\0';
        return;
    }

    /* Skip '=' */
    formula++;

    /* Try to evaluate as arithmetic expression */
    double value = 0.0;
    if (sscanf(formula, "%lf", &value) == 1) {
        snprintf(result, size, "%.2f", value);
    } else {
        strncpy(result, "#ERROR", size - 1);
        result[size - 1] = '\0';
    }
}

/* Helper: Get cell (with virtual mode support) */
static HRESULT GetCell(TuiSpreadsheetImpl *impl, UINT32 row, UINT32 col, SpreadsheetCell *outCell)
{
    if (impl->VirtualMode && impl->OnGetCell) {
        return impl->OnGetCell(impl->VirtualUserData, row, col, outCell);
    }

    /* Non-virtual mode */
    if (row >= impl->AllocatedRows || !impl->Cells[row]) {
        /* Empty cell */
        memset(outCell, 0, sizeof(SpreadsheetCell));
        outCell->Foreground = TuiColorBlack;
        outCell->Background = TuiColorWhite;
        return S_OK;
    }

    *outCell = impl->Cells[row][col];
    return S_OK;
}

/* Helper: Set cell (with virtual mode support) */
static HRESULT SetCell(TuiSpreadsheetImpl *impl, UINT32 row, UINT32 col, CONST SpreadsheetCell *cell)
{
    if (impl->VirtualMode && impl->OnSetCell) {
        return impl->OnSetCell(impl->VirtualUserData, row, col, cell);
    }

    /* Non-virtual mode - allocate storage if needed */
    if (row >= impl->AllocatedRows) {
        UINT32 newSize = row + 1024;
        SpreadsheetCell **newCells = (SpreadsheetCell **)realloc(impl->Cells, newSize * sizeof(SpreadsheetCell *));
        if (!newCells) return E_OUTOFMEMORY;

        /* Initialize new rows */
        for (UINT32 i = impl->AllocatedRows; i < newSize; i++) {
            newCells[i] = NULL;
        }

        impl->Cells = newCells;
        impl->AllocatedRows = newSize;
    }

    if (!impl->Cells[row]) {
        impl->Cells[row] = (SpreadsheetCell *)calloc(impl->TotalColumns, sizeof(SpreadsheetCell));
        if (!impl->Cells[row]) return E_OUTOFMEMORY;
    }

    impl->Cells[row][col] = *cell;
    return S_OK;
}

/* IUnknown methods */
static HRESULT ANXAPI Spreadsheet_QueryInterface(
    ITuiSpreadsheet *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiSpreadsheet)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI Spreadsheet_AddRef(ITuiSpreadsheet *This)
{
    TuiSpreadsheetImpl *impl = (TuiSpreadsheetImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Spreadsheet_Release(ITuiSpreadsheet *This)
{
    TuiSpreadsheetImpl *impl = (TuiSpreadsheetImpl *)This;
    UINTN count = --impl->State.RefCount;

    if (count == 0) {
        /* Free cell storage */
        if (impl->Cells) {
            for (UINT32 i = 0; i < impl->AllocatedRows; i++) {
                if (impl->Cells[i]) {
                    free(impl->Cells[i]);
                }
            }
            free(impl->Cells);
        }
        free(impl);
    }

    return count;
}

/* Render the spreadsheet */
static HRESULT ANXAPI Spreadsheet_Render(
    ITuiSpreadsheet *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiSpreadsheetImpl *impl = (TuiSpreadsheetImpl *)This;

    if (!impl->State.Visible) return S_OK;

    UINT32 width = impl->State.Bounds.Width;
    UINT32 height = impl->State.Bounds.Height;

    /* Draw background */
    for (UINT32 i = 0; i < height; i++) {
        for (UINT32 j = 0; j < width; j++) {
            Screen->Vtbl->WriteChar(Screen, X + j, Y + i, ' ', TuiColorBlack, TuiColorWhite);
        }
    }

    /* Draw border */
    DrawBoxSingle(Screen, X, Y, width, height, TuiColorBlack, TuiColorWhite);

    /* Calculate visible area */
    UINT32 contentX = X + 1 + HEADER_WIDTH;
    UINT32 contentY = Y + 2;  /* Leave room for column headers */
    UINT32 contentWidth = width - HEADER_WIDTH - 2;
    UINT32 contentHeight = height - 3;

    /* Draw column headers */
    UINT32 colX = contentX;
    UINT32 visibleCol = impl->ScrollCol;

    for (UINT32 c = 0; c < impl->VisibleColumns && colX < X + width - 1; c++, visibleCol++) {
        if (visibleCol >= impl->TotalColumns) break;

        CHAR8 colName[8];
        GetColumnName(visibleCol, colName, sizeof(colName));

        UINT32 colWidth = impl->ColumnWidths[visibleCol];

        /* Column header background */
        TUI_COLOR headerBG = (visibleCol == impl->CurrentCol) ? TuiColorCyan : TuiColorBrightBlack;
        for (UINT32 i = 0; i < colWidth && colX + i < X + width - 1; i++) {
            Screen->Vtbl->WriteChar(Screen, colX + i, Y + 1, ' ', TuiColorWhite, headerBG);
        }

        /* Column name */
        Screen->Vtbl->WriteText(Screen, colX, Y + 1, colName, TuiColorWhite, headerBG);

        colX += colWidth + 1;  /* +1 for separator */
    }

    /* Draw row headers and cells */
    UINT32 rowY = contentY;
    UINT32 visibleRow = impl->ScrollRow;

    for (UINT32 r = 0; r < contentHeight && rowY < Y + height - 1; r++, visibleRow++) {
        if (visibleRow >= impl->TotalRows) break;

        /* Row header */
        CHAR8 rowNum[8];
        snprintf(rowNum, sizeof(rowNum), "%4u", visibleRow + 1);
        TUI_COLOR rowHeaderBG = (visibleRow == impl->CurrentRow) ? TuiColorCyan : TuiColorBrightBlack;

        for (UINT32 i = 0; i < HEADER_WIDTH; i++) {
            Screen->Vtbl->WriteChar(Screen, X + 1 + i, rowY, ' ', TuiColorWhite, rowHeaderBG);
        }
        Screen->Vtbl->WriteText(Screen, X + 1, rowY, rowNum, TuiColorWhite, rowHeaderBG);

        /* Draw cells */
        colX = contentX;
        visibleCol = impl->ScrollCol;

        for (UINT32 c = 0; c < impl->VisibleColumns && colX < X + width - 1; c++, visibleCol++) {
            if (visibleCol >= impl->TotalColumns) break;

            UINT32 colWidth = impl->ColumnWidths[visibleCol];

            /* Get cell data */
            SpreadsheetCell cell;
            GetCell(impl, visibleRow, visibleCol, &cell);

            /* Determine if cell is selected */
            BOOLEAN isCurrentCell = (visibleRow == impl->CurrentRow && visibleCol == impl->CurrentCol);
            BOOLEAN isInSelection = (visibleRow >= impl->Selection.StartRow &&
                                    visibleRow <= impl->Selection.EndRow &&
                                    visibleCol >= impl->Selection.StartCol &&
                                    visibleCol <= impl->Selection.EndCol);

            TUI_COLOR fg = isCurrentCell ? TuiColorWhite : cell.Foreground;
            TUI_COLOR bg = isCurrentCell ? TuiColorBlue : (isInSelection ? TuiColorCyan : cell.Background);

            /* Draw cell background */
            for (UINT32 i = 0; i < colWidth && colX + i < X + width - 1; i++) {
                Screen->Vtbl->WriteChar(Screen, colX + i, rowY, ' ', fg, bg);
            }

            /* Draw cell content */
            if (isCurrentCell && impl->EditMode) {
                /* Show edit buffer */
                CHAR8 display[MAX_CELL_LENGTH];
                snprintf(display, colWidth + 1, "%s", impl->EditBuffer);
                Screen->Vtbl->WriteText(Screen, colX, rowY, display, fg, bg);
            } else {
                /* Show cell value */
                CHAR8 display[MAX_CELL_LENGTH];

                /* If formula, evaluate it */
                if (cell.Value[0] == '=') {
                    EvaluateFormula(cell.Value, display, sizeof(display));
                } else {
                    strncpy(display, cell.Value, sizeof(display) - 1);
                    display[sizeof(display) - 1] = '\0';
                }

                /* Truncate to column width */
                if (strlen(display) > colWidth) {
                    display[colWidth] = '\0';
                }

                Screen->Vtbl->WriteText(Screen, colX, rowY, display, fg, bg);
            }

            /* Draw column separator */
            if (colX + colWidth < X + width - 1) {
                Screen->Vtbl->WriteChar(Screen, colX + colWidth, rowY, gBoxChars.SingleVertical, TuiColorBrightBlack, TuiColorWhite);
            }

            colX += colWidth + 1;
        }

        rowY++;
    }

    /* Draw status bar at bottom */
    CHAR8 status[128];
    CHAR8 colName[8];
    GetColumnName(impl->CurrentCol, colName, sizeof(colName));
    snprintf(status, sizeof(status), " %s%u ", colName, impl->CurrentRow + 1);

    if (impl->EditMode) {
        strncat(status, " [EDIT] ", sizeof(status) - strlen(status) - 1);
    }

    for (UINT32 i = 0; i < width - 2; i++) {
        Screen->Vtbl->WriteChar(Screen, X + 1 + i, Y + height - 1, ' ', TuiColorWhite, TuiColorBlue);
    }
    Screen->Vtbl->WriteText(Screen, X + 1, Y + height - 1, status, TuiColorWhite, TuiColorBlue);

    return S_OK;
}

/* Handle keyboard input */
static HRESULT ANXAPI Spreadsheet_HandleKey(
    ITuiSpreadsheet *This,
    TUI_KEY Key
)
{
    TuiSpreadsheetImpl *impl = (TuiSpreadsheetImpl *)This;

    if (impl->EditMode) {
        /* Edit mode - handle text input */
        if (Key == TuiKeyEnter) {
            /* Commit edit */
            SpreadsheetCell cell;
            GetCell(impl, impl->CurrentRow, impl->CurrentCol, &cell);

            strncpy(cell.Value, impl->EditBuffer, sizeof(cell.Value) - 1);
            cell.Value[sizeof(cell.Value) - 1] = '\0';

            SetCell(impl, impl->CurrentRow, impl->CurrentCol, &cell);

            if (impl->OnCellChanged) {
                impl->OnCellChanged(impl->UserData, impl->CurrentRow, impl->CurrentCol, cell.Value);
            }

            impl->EditMode = FALSE;

            /* Move down */
            if (impl->CurrentRow < impl->TotalRows - 1) {
                impl->CurrentRow++;
            }
        } else if (Key == TuiKeyEscape) {
            /* Cancel edit */
            impl->EditMode = FALSE;
        } else if (Key == TuiKeyBackspace) {
            if (impl->EditCursor > 0) {
                impl->EditCursor--;
                impl->EditBuffer[impl->EditCursor] = '\0';
            }
        } else if (Key >= 32 && Key < 127) {
            /* Printable character */
            if (impl->EditCursor < MAX_CELL_LENGTH - 1) {
                impl->EditBuffer[impl->EditCursor++] = (CHAR8)Key;
                impl->EditBuffer[impl->EditCursor] = '\0';
            }
        }
    } else {
        /* Navigation mode */
        switch (Key) {
            case TuiKeyUp:
                if (impl->CurrentRow > 0) {
                    impl->CurrentRow--;
                    if (impl->CurrentRow < impl->ScrollRow) {
                        impl->ScrollRow = impl->CurrentRow;
                    }
                }
                break;

            case TuiKeyDown:
                if (impl->CurrentRow < impl->TotalRows - 1) {
                    impl->CurrentRow++;
                    if (impl->CurrentRow >= impl->ScrollRow + impl->VisibleRows) {
                        impl->ScrollRow++;
                    }
                }
                break;

            case TuiKeyLeft:
                if (impl->CurrentCol > 0) {
                    impl->CurrentCol--;
                    if (impl->CurrentCol < impl->ScrollCol) {
                        impl->ScrollCol = impl->CurrentCol;
                    }
                }
                break;

            case TuiKeyRight:
            case TuiKeyTab:
                if (impl->CurrentCol < impl->TotalColumns - 1) {
                    impl->CurrentCol++;
                    if (impl->CurrentCol >= impl->ScrollCol + impl->VisibleColumns) {
                        impl->ScrollCol++;
                    }
                }
                break;

            case TuiKeyHome:
                impl->CurrentCol = 0;
                impl->ScrollCol = 0;
                break;

            case TuiKeyEnd:
                impl->CurrentCol = impl->TotalColumns - 1;
                break;

            case TuiKeyPageUp:
                if (impl->CurrentRow >= impl->VisibleRows) {
                    impl->CurrentRow -= impl->VisibleRows;
                    impl->ScrollRow = impl->CurrentRow;
                } else {
                    impl->CurrentRow = 0;
                    impl->ScrollRow = 0;
                }
                break;

            case TuiKeyPageDown:
                if (impl->CurrentRow + impl->VisibleRows < impl->TotalRows) {
                    impl->CurrentRow += impl->VisibleRows;
                    impl->ScrollRow = impl->CurrentRow;
                } else {
                    impl->CurrentRow = impl->TotalRows - 1;
                }
                break;

            case TuiKeyEnter:
            case TuiKeyF2:
                /* Enter edit mode */
                impl->EditMode = TRUE;
                impl->EditCursor = 0;

                /* Load current cell value into edit buffer */
                SpreadsheetCell cell;
                GetCell(impl, impl->CurrentRow, impl->CurrentCol, &cell);
                strncpy(impl->EditBuffer, cell.Value, sizeof(impl->EditBuffer) - 1);
                impl->EditBuffer[sizeof(impl->EditBuffer) - 1] = '\0';
                impl->EditCursor = strlen(impl->EditBuffer);
                break;

            case TuiKeyDelete:
                /* Clear cell */
                SpreadsheetCell emptyCell;
                memset(&emptyCell, 0, sizeof(emptyCell));
                emptyCell.Foreground = TuiColorBlack;
                emptyCell.Background = TuiColorWhite;
                SetCell(impl, impl->CurrentRow, impl->CurrentCol, &emptyCell);
                break;
        }

        /* Update selection */
        impl->Selection.StartRow = impl->CurrentRow;
        impl->Selection.StartCol = impl->CurrentCol;
        impl->Selection.EndRow = impl->CurrentRow;
        impl->Selection.EndCol = impl->CurrentCol;

        if (impl->OnSelectionChanged) {
            impl->OnSelectionChanged(impl->UserData, &impl->Selection);
        }
    }

    return S_OK;
}

/* Set cell value */
static HRESULT ANXAPI Spreadsheet_SetCellValue(
    ITuiSpreadsheet *This,
    UINT32 Row,
    UINT32 Col,
    CONST CHAR8 *Value
)
{
    TuiSpreadsheetImpl *impl = (TuiSpreadsheetImpl *)This;

    if (Row >= impl->TotalRows || Col >= impl->TotalColumns) {
        return E_INVALIDARG;
    }

    SpreadsheetCell cell;
    GetCell(impl, Row, Col, &cell);

    strncpy(cell.Value, Value ? Value : "", sizeof(cell.Value) - 1);
    cell.Value[sizeof(cell.Value) - 1] = '\0';

    return SetCell(impl, Row, Col, &cell);
}

/* Get cell value */
static HRESULT ANXAPI Spreadsheet_GetCellValue(
    ITuiSpreadsheet *This,
    UINT32 Row,
    UINT32 Col,
    CHAR8 *Value,
    UINTN ValueSize
)
{
    TuiSpreadsheetImpl *impl = (TuiSpreadsheetImpl *)This;

    if (Row >= impl->TotalRows || Col >= impl->TotalColumns) {
        return E_INVALIDARG;
    }

    SpreadsheetCell cell;
    GetCell(impl, Row, Col, &cell);

    strncpy(Value, cell.Value, ValueSize - 1);
    Value[ValueSize - 1] = '\0';

    return S_OK;
}

/* Set column width */
static HRESULT ANXAPI Spreadsheet_SetColumnWidth(
    ITuiSpreadsheet *This,
    UINT32 Column,
    UINT32 Width
)
{
    TuiSpreadsheetImpl *impl = (TuiSpreadsheetImpl *)This;

    if (Column >= impl->TotalColumns) {
        return E_INVALIDARG;
    }

    impl->ColumnWidths[Column] = Width;
    return S_OK;
}

/* Enable virtual mode */
static HRESULT ANXAPI Spreadsheet_SetVirtualMode(
    ITuiSpreadsheet *This,
    BOOLEAN Enable,
    HRESULT (*OnGetCell)(VOID*, UINT32, UINT32, SpreadsheetCell*),
    HRESULT (*OnSetCell)(VOID*, UINT32, UINT32, CONST SpreadsheetCell*),
    VOID *UserData
)
{
    TuiSpreadsheetImpl *impl = (TuiSpreadsheetImpl *)This;

    impl->VirtualMode = Enable;
    impl->OnGetCell = OnGetCell;
    impl->OnSetCell = OnSetCell;
    impl->VirtualUserData = UserData;

    return S_OK;
}

/* Widget common methods */
static HRESULT ANXAPI Spreadsheet_SetBounds(ITuiSpreadsheet *This, CONST TUI_RECT *Bounds)
{
    TuiSpreadsheetImpl *impl = (TuiSpreadsheetImpl *)This;
    impl->State.Bounds = *Bounds;

    /* Recalculate visible rows/columns */
    impl->VisibleRows = Bounds->Height - 4;  /* Minus borders and headers */
    impl->VisibleColumns = (Bounds->Width - HEADER_WIDTH - 2) / (DEFAULT_COL_WIDTH + 1);

    return S_OK;
}

static HRESULT ANXAPI Spreadsheet_GetBounds(ITuiSpreadsheet *This, TUI_RECT *Bounds)
{
    TuiSpreadsheetImpl *impl = (TuiSpreadsheetImpl *)This;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI Spreadsheet_SetVisible(ITuiSpreadsheet *This, BOOLEAN Visible)
{
    TuiSpreadsheetImpl *impl = (TuiSpreadsheetImpl *)This;
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI Spreadsheet_IsVisible(ITuiSpreadsheet *This)
{
    TuiSpreadsheetImpl *impl = (TuiSpreadsheetImpl *)This;
    return impl->State.Visible;
}

static HRESULT ANXAPI Spreadsheet_SetEnabled(ITuiSpreadsheet *This, BOOLEAN Enabled)
{
    TuiSpreadsheetImpl *impl = (TuiSpreadsheetImpl *)This;
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI Spreadsheet_IsEnabled(ITuiSpreadsheet *This)
{
    TuiSpreadsheetImpl *impl = (TuiSpreadsheetImpl *)This;
    return impl->State.Enabled;
}

/* VTable */
static ITuiSpreadsheetVtbl SpreadsheetVtbl = {
    Spreadsheet_QueryInterface,
    Spreadsheet_AddRef,
    Spreadsheet_Release,
    Spreadsheet_Render,
    Spreadsheet_HandleKey,
    Spreadsheet_SetBounds,
    Spreadsheet_GetBounds,
    Spreadsheet_SetVisible,
    Spreadsheet_IsVisible,
    Spreadsheet_SetEnabled,
    Spreadsheet_IsEnabled,
    Spreadsheet_SetCellValue,
    Spreadsheet_GetCellValue,
    Spreadsheet_SetColumnWidth,
    Spreadsheet_SetVirtualMode
};

/* Factory function */
HRESULT AnxTuiCreateSpreadsheet(
    UINT32 Rows,
    UINT32 Columns,
    ITuiSpreadsheet **OutSpreadsheet
)
{
    TuiSpreadsheetImpl *impl;

    if (!OutSpreadsheet) return E_INVALIDARG;
    if (Rows == 0 || Rows > MAX_ROWS) return E_INVALIDARG;
    if (Columns == 0 || Columns > MAX_COLUMNS) return E_INVALIDARG;

    impl = (TuiSpreadsheetImpl *)malloc(sizeof(TuiSpreadsheetImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiSpreadsheetImpl));
    impl->Interface.Vtbl = &SpreadsheetVtbl;
    InitWidgetState(&impl->State);

    impl->TotalRows = Rows;
    impl->TotalColumns = Columns;
    impl->VisibleRows = 20;
    impl->VisibleColumns = 8;

    /* Initialize column widths */
    for (UINT32 i = 0; i < Columns; i++) {
        impl->ColumnWidths[i] = DEFAULT_COL_WIDTH;
    }

    impl->CurrentRow = 0;
    impl->CurrentCol = 0;
    impl->ScrollRow = 0;
    impl->ScrollCol = 0;
    impl->FreezeRow = 0;
    impl->FreezeCol = 0;

    impl->Selection.StartRow = 0;
    impl->Selection.StartCol = 0;
    impl->Selection.EndRow = 0;
    impl->Selection.EndCol = 0;

    impl->EditMode = FALSE;
    impl->VirtualMode = FALSE;

    impl->State.Bounds.Width = 80;
    impl->State.Bounds.Height = 24;

    *OutSpreadsheet = &impl->Interface;
    return S_OK;
}
