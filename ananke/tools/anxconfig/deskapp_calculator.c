/*
 * Calculator Desk Accessory
 *
 * Scientific calculator with basic and advanced functions.
 * Can be launched as a desk accessory within the TUI system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "widgets_common.h"

#define MAX_DISPLAY_LENGTH 32
#define MAX_EXPRESSION_LENGTH 256

typedef enum {
    CalcModeBasic,
    CalcModeScientific,
    CalcModeProgrammer
} CalculatorMode;

typedef enum {
    CalcOpNone,
    CalcOpAdd,
    CalcOpSubtract,
    CalcOpMultiply,
    CalcOpDivide,
    CalcOpPower,
    CalcOpModulo
} CalculatorOperation;

typedef struct {
    ITuiDeskApp Interface;
    WIDGET_STATE State;
    CHAR8 Title[64];
    CHAR8 Display[MAX_DISPLAY_LENGTH];
    CHAR8 Expression[MAX_EXPRESSION_LENGTH];
    DOUBLE Accumulator;
    DOUBLE CurrentValue;
    CalculatorOperation PendingOp;
    CalculatorMode Mode;
    BOOLEAN NewNumber;
    BOOLEAN Error;
    UINT32 Width;
    UINT32 Height;
    INT32 SelectedButton;
} CalculatorImpl;

/* IUnknown methods */
static HRESULT ANXAPI Calculator_QueryInterface(
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

static UINTN ANXAPI Calculator_AddRef(ITuiDeskApp *This)
{
    CalculatorImpl *impl = (CalculatorImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Calculator_Release(ITuiDeskApp *This)
{
    CalculatorImpl *impl = (CalculatorImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* Helper: Update display */
static VOID UpdateDisplay(CalculatorImpl *impl)
{
    if (impl->Error) {
        strcpy(impl->Display, "Error");
    } else if (impl->NewNumber && impl->Display[0] == '\0') {
        strcpy(impl->Display, "0");
    } else {
        /* Format the current value */
        if (floor(impl->CurrentValue) == impl->CurrentValue) {
            /* Integer */
            snprintf(impl->Display, sizeof(impl->Display),
                     "%.0f", impl->CurrentValue);
        } else {
            /* Decimal */
            snprintf(impl->Display, sizeof(impl->Display),
                     "%.10g", impl->CurrentValue);
        }
    }
}

/* Helper: Perform pending operation */
static VOID PerformOperation(CalculatorImpl *impl)
{
    switch (impl->PendingOp) {
        case CalcOpAdd:
            impl->Accumulator += impl->CurrentValue;
            break;
        case CalcOpSubtract:
            impl->Accumulator -= impl->CurrentValue;
            break;
        case CalcOpMultiply:
            impl->Accumulator *= impl->CurrentValue;
            break;
        case CalcOpDivide:
            if (impl->CurrentValue != 0) {
                impl->Accumulator /= impl->CurrentValue;
            } else {
                impl->Error = TRUE;
            }
            break;
        case CalcOpPower:
            impl->Accumulator = pow(impl->Accumulator, impl->CurrentValue);
            break;
        case CalcOpModulo:
            impl->Accumulator = fmod(impl->Accumulator, impl->CurrentValue);
            break;
        default:
            impl->Accumulator = impl->CurrentValue;
            break;
    }

    impl->CurrentValue = impl->Accumulator;
    impl->PendingOp = CalcOpNone;
}

/* Helper: Handle digit input */
static VOID InputDigit(CalculatorImpl *impl, CHAR8 digit)
{
    if (impl->Error) {
        impl->Error = FALSE;
        impl->Display[0] = '\0';
    }

    if (impl->NewNumber) {
        impl->Display[0] = digit;
        impl->Display[1] = '\0';
        impl->NewNumber = FALSE;
    } else {
        UINTN len = strlen(impl->Display);
        if (len < MAX_DISPLAY_LENGTH - 1) {
            impl->Display[len] = digit;
            impl->Display[len + 1] = '\0';
        }
    }

    impl->CurrentValue = atof(impl->Display);
}

/* Helper: Handle decimal point */
static VOID InputDecimal(CalculatorImpl *impl)
{
    if (impl->NewNumber) {
        strcpy(impl->Display, "0.");
        impl->NewNumber = FALSE;
    } else if (strchr(impl->Display, '.') == NULL) {
        UINTN len = strlen(impl->Display);
        if (len < MAX_DISPLAY_LENGTH - 1) {
            impl->Display[len] = '.';
            impl->Display[len + 1] = '\0';
        }
    }
}

/* Helper: Handle operation */
static VOID InputOperation(CalculatorImpl *impl, CalculatorOperation op)
{
    if (!impl->NewNumber && impl->PendingOp != CalcOpNone) {
        PerformOperation(impl);
    } else {
        impl->Accumulator = impl->CurrentValue;
    }

    impl->PendingOp = op;
    impl->NewNumber = TRUE;
    UpdateDisplay(impl);
}

/* Helper: Handle equals */
static VOID InputEquals(CalculatorImpl *impl)
{
    if (impl->PendingOp != CalcOpNone) {
        PerformOperation(impl);
    }
    impl->NewNumber = TRUE;
    UpdateDisplay(impl);
}

/* Helper: Clear */
static VOID Clear(CalculatorImpl *impl)
{
    impl->Display[0] = '\0';
    impl->CurrentValue = 0;
    impl->Accumulator = 0;
    impl->PendingOp = CalcOpNone;
    impl->NewNumber = TRUE;
    impl->Error = FALSE;
    UpdateDisplay(impl);
}

/* ITuiDeskApp methods */
static HRESULT ANXAPI Calculator_GetTitle(
    ITuiDeskApp *This,
    CHAR8 *Buffer,
    UINTN BufferSize
)
{
    CalculatorImpl *impl = (CalculatorImpl *)This;
    if (Buffer == NULL) return E_POINTER;

    strncpy(Buffer, impl->Title, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI Calculator_Show(
    ITuiDeskApp *This,
    ITuiScreen *Screen
)
{
    CalculatorImpl *impl = (CalculatorImpl *)This;
    impl->State.Visible = TRUE;
    return S_OK;
}

static HRESULT ANXAPI Calculator_Hide(ITuiDeskApp *This)
{
    CalculatorImpl *impl = (CalculatorImpl *)This;
    impl->State.Visible = FALSE;
    return S_OK;
}

static HRESULT ANXAPI Calculator_Render(
    ITuiDeskApp *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    CalculatorImpl *impl = (CalculatorImpl *)This;
    UINT32 i, j;
    CHAR8 display[64];

    if (!impl->State.Visible) return S_OK;

    /* Draw shadow */
    for (i = 0; i < impl->Height + 1; i++) {
        ClearRect(Screen, X + 2, Y + i + 1, impl->Width, 1, TuiColorBlack);
    }

    /* Draw calculator box */
    DrawBoxSingle(Screen, X, Y, impl->Width, impl->Height,
                  TuiColorBlack, TuiColorWhite);

    /* Draw title bar */
    snprintf(display, sizeof(display), " %s ", impl->Title);
    Screen->Vtbl->WriteText(Screen, X + 2, Y, display,
                            TuiColorWhite, TuiColorBlue);

    /* Draw display */
    snprintf(display, sizeof(display), " %-*s ",
             impl->Width - 4, impl->Display);
    Screen->Vtbl->WriteText(Screen, X + 1, Y + 2, display,
                            TuiColorBlack, TuiColorGreen);

    /* Draw buttons */
    CONST CHAR8 *buttons[] = {
        "7", "8", "9", "/",
        "4", "5", "6", "*",
        "1", "2", "3", "-",
        "0", ".", "=", "+",
        "C", "^", "%", "√"
    };

    INT32 buttonY = Y + 4;
    INT32 buttonIndex = 0;

    for (i = 0; i < 5; i++) {
        INT32 buttonX = X + 2;
        for (j = 0; j < 4; j++) {
            if (buttonIndex < 20) {
                snprintf(display, sizeof(display), "[%s]", buttons[buttonIndex]);

                if (buttonIndex == impl->SelectedButton) {
                    Screen->Vtbl->WriteText(Screen, buttonX, buttonY, display,
                                            TuiColorBlack, TuiColorYellow);
                } else {
                    Screen->Vtbl->WriteText(Screen, buttonX, buttonY, display,
                                            TuiColorBlack, TuiColorWhite);
                }

                buttonX += 5;
                buttonIndex++;
            }
        }
        buttonY++;
    }

    /* Draw mode indicator */
    CONST CHAR8 *modeStr = "";
    switch (impl->Mode) {
        case CalcModeBasic: modeStr = "Basic"; break;
        case CalcModeScientific: modeStr = "Scientific"; break;
        case CalcModeProgrammer: modeStr = "Programmer"; break;
    }
    Screen->Vtbl->WriteText(Screen, X + 2, Y + impl->Height - 2, modeStr,
                            TuiColorBlack, TuiColorWhite);

    return S_OK;
}

static HRESULT ANXAPI Calculator_HandleKey(
    ITuiDeskApp *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    CalculatorImpl *impl = (CalculatorImpl *)This;

    if (!impl->State.Visible) {
        *Handled = FALSE;
        return S_OK;
    }

    /* Handle numeric keys */
    if (Key >= '0' && Key <= '9') {
        InputDigit(impl, (CHAR8)Key);
        UpdateDisplay(impl);
        *Handled = TRUE;
        return S_OK;
    }

    /* Handle operations */
    switch (Key) {
        case '+':
            InputOperation(impl, CalcOpAdd);
            *Handled = TRUE;
            return S_OK;

        case '-':
            InputOperation(impl, CalcOpSubtract);
            *Handled = TRUE;
            return S_OK;

        case '*':
            InputOperation(impl, CalcOpMultiply);
            *Handled = TRUE;
            return S_OK;

        case '/':
            InputOperation(impl, CalcOpDivide);
            *Handled = TRUE;
            return S_OK;

        case '^':
            InputOperation(impl, CalcOpPower);
            *Handled = TRUE;
            return S_OK;

        case '%':
            InputOperation(impl, CalcOpModulo);
            *Handled = TRUE;
            return S_OK;

        case '.':
            InputDecimal(impl);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyEnter:
        case '=':
            InputEquals(impl);
            *Handled = TRUE;
            return S_OK;

        case 'c':
        case 'C':
        case TuiKeyEsc:
            Clear(impl);
            *Handled = TRUE;
            return S_OK;

        case 's':
        case 'S':
            /* Square root */
            if (impl->CurrentValue >= 0) {
                impl->CurrentValue = sqrt(impl->CurrentValue);
                impl->NewNumber = TRUE;
                UpdateDisplay(impl);
            } else {
                impl->Error = TRUE;
                UpdateDisplay(impl);
            }
            *Handled = TRUE;
            return S_OK;
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiDeskApp_Vtbl CalculatorVtbl = {
    Calculator_QueryInterface,
    Calculator_AddRef,
    Calculator_Release,
    Calculator_GetTitle,
    Calculator_Show,
    Calculator_Hide,
    Calculator_Render,
    Calculator_HandleKey
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateCalculator(OUT ITuiDeskApp **Calculator)
{
    CalculatorImpl *impl;

    if (Calculator == NULL) return E_POINTER;

    impl = (CalculatorImpl *)calloc(1, sizeof(CalculatorImpl));
    if (impl == NULL) {
        *Calculator = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &CalculatorVtbl;
    InitWidgetState(&impl->State);

    strcpy(impl->Title, "Calculator");
    impl->Display[0] = '\0';
    impl->Expression[0] = '\0';
    impl->Accumulator = 0;
    impl->CurrentValue = 0;
    impl->PendingOp = CalcOpNone;
    impl->Mode = CalcModeBasic;
    impl->NewNumber = TRUE;
    impl->Error = FALSE;
    impl->Width = 24;
    impl->Height = 12;
    impl->SelectedButton = -1;

    UpdateDisplay(impl);

    *Calculator = &impl->Interface;
    return S_OK;
}
