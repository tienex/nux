/*
 * Calendar Desk Accessory
 *
 * Interactive calendar with date selection and navigation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "widgets_common.h"

#define DAYS_IN_WEEK 7
#define MAX_WEEKS 6

typedef struct {
    ITuiDeskApp Interface;
    WIDGET_STATE State;
    CHAR8 Title[64];

    /* Calendar state */
    INT32 Year;
    INT32 Month;  /* 1-12 */
    INT32 Day;    /* 1-31 */

    /* Selection */
    INT32 SelectedDay;
    INT32 HoverDay;

    /* Display dimensions */
    UINT32 Width;
    UINT32 Height;

    /* Callback */
    HRESULT (*OnDateSelected)(VOID *UserData, INT32 Year, INT32 Month, INT32 Day);
    VOID *UserData;

} CalendarImpl;

static CONST CHAR8 *MonthNames[] = {
    "", "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static CONST CHAR8 *DayNames[] = {
    "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"
};

/* Helper: Check if year is leap year */
static BOOLEAN IsLeapYear(INT32 year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/* Helper: Get days in month */
static INT32 GetDaysInMonth(INT32 year, INT32 month)
{
    static INT32 daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month == 2 && IsLeapYear(year)) {
        return 29;
    }
    return daysInMonth[month];
}

/* Helper: Get day of week for first day of month (0=Sunday) */
static INT32 GetFirstDayOfMonth(INT32 year, INT32 month)
{
    /* Zeller's congruence algorithm */
    INT32 q = 1;  /* day of month */
    INT32 m = month;
    INT32 y = year;

    if (m < 3) {
        m += 12;
        y--;
    }

    INT32 k = y % 100;
    INT32 j = y / 100;

    INT32 h = (q + ((13 * (m + 1)) / 5) + k + (k / 4) + (j / 4) - (2 * j)) % 7;

    /* Convert to 0=Sunday */
    return (h + 6) % 7;
}

/* IUnknown methods */
static HRESULT ANXAPI Calendar_QueryInterface(
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

static UINTN ANXAPI Calendar_AddRef(ITuiDeskApp *This)
{
    CalendarImpl *impl = (CalendarImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Calendar_Release(ITuiDeskApp *This)
{
    CalendarImpl *impl = (CalendarImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* Render calendar */
static HRESULT ANXAPI Calendar_Render(
    ITuiDeskApp *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    CalendarImpl *impl = (CalendarImpl *)This;
    CHAR8 buffer[128];
    INT32 currentY = Y;

    if (!impl->State.Visible) return S_OK;

    /* Draw title with month and year */
    snprintf(buffer, sizeof(buffer), " %s %d ",
             MonthNames[impl->Month], impl->Year);
    INT32 titleX = X + (impl->Width - strlen(buffer)) / 2;
    Screen->Vtbl->WriteText(Screen, titleX, currentY, buffer,
                           TuiColorBlack, TuiColorCyan);
    currentY += 2;

    /* Draw day names */
    INT32 dayX = X + 2;
    for (INT32 i = 0; i < DAYS_IN_WEEK; i++) {
        Screen->Vtbl->WriteText(Screen, dayX, currentY, DayNames[i],
                               TuiColorYellow, TuiColorBlack);
        dayX += 4;
    }
    currentY++;

    /* Draw separator */
    for (INT32 i = 0; i < impl->Width - 4; i++) {
        Screen->Vtbl->WriteChar(Screen, X + 2 + i, currentY, gBoxChars.Horizontal,
                               TuiColorWhite, TuiColorBlack);
    }
    currentY++;

    /* Get calendar info */
    INT32 firstDay = GetFirstDayOfMonth(impl->Year, impl->Month);
    INT32 daysInMonth = GetDaysInMonth(impl->Year, impl->Month);

    /* Get today's date for highlighting */
    time_t now = time(NULL);
    struct tm *today = localtime(&now);
    INT32 todayYear = today->tm_year + 1900;
    INT32 todayMonth = today->tm_mon + 1;
    INT32 todayDay = today->tm_mday;

    /* Draw calendar grid */
    INT32 dayNum = 1;
    for (INT32 week = 0; week < MAX_WEEKS && dayNum <= daysInMonth; week++) {
        dayX = X + 2;

        for (INT32 dow = 0; dow < DAYS_IN_WEEK; dow++) {
            if ((week == 0 && dow < firstDay) || dayNum > daysInMonth) {
                /* Empty cell */
                Screen->Vtbl->WriteText(Screen, dayX, currentY, "  ",
                                       TuiColorWhite, TuiColorBlack);
            } else {
                /* Day number */
                snprintf(buffer, sizeof(buffer), "%2d", dayNum);

                TUI_COLOR fg = TuiColorWhite;
                TUI_COLOR bg = TuiColorBlack;

                /* Highlight today */
                if (impl->Year == todayYear && impl->Month == todayMonth && dayNum == todayDay) {
                    fg = TuiColorBlack;
                    bg = TuiColorGreen;
                }

                /* Highlight selected day */
                if (dayNum == impl->SelectedDay) {
                    fg = TuiColorBlack;
                    bg = TuiColorYellow;
                }

                /* Highlight hover day */
                if (dayNum == impl->HoverDay && dayNum != impl->SelectedDay) {
                    fg = TuiColorBlack;
                    bg = TuiColorBlue;
                }

                /* Sunday in red */
                if (dow == 0 && bg == TuiColorBlack) {
                    fg = TuiColorRed;
                }

                Screen->Vtbl->WriteText(Screen, dayX, currentY, buffer, fg, bg);
                dayNum++;
            }

            dayX += 4;
        }

        currentY++;
    }

    /* Draw navigation help */
    currentY++;
    Screen->Vtbl->WriteText(Screen, X + 2, currentY,
                           "Arrows:Navigate  Enter:Select  PgUp/Dn:Month  Home/End:Year",
                           TuiColorBrightBlack, TuiColorBlack);

    return S_OK;
}

/* Handle input */
static HRESULT ANXAPI Calendar_HandleKey(
    ITuiDeskApp *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    CalendarImpl *impl = (CalendarImpl *)This;

    *Handled = FALSE;

    if (!impl->State.Enabled) return S_OK;

    INT32 daysInMonth = GetDaysInMonth(impl->Year, impl->Month);

    switch (Key) {
        case TuiKeyUp:
            impl->SelectedDay -= 7;
            if (impl->SelectedDay < 1) impl->SelectedDay = 1;
            *Handled = TRUE;
            break;

        case TuiKeyDown:
            impl->SelectedDay += 7;
            if (impl->SelectedDay > daysInMonth) impl->SelectedDay = daysInMonth;
            *Handled = TRUE;
            break;

        case TuiKeyLeft:
            impl->SelectedDay--;
            if (impl->SelectedDay < 1) {
                /* Previous month */
                impl->Month--;
                if (impl->Month < 1) {
                    impl->Month = 12;
                    impl->Year--;
                }
                impl->SelectedDay = GetDaysInMonth(impl->Year, impl->Month);
            }
            *Handled = TRUE;
            break;

        case TuiKeyRight:
            impl->SelectedDay++;
            if (impl->SelectedDay > daysInMonth) {
                /* Next month */
                impl->Month++;
                if (impl->Month > 12) {
                    impl->Month = 1;
                    impl->Year++;
                }
                impl->SelectedDay = 1;
            }
            *Handled = TRUE;
            break;

        case TuiKeyPageUp:
            /* Previous month */
            impl->Month--;
            if (impl->Month < 1) {
                impl->Month = 12;
                impl->Year--;
            }
            /* Adjust day if necessary */
            daysInMonth = GetDaysInMonth(impl->Year, impl->Month);
            if (impl->SelectedDay > daysInMonth) {
                impl->SelectedDay = daysInMonth;
            }
            *Handled = TRUE;
            break;

        case TuiKeyPageDown:
            /* Next month */
            impl->Month++;
            if (impl->Month > 12) {
                impl->Month = 1;
                impl->Year++;
            }
            /* Adjust day if necessary */
            daysInMonth = GetDaysInMonth(impl->Year, impl->Month);
            if (impl->SelectedDay > daysInMonth) {
                impl->SelectedDay = daysInMonth;
            }
            *Handled = TRUE;
            break;

        case TuiKeyHome:
            /* Previous year */
            impl->Year--;
            /* Adjust for leap year */
            if (impl->Month == 2) {
                daysInMonth = GetDaysInMonth(impl->Year, impl->Month);
                if (impl->SelectedDay > daysInMonth) {
                    impl->SelectedDay = daysInMonth;
                }
            }
            *Handled = TRUE;
            break;

        case TuiKeyEnd:
            /* Next year */
            impl->Year++;
            /* Adjust for leap year */
            if (impl->Month == 2) {
                daysInMonth = GetDaysInMonth(impl->Year, impl->Month);
                if (impl->SelectedDay > daysInMonth) {
                    impl->SelectedDay = daysInMonth;
                }
            }
            *Handled = TRUE;
            break;

        case TuiKeyEnter:
            /* Select date */
            if (impl->OnDateSelected) {
                impl->OnDateSelected(impl->UserData, impl->Year, impl->Month, impl->SelectedDay);
            }
            *Handled = TRUE;
            break;

        case 't':
        case 'T':
            /* Jump to today */
            {
                time_t now = time(NULL);
                struct tm *today = localtime(&now);
                impl->Year = today->tm_year + 1900;
                impl->Month = today->tm_mon + 1;
                impl->SelectedDay = today->tm_mday;
                *Handled = TRUE;
            }
            break;
    }

    return S_OK;
}

/* Set callback */
static HRESULT ANXAPI Calendar_SetCallback(
    ITuiDeskApp *This,
    HRESULT (*Callback)(VOID *UserData, INT32 Year, INT32 Month, INT32 Day),
    VOID *UserData
)
{
    CalendarImpl *impl = (CalendarImpl *)This;
    impl->OnDateSelected = Callback;
    impl->UserData = UserData;
    return S_OK;
}

/* Get title */
static HRESULT ANXAPI Calendar_GetTitle(
    ITuiDeskApp *This,
    CONST CHAR8 **OutTitle
)
{
    CalendarImpl *impl = (CalendarImpl *)This;
    *OutTitle = impl->Title;
    return S_OK;
}

/* VTable */
static CONST ITuiDeskApp_Vtbl CalendarVtbl = {
    Calendar_QueryInterface,
    Calendar_AddRef,
    Calendar_Release,
    Calendar_Render,
    Calendar_HandleKey,
    Calendar_GetTitle
};

/* Factory function */
HRESULT AnxTuiCreateCalendar(ITuiDeskApp **OutCalendar)
{
    CalendarImpl *impl;
    time_t now;
    struct tm *today;

    if (!OutCalendar) return E_POINTER;

    impl = (CalendarImpl *)calloc(1, sizeof(CalendarImpl));
    if (!impl) {
        *OutCalendar = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &CalendarVtbl;
    InitWidgetState(&impl->State);

    strcpy(impl->Title, "Calendar");

    /* Initialize to today's date */
    now = time(NULL);
    today = localtime(&now);
    impl->Year = today->tm_year + 1900;
    impl->Month = today->tm_mon + 1;
    impl->Day = today->tm_mday;
    impl->SelectedDay = impl->Day;
    impl->HoverDay = 0;

    impl->Width = 32;
    impl->Height = 12;

    impl->OnDateSelected = NULL;
    impl->UserData = NULL;

    *OutCalendar = &impl->Interface;
    return S_OK;
}
