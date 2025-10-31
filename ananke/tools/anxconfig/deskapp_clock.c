/*
 * Clock Desk Accessory
 *
 * Analog and digital clock display with multiple timezones.
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

typedef enum {
    ClockModeAnalog,
    ClockModeDigital,
    ClockModeBoth
} ClockMode;

typedef struct {
    ITuiDeskApp Interface;
    WIDGET_STATE State;
    CHAR8 Title[64];

    /* Clock state */
    ClockMode Mode;
    INT32 TimezoneOffset;  /* Hours offset from system time */
    BOOLEAN Show24Hour;
    BOOLEAN ShowSeconds;
    BOOLEAN ShowDate;

    /* Display dimensions */
    UINT32 Width;
    UINT32 Height;

    /* Animation */
    time_t LastUpdate;
    INT32 BlinkState;  /* For digital clock colon blinking */

} ClockImpl;

static CONST CHAR8 *MonthNames[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static CONST CHAR8 *DayNames[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

/* IUnknown methods */
static HRESULT ANXAPI Clock_QueryInterface(
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

static UINTN ANXAPI Clock_AddRef(ITuiDeskApp *This)
{
    ClockImpl *impl = (ClockImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Clock_Release(ITuiDeskApp *This)
{
    ClockImpl *impl = (ClockImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* Helper: Draw analog clock */
static VOID DrawAnalogClock(
    ClockImpl *impl,
    ITuiScreen *Screen,
    INT32 CenterX,
    INT32 CenterY,
    INT32 Radius,
    INT32 Hour,
    INT32 Minute,
    INT32 Second
)
{
    /* Draw clock face circle */
    for (INT32 angle = 0; angle < 360; angle += 15) {
        DOUBLE rad = angle * M_PI / 180.0;
        INT32 x = CenterX + (INT32)(Radius * cos(rad));
        INT32 y = CenterY + (INT32)((Radius / 2) * sin(rad));  /* Adjust for character aspect ratio */

        CHAR8 marker = (angle % 90 == 0) ? gBoxChars.CrossHatch : '.';
        Screen->Vtbl->WriteChar(Screen, x, y, marker,
                               TuiColorBrightBlack, TuiColorBlack);
    }

    /* Draw hour markers */
    Screen->Vtbl->WriteText(Screen, CenterX, CenterY - Radius / 2, "12",
                           TuiColorWhite, TuiColorBlack);
    Screen->Vtbl->WriteText(Screen, CenterX + Radius, CenterY, "3",
                           TuiColorWhite, TuiColorBlack);
    Screen->Vtbl->WriteText(Screen, CenterX, CenterY + Radius / 2, "6",
                           TuiColorWhite, TuiColorBlack);
    Screen->Vtbl->WriteText(Screen, CenterX - Radius, CenterY, "9",
                           TuiColorWhite, TuiColorBlack);

    /* Draw hour hand */
    {
        DOUBLE hourAngle = ((Hour % 12) * 30 + Minute * 0.5 - 90) * M_PI / 180.0;
        INT32 hourLength = Radius * 2 / 3;

        for (INT32 i = 0; i < hourLength; i++) {
            INT32 x = CenterX + (INT32)(i * cos(hourAngle));
            INT32 y = CenterY + (INT32)((i / 2) * sin(hourAngle));
            Screen->Vtbl->WriteChar(Screen, x, y, gBoxChars.Solid,
                                   TuiColorYellow, TuiColorBlack);
        }
    }

    /* Draw minute hand */
    {
        DOUBLE minuteAngle = (Minute * 6 - 90) * M_PI / 180.0;
        INT32 minuteLength = Radius * 4 / 5;

        for (INT32 i = 0; i < minuteLength; i++) {
            INT32 x = CenterX + (INT32)(i * cos(minuteAngle));
            INT32 y = CenterY + (INT32)((i / 2) * sin(minuteAngle));
            Screen->Vtbl->WriteChar(Screen, x, y, gBoxChars.Medium,
                                   TuiColorCyan, TuiColorBlack);
        }
    }

    /* Draw second hand */
    if (impl->ShowSeconds) {
        DOUBLE secondAngle = (Second * 6 - 90) * M_PI / 180.0;
        INT32 secondLength = Radius;

        for (INT32 i = 0; i < secondLength; i++) {
            INT32 x = CenterX + (INT32)(i * cos(secondAngle));
            INT32 y = CenterY + (INT32)((i / 2) * sin(secondAngle));
            Screen->Vtbl->WriteChar(Screen, x, y, '.',
                                   TuiColorRed, TuiColorBlack);
        }
    }

    /* Draw center dot */
    Screen->Vtbl->WriteChar(Screen, CenterX, CenterY, gBoxChars.Bullet,
                           TuiColorWhite, TuiColorBlack);
}

/* Helper: Draw digital clock */
static VOID DrawDigitalClock(
    ClockImpl *impl,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    INT32 Hour,
    INT32 Minute,
    INT32 Second
)
{
    CHAR8 timeStr[64];
    CHAR8 colonChar = (impl->BlinkState % 2 == 0) ? ':' : ' ';

    if (impl->Show24Hour) {
        if (impl->ShowSeconds) {
            snprintf(timeStr, sizeof(timeStr), "%02d%c%02d%c%02d",
                    Hour, colonChar, Minute, colonChar, Second);
        } else {
            snprintf(timeStr, sizeof(timeStr), "%02d%c%02d",
                    Hour, colonChar, Minute);
        }
    } else {
        INT32 displayHour = Hour % 12;
        if (displayHour == 0) displayHour = 12;
        CONST CHAR8 *ampm = (Hour >= 12) ? "PM" : "AM";

        if (impl->ShowSeconds) {
            snprintf(timeStr, sizeof(timeStr), "%2d%c%02d%c%02d %s",
                    displayHour, colonChar, Minute, colonChar, Second, ampm);
        } else {
            snprintf(timeStr, sizeof(timeStr), "%2d%c%02d %s",
                    displayHour, colonChar, Minute, ampm);
        }
    }

    /* Draw large digits */
    Screen->Vtbl->WriteText(Screen, X, Y, timeStr,
                           TuiColorBrightGreen, TuiColorBlack);
}

/* Render clock */
static HRESULT ANXAPI Clock_Render(
    ITuiDeskApp *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    ClockImpl *impl = (ClockImpl *)This;
    CHAR8 buffer[128];

    if (!impl->State.Visible) return S_OK;

    /* Get current time */
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);

    /* Apply timezone offset */
    timeinfo->tm_hour += impl->TimezoneOffset;
    if (timeinfo->tm_hour < 0) timeinfo->tm_hour += 24;
    if (timeinfo->tm_hour >= 24) timeinfo->tm_hour -= 24;

    INT32 hour = timeinfo->tm_hour;
    INT32 minute = timeinfo->tm_min;
    INT32 second = timeinfo->tm_sec;

    /* Update blink state */
    if (now != impl->LastUpdate) {
        impl->LastUpdate = now;
        impl->BlinkState++;
    }

    /* Draw title */
    snprintf(buffer, sizeof(buffer), " %s ", impl->Title);
    INT32 titleX = X + (impl->Width - strlen(buffer)) / 2;
    Screen->Vtbl->WriteText(Screen, titleX, Y, buffer,
                           TuiColorBlack, TuiColorCyan);
    Y += 2;

    /* Draw clock based on mode */
    switch (impl->Mode) {
        case ClockModeAnalog:
            DrawAnalogClock(impl, Screen, X + impl->Width / 2, Y + 6, 8,
                          hour, minute, second);
            Y += 13;
            break;

        case ClockModeDigital:
            DrawDigitalClock(impl, Screen, X + 4, Y + 2,
                           hour, minute, second);
            Y += 5;
            break;

        case ClockModeBoth:
            DrawAnalogClock(impl, Screen, X + impl->Width / 2, Y + 6, 6,
                          hour, minute, second);
            Y += 13;
            DrawDigitalClock(impl, Screen, X + 6, Y,
                           hour, minute, second);
            Y += 2;
            break;
    }

    /* Draw date if enabled */
    if (impl->ShowDate) {
        snprintf(buffer, sizeof(buffer), "%s, %s %d, %d",
                DayNames[timeinfo->tm_wday],
                MonthNames[timeinfo->tm_mon],
                timeinfo->tm_mday,
                timeinfo->tm_year + 1900);
        INT32 dateX = X + (impl->Width - strlen(buffer)) / 2;
        Screen->Vtbl->WriteText(Screen, dateX, Y, buffer,
                               TuiColorYellow, TuiColorBlack);
        Y++;
    }

    /* Draw help */
    Screen->Vtbl->WriteText(Screen, X + 2, Y + 1,
                           "M:Mode  +/-:Timezone  T:24hr  S:Seconds  D:Date",
                           TuiColorBrightBlack, TuiColorBlack);

    return S_OK;
}

/* Handle input */
static HRESULT ANXAPI Clock_HandleKey(
    ITuiDeskApp *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    ClockImpl *impl = (ClockImpl *)This;

    *Handled = FALSE;

    if (!impl->State.Enabled) return S_OK;

    switch (Key) {
        case 'm':
        case 'M':
            /* Cycle through modes */
            impl->Mode = (impl->Mode + 1) % 3;
            *Handled = TRUE;
            break;

        case '+':
        case '=':
            /* Increase timezone offset */
            impl->TimezoneOffset++;
            if (impl->TimezoneOffset > 12) impl->TimezoneOffset = 12;
            *Handled = TRUE;
            break;

        case '-':
        case '_':
            /* Decrease timezone offset */
            impl->TimezoneOffset--;
            if (impl->TimezoneOffset < -12) impl->TimezoneOffset = -12;
            *Handled = TRUE;
            break;

        case 't':
        case 'T':
            /* Toggle 24-hour mode */
            impl->Show24Hour = !impl->Show24Hour;
            *Handled = TRUE;
            break;

        case 's':
        case 'S':
            /* Toggle seconds display */
            impl->ShowSeconds = !impl->ShowSeconds;
            *Handled = TRUE;
            break;

        case 'd':
        case 'D':
            /* Toggle date display */
            impl->ShowDate = !impl->ShowDate;
            *Handled = TRUE;
            break;

        case '0':
            /* Reset to local timezone */
            impl->TimezoneOffset = 0;
            *Handled = TRUE;
            break;
    }

    return S_OK;
}

/* Get title */
static HRESULT ANXAPI Clock_GetTitle(
    ITuiDeskApp *This,
    CONST CHAR8 **OutTitle
)
{
    ClockImpl *impl = (ClockImpl *)This;
    *OutTitle = impl->Title;
    return S_OK;
}

/* VTable */
static CONST ITuiDeskApp_Vtbl ClockVtbl = {
    Clock_QueryInterface,
    Clock_AddRef,
    Clock_Release,
    Clock_Render,
    Clock_HandleKey,
    Clock_GetTitle
};

/* Factory function */
HRESULT AnxTuiCreateClock(ITuiDeskApp **OutClock)
{
    ClockImpl *impl;

    if (!OutClock) return E_POINTER;

    impl = (ClockImpl *)calloc(1, sizeof(ClockImpl));
    if (!impl) {
        *OutClock = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &ClockVtbl;
    InitWidgetState(&impl->State);

    strcpy(impl->Title, "Clock");

    impl->Mode = ClockModeBoth;
    impl->TimezoneOffset = 0;
    impl->Show24Hour = FALSE;
    impl->ShowSeconds = TRUE;
    impl->ShowDate = TRUE;

    impl->Width = 40;
    impl->Height = 20;

    impl->LastUpdate = 0;
    impl->BlinkState = 0;

    *OutClock = &impl->Interface;
    return S_OK;
}
