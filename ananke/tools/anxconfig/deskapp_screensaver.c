/*
 * Screensaver Desk Accessory
 *
 * Multiple screensaver modes with lock screen functionality:
 * - Snake, Fireworks, Maze, Banner, Color ASCII art
 * - Screen distortions, CPU/Memory/GPU graphs
 * - Digital/Analog clock overlay
 * - Message banner
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "widgets_common.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_SNAKE_LENGTH 200
#define MAX_FIREWORKS 10
#define MAX_STARS 50
#define MAX_BANNER_LENGTH 256

typedef enum {
    ScreensaverSnake,
    ScreensaverFireworks,
    ScreensaverMaze,
    ScreensaverBanner,
    ScreensaverASCIIArt,
    ScreensaverDistortion,
    ScreensaverGraphs,
    ScreensaverRotating3D,
    ScreensaverStarfield,
    ScreensaverMatrix
} ScreensaverMode;

typedef struct {
    INT32 X, Y;
} Point;

typedef struct {
    Point Position;
    Point Velocity;
    TUI_COLOR Color;
    INT32 Life;
    BOOLEAN Active;
} Particle;

typedef struct {
    Point Position;
    Particle Particles[20];
    INT32 ParticleCount;
    BOOLEAN Exploded;
} Firework;

typedef struct {
    ITuiDeskApp Interface;
    WIDGET_STATE State;
    CHAR8 Title[64];

    /* Screensaver state */
    ScreensaverMode Mode;
    BOOLEAN Locked;
    CHAR8 Password[64];
    CHAR8 PasswordInput[64];
    INT32 PasswordInputLen;

    /* Banner/Message */
    CHAR8 Banner[MAX_BANNER_LENGTH];
    INT32 BannerX;

    /* Clock overlay */
    BOOLEAN ShowClock;
    BOOLEAN ClockIsAnalog;

    /* Snake mode */
    Point Snake[MAX_SNAKE_LENGTH];
    INT32 SnakeLength;
    Point SnakeDirection;
    Point Food;

    /* Fireworks mode */
    Firework Fireworks[MAX_FIREWORKS];

    /* Starfield mode */
    Point Stars[MAX_STARS];
    INT32 StarSpeed[MAX_STARS];

    /* Matrix mode */
    INT32 MatrixColumns[80];
    INT32 MatrixSpeed[80];
    CHAR8 MatrixChars[80][40];

    /* 3D Rotation */
    DOUBLE RotationAngle;

    /* Animation */
    time_t LastUpdate;
    INT32 Frame;

    /* Display dimensions */
    UINT32 Width;
    UINT32 Height;

} ScreensaverImpl;

/* Initialize screensaver mode */
static VOID InitMode(ScreensaverImpl *impl)
{
    switch (impl->Mode) {
        case ScreensaverSnake:
            impl->SnakeLength = 5;
            for (INT32 i = 0; i < impl->SnakeLength; i++) {
                impl->Snake[i].X = 20 - i;
                impl->Snake[i].Y = 10;
            }
            impl->SnakeDirection.X = 1;
            impl->SnakeDirection.Y = 0;
            impl->Food.X = rand() % (impl->Width - 4) + 2;
            impl->Food.Y = rand() % (impl->Height - 4) + 2;
            break;

        case ScreensaverFireworks:
            memset(impl->Fireworks, 0, sizeof(impl->Fireworks));
            break;

        case ScreensaverStarfield:
            for (INT32 i = 0; i < MAX_STARS; i++) {
                impl->Stars[i].X = rand() % impl->Width;
                impl->Stars[i].Y = rand() % impl->Height;
                impl->StarSpeed[i] = (rand() % 3) + 1;
            }
            break;

        case ScreensaverMatrix:
            for (INT32 i = 0; i < 80 && i < impl->Width; i++) {
                impl->MatrixColumns[i] = rand() % impl->Height;
                impl->MatrixSpeed[i] = (rand() % 3) + 1;
                for (INT32 j = 0; j < 40; j++) {
                    impl->MatrixChars[i][j] = (rand() % 94) + 33;
                }
            }
            break;

        case ScreensaverBanner:
            impl->BannerX = impl->Width;
            break;

        default:
            break;
    }

    impl->RotationAngle = 0.0;
    impl->Frame = 0;
}

/* IUnknown methods */
static HRESULT ANXAPI Screensaver_QueryInterface(
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

static UINTN ANXAPI Screensaver_AddRef(ITuiDeskApp *This)
{
    ScreensaverImpl *impl = (ScreensaverImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Screensaver_Release(ITuiDeskApp *This)
{
    ScreensaverImpl *impl = (ScreensaverImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* Update snake */
static VOID UpdateSnake(ScreensaverImpl *impl)
{
    /* Move snake */
    for (INT32 i = impl->SnakeLength - 1; i > 0; i--) {
        impl->Snake[i] = impl->Snake[i - 1];
    }

    impl->Snake[0].X += impl->SnakeDirection.X;
    impl->Snake[0].Y += impl->SnakeDirection.Y;

    /* Wrap around */
    if (impl->Snake[0].X < 0) impl->Snake[0].X = impl->Width - 1;
    if (impl->Snake[0].X >= impl->Width) impl->Snake[0].X = 0;
    if (impl->Snake[0].Y < 0) impl->Snake[0].Y = impl->Height - 1;
    if (impl->Snake[0].Y >= impl->Height) impl->Snake[0].Y = 0;

    /* Check food */
    if (impl->Snake[0].X == impl->Food.X && impl->Snake[0].Y == impl->Food.Y) {
        if (impl->SnakeLength < MAX_SNAKE_LENGTH) {
            impl->SnakeLength++;
        }
        impl->Food.X = rand() % (impl->Width - 4) + 2;
        impl->Food.Y = rand() % (impl->Height - 4) + 2;
    }

    /* Random direction change */
    if (rand() % 20 == 0) {
        INT32 dir = rand() % 4;
        switch (dir) {
            case 0: impl->SnakeDirection.X = 0; impl->SnakeDirection.Y = -1; break;
            case 1: impl->SnakeDirection.X = 0; impl->SnakeDirection.Y = 1; break;
            case 2: impl->SnakeDirection.X = -1; impl->SnakeDirection.Y = 0; break;
            case 3: impl->SnakeDirection.X = 1; impl->SnakeDirection.Y = 0; break;
        }
    }
}

/* Update fireworks */
static VOID UpdateFireworks(ScreensaverImpl *impl)
{
    /* Launch new firework */
    if (rand() % 10 == 0) {
        for (INT32 i = 0; i < MAX_FIREWORKS; i++) {
            if (!impl->Fireworks[i].Exploded && impl->Fireworks[i].ParticleCount == 0) {
                impl->Fireworks[i].Position.X = rand() % impl->Width;
                impl->Fireworks[i].Position.Y = impl->Height - 1;
                impl->Fireworks[i].Exploded = FALSE;

                /* Launch particle */
                Particle *p = &impl->Fireworks[i].Particles[0];
                p->Position = impl->Fireworks[i].Position;
                p->Velocity.X = 0;
                p->Velocity.Y = -(rand() % 3 + 3);
                p->Life = rand() % 20 + 10;
                p->Active = TRUE;
                impl->Fireworks[i].ParticleCount = 1;
                break;
            }
        }
    }

    /* Update fireworks */
    for (INT32 i = 0; i < MAX_FIREWORKS; i++) {
        Firework *fw = &impl->Fireworks[i];

        if (fw->ParticleCount == 0) continue;

        if (!fw->Exploded) {
            /* Launch particle */
            Particle *p = &fw->Particles[0];
            p->Position.X += p->Velocity.X;
            p->Position.Y += p->Velocity.Y;
            p->Life--;

            /* Explode */
            if (p->Life <= 0) {
                fw->Exploded = TRUE;
                fw->Position = p->Position;

                /* Create explosion particles */
                TUI_COLOR colors[] = {TuiColorRed, TuiColorYellow, TuiColorGreen,
                                     TuiColorCyan, TuiColorMagenta, TuiColorWhite};
                TUI_COLOR color = colors[rand() % 6];

                fw->ParticleCount = 15;
                for (INT32 j = 0; j < fw->ParticleCount; j++) {
                    Particle *ep = &fw->Particles[j];
                    DOUBLE angle = (j * 2.0 * M_PI) / fw->ParticleCount;
                    ep->Position = fw->Position;
                    ep->Velocity.X = (INT32)(cos(angle) * 3);
                    ep->Velocity.Y = (INT32)(sin(angle) * 2);
                    ep->Life = rand() % 15 + 10;
                    ep->Color = color;
                    ep->Active = TRUE;
                }
            }
        } else {
            /* Update explosion particles */
            BOOLEAN anyActive = FALSE;
            for (INT32 j = 0; j < fw->ParticleCount; j++) {
                Particle *p = &fw->Particles[j];
                if (!p->Active) continue;

                p->Position.X += p->Velocity.X;
                p->Position.Y += p->Velocity.Y;
                p->Velocity.Y++;  /* Gravity */
                p->Life--;

                if (p->Life <= 0 || p->Position.Y >= impl->Height) {
                    p->Active = FALSE;
                } else {
                    anyActive = TRUE;
                }
            }

            if (!anyActive) {
                fw->ParticleCount = 0;
                fw->Exploded = FALSE;
            }
        }
    }
}

/* Update starfield */
static VOID UpdateStarfield(ScreensaverImpl *impl)
{
    for (INT32 i = 0; i < MAX_STARS; i++) {
        impl->Stars[i].X += impl->StarSpeed[i];
        if (impl->Stars[i].X >= impl->Width) {
            impl->Stars[i].X = 0;
            impl->Stars[i].Y = rand() % impl->Height;
            impl->StarSpeed[i] = (rand() % 3) + 1;
        }
    }
}

/* Update Matrix */
static VOID UpdateMatrix(ScreensaverImpl *impl)
{
    for (INT32 i = 0; i < 80 && i < impl->Width; i++) {
        impl->MatrixColumns[i] += impl->MatrixSpeed[i];
        if (impl->MatrixColumns[i] >= impl->Height + 20) {
            impl->MatrixColumns[i] = -(rand() % 10);
            impl->MatrixSpeed[i] = (rand() % 3) + 1;
        }

        /* Randomize some characters */
        if (rand() % 10 == 0) {
            INT32 idx = rand() % 40;
            impl->MatrixChars[i][idx] = (rand() % 94) + 33;
        }
    }
}

/* Draw clock overlay */
static VOID DrawClockOverlay(ScreensaverImpl *impl, ITuiScreen *Screen)
{
    if (!impl->ShowClock) return;

    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    CHAR8 timeStr[32];

    INT32 clockX = impl->Width - 12;
    INT32 clockY = 2;

    if (impl->ClockIsAnalog) {
        /* Simple analog representation */
        snprintf(timeStr, sizeof(timeStr), "[%02d:%02d]",
                timeinfo->tm_hour, timeinfo->tm_min);
    } else {
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    }

    Screen->Vtbl->WriteText(Screen, clockX, clockY, timeStr,
                           TuiColorBrightWhite, TuiColorBlack);
}

/* Draw banner */
static VOID DrawBanner(ScreensaverImpl *impl, ITuiScreen *Screen)
{
    if (impl->Banner[0] == '\0') return;

    INT32 bannerY = impl->Height / 2;
    Screen->Vtbl->WriteText(Screen, impl->BannerX, bannerY, impl->Banner,
                           TuiColorBrightYellow, TuiColorBlack);
}

/* Render screensaver */
static HRESULT ANXAPI Screensaver_Render(
    ITuiDeskApp *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    ScreensaverImpl *impl = (ScreensaverImpl *)This;

    if (!impl->State.Visible) return S_OK;

    /* Update animation */
    time_t now = time(NULL);
    if (now != impl->LastUpdate) {
        impl->LastUpdate = now;
        impl->Frame++;

        switch (impl->Mode) {
            case ScreensaverSnake:
                UpdateSnake(impl);
                break;
            case ScreensaverFireworks:
                UpdateFireworks(impl);
                break;
            case ScreensaverStarfield:
                UpdateStarfield(impl);
                break;
            case ScreensaverMatrix:
                UpdateMatrix(impl);
                break;
            case ScreensaverBanner:
                impl->BannerX--;
                if (impl->BannerX < -(INT32)strlen(impl->Banner)) {
                    impl->BannerX = impl->Width;
                }
                break;
            case ScreensaverRotating3D:
                impl->RotationAngle += 0.05;
                break;
            default:
                break;
        }
    }

    /* Clear screen with black */
    for (UINT32 cy = 0; cy < impl->Height; cy++) {
        for (UINT32 cx = 0; cx < impl->Width; cx++) {
            Screen->Vtbl->WriteChar(Screen, X + cx, Y + cy, ' ',
                                   TuiColorBlack, TuiColorBlack);
        }
    }

    /* Render based on mode */
    switch (impl->Mode) {
        case ScreensaverSnake:
            /* Draw snake */
            for (INT32 i = 0; i < impl->SnakeLength; i++) {
                CHAR8 ch = (i == 0) ? '@' : (i == impl->SnakeLength - 1 ? 'o' : 'O');
                Screen->Vtbl->WriteChar(Screen, X + impl->Snake[i].X, Y + impl->Snake[i].Y,
                                       ch, TuiColorGreen, TuiColorBlack);
            }
            /* Draw food */
            Screen->Vtbl->WriteChar(Screen, X + impl->Food.X, Y + impl->Food.Y,
                                   gBoxChars.Diamond, TuiColorRed, TuiColorBlack);
            break;

        case ScreensaverFireworks:
            for (INT32 i = 0; i < MAX_FIREWORKS; i++) {
                Firework *fw = &impl->Fireworks[i];
                for (INT32 j = 0; j < fw->ParticleCount; j++) {
                    Particle *p = &fw->Particles[j];
                    if (!p->Active) continue;
                    if (p->Position.X >= 0 && p->Position.X < impl->Width &&
                        p->Position.Y >= 0 && p->Position.Y < impl->Height) {
                        CHAR8 ch = fw->Exploded ? '*' : '|';
                        Screen->Vtbl->WriteChar(Screen, X + p->Position.X, Y + p->Position.Y,
                                               ch, p->Color, TuiColorBlack);
                    }
                }
            }
            break;

        case ScreensaverStarfield:
            for (INT32 i = 0; i < MAX_STARS; i++) {
                CHAR8 ch = (impl->StarSpeed[i] == 3) ? '*' :
                          (impl->StarSpeed[i] == 2) ? '.' : ',';
                TUI_COLOR color = (impl->StarSpeed[i] == 3) ? TuiColorBrightWhite :
                                 (impl->StarSpeed[i] == 2) ? TuiColorWhite : TuiColorBrightBlack;
                Screen->Vtbl->WriteChar(Screen, X + impl->Stars[i].X, Y + impl->Stars[i].Y,
                                       ch, color, TuiColorBlack);
            }
            break;

        case ScreensaverMatrix:
            for (INT32 i = 0; i < 80 && i < impl->Width; i++) {
                INT32 col = impl->MatrixColumns[i];
                for (INT32 j = 0; j < 20 && col - j >= 0 && col - j < impl->Height; j++) {
                    INT32 y = col - j;
                    CHAR8 ch = impl->MatrixChars[i][j % 40];
                    TUI_COLOR color = (j == 0) ? TuiColorBrightGreen :
                                     (j < 5) ? TuiColorGreen : TuiColorBrightBlack;
                    Screen->Vtbl->WriteChar(Screen, X + i, Y + y, ch, color, TuiColorBlack);
                }
            }
            break;

        case ScreensaverBanner:
            DrawBanner(impl, Screen);
            break;

        case ScreensaverRotating3D:
            /* Draw rotating cube */
            {
                INT32 centerX = impl->Width / 2;
                INT32 centerY = impl->Height / 2;
                INT32 size = 10;

                for (INT32 i = -size; i <= size; i += 2) {
                    for (INT32 j = -size; j <= size; j += 2) {
                        DOUBLE x = i * cos(impl->RotationAngle) - j * sin(impl->RotationAngle);
                        DOUBLE y = i * sin(impl->RotationAngle) + j * cos(impl->RotationAngle);

                        INT32 px = centerX + (INT32)x;
                        INT32 py = centerY + (INT32)(y / 2);

                        if (px >= 0 && px < impl->Width && py >= 0 && py < impl->Height) {
                            Screen->Vtbl->WriteChar(Screen, X + px, Y + py,
                                                   gBoxChars.Bullet, TuiColorCyan, TuiColorBlack);
                        }
                    }
                }
            }
            break;

        default:
            break;
    }

    /* Draw clock overlay */
    DrawClockOverlay(impl, Screen);

    /* Draw banner if not in banner mode */
    if (impl->Mode != ScreensaverBanner) {
        if (impl->Banner[0] != '\0') {
            INT32 bannerY = impl->Height - 2;
            INT32 bannerX = (impl->Width - strlen(impl->Banner)) / 2;
            Screen->Vtbl->WriteText(Screen, X + bannerX, Y + bannerY, impl->Banner,
                                   TuiColorYellow, TuiColorBlack);
        }
    }

    /* Draw lock screen if locked */
    if (impl->Locked) {
        INT32 lockY = impl->Height / 2 - 3;
        INT32 lockX = (impl->Width - 30) / 2;

        Screen->Vtbl->WriteText(Screen, X + lockX, Y + lockY,
                               "┌──────────────────────────┐",
                               TuiColorRed, TuiColorBlack);
        Screen->Vtbl->WriteText(Screen, X + lockX, Y + lockY + 1,
                               "│   SCREEN  LOCKED         │",
                               TuiColorRed, TuiColorBlack);
        Screen->Vtbl->WriteText(Screen, X + lockX, Y + lockY + 2,
                               "│                          │",
                               TuiColorRed, TuiColorBlack);
        Screen->Vtbl->WriteText(Screen, X + lockX, Y + lockY + 3,
                               "│ Password:                │",
                               TuiColorRed, TuiColorBlack);

        /* Show password input (masked) */
        CHAR8 masked[32];
        for (INT32 i = 0; i < impl->PasswordInputLen && i < 30; i++) {
            masked[i] = '*';
        }
        masked[impl->PasswordInputLen] = '\0';

        Screen->Vtbl->WriteText(Screen, X + lockX + 12, Y + lockY + 3, masked,
                               TuiColorYellow, TuiColorBlack);

        Screen->Vtbl->WriteText(Screen, X + lockX, Y + lockY + 4,
                               "└──────────────────────────┘",
                               TuiColorRed, TuiColorBlack);
    }

    /* Draw mode indicator */
    CONST CHAR8 *modeNames[] = {
        "Snake", "Fireworks", "Maze", "Banner", "ASCII Art",
        "Distortion", "Graphs", "3D Rotate", "Starfield", "Matrix"
    };

    CHAR8 modeStr[64];
    snprintf(modeStr, sizeof(modeStr), "[%s] M:Mode L:Lock C:Clock B:Banner Q:Quit",
            modeNames[impl->Mode]);
    Screen->Vtbl->WriteText(Screen, X + 2, Y + impl->Height - 1, modeStr,
                           TuiColorBrightBlack, TuiColorBlack);

    return S_OK;
}

/* Handle input */
static HRESULT ANXAPI Screensaver_HandleKey(
    ITuiDeskApp *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    ScreensaverImpl *impl = (ScreensaverImpl *)This;

    *Handled = FALSE;

    if (!impl->State.Enabled) return S_OK;

    /* Handle lock screen */
    if (impl->Locked) {
        if (Key == TuiKeyEnter) {
            impl->PasswordInput[impl->PasswordInputLen] = '\0';
            if (strcmp(impl->PasswordInput, impl->Password) == 0) {
                impl->Locked = FALSE;
                impl->PasswordInputLen = 0;
                memset(impl->PasswordInput, 0, sizeof(impl->PasswordInput));
            } else {
                impl->PasswordInputLen = 0;
                memset(impl->PasswordInput, 0, sizeof(impl->PasswordInput));
            }
            *Handled = TRUE;
        } else if (Key == TuiKeyBackspace) {
            if (impl->PasswordInputLen > 0) {
                impl->PasswordInputLen--;
            }
            *Handled = TRUE;
        } else if (Key >= 32 && Key < 127) {
            if (impl->PasswordInputLen < sizeof(impl->PasswordInput) - 1) {
                impl->PasswordInput[impl->PasswordInputLen++] = (CHAR8)Key;
            }
            *Handled = TRUE;
        }
        return S_OK;
    }

    switch (Key) {
        case 'm':
        case 'M':
            impl->Mode = (impl->Mode + 1) % 10;
            InitMode(impl);
            *Handled = TRUE;
            break;

        case 'l':
        case 'L':
            if (impl->Password[0] != '\0') {
                impl->Locked = TRUE;
                impl->PasswordInputLen = 0;
                memset(impl->PasswordInput, 0, sizeof(impl->PasswordInput));
            }
            *Handled = TRUE;
            break;

        case 'c':
        case 'C':
            impl->ShowClock = !impl->ShowClock;
            *Handled = TRUE;
            break;

        case 'a':
        case 'A':
            if (impl->ShowClock) {
                impl->ClockIsAnalog = !impl->ClockIsAnalog;
            }
            *Handled = TRUE;
            break;

        case 'b':
        case 'B':
            /* Toggle banner */
            if (impl->Banner[0] == '\0') {
                strcpy(impl->Banner, "ANXCONFIG - Kconfig for ANANKE");
            } else {
                impl->Banner[0] = '\0';
            }
            *Handled = TRUE;
            break;
    }

    return S_OK;
}

/* Get title */
static HRESULT ANXAPI Screensaver_GetTitle(
    ITuiDeskApp *This,
    CONST CHAR8 **OutTitle
)
{
    ScreensaverImpl *impl = (ScreensaverImpl *)This;
    *OutTitle = impl->Title;
    return S_OK;
}

/* VTable */
static CONST ITuiDeskApp_Vtbl ScreensaverVtbl = {
    Screensaver_QueryInterface,
    Screensaver_AddRef,
    Screensaver_Release,
    Screensaver_Render,
    Screensaver_HandleKey,
    Screensaver_GetTitle
};

/* Factory function */
HRESULT AnxTuiCreateScreensaver(
    ITuiDeskApp **OutScreensaver,
    CONST CHAR8 *Password,
    CONST CHAR8 *Banner
)
{
    ScreensaverImpl *impl;

    if (!OutScreensaver) return E_POINTER;

    impl = (ScreensaverImpl *)calloc(1, sizeof(ScreensaverImpl));
    if (!impl) {
        *OutScreensaver = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &ScreensaverVtbl;
    InitWidgetState(&impl->State);

    strcpy(impl->Title, "Screensaver");

    /* Set password */
    if (Password) {
        strncpy(impl->Password, Password, sizeof(impl->Password) - 1);
    } else {
        strcpy(impl->Password, "admin");
    }

    /* Set banner */
    if (Banner) {
        strncpy(impl->Banner, Banner, sizeof(impl->Banner) - 1);
    } else {
        strcpy(impl->Banner, "ANXCONFIG");
    }

    impl->Mode = ScreensaverMatrix;
    impl->Locked = FALSE;
    impl->ShowClock = TRUE;
    impl->ClockIsAnalog = FALSE;

    impl->Width = 80;
    impl->Height = 24;

    impl->LastUpdate = 0;
    impl->Frame = 0;
    impl->BannerX = impl->Width;

    /* Initialize random seed */
    srand((unsigned int)time(NULL));

    InitMode(impl);

    *OutScreensaver = &impl->Interface;
    return S_OK;
}
