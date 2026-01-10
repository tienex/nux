/*
 * MessageBox Widget Implementation
 *
 * Modal dialog boxes with shadows for displaying messages
 * and getting user confirmation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

typedef enum {
    MsgBoxOK,
    MsgBoxOKCancel,
    MsgBoxYesNo,
    MsgBoxYesNoCancel,
    MsgBoxRetryCancel,
    MsgBoxAbortRetryIgnore
} MessageBoxButtons;

typedef enum {
    MsgBoxResultNone = 0,
    MsgBoxResultOK = 1,
    MsgBoxResultCancel = 2,
    MsgBoxResultYes = 3,
    MsgBoxResultNo = 4,
    MsgBoxResultRetry = 5,
    MsgBoxResultAbort = 6,
    MsgBoxResultIgnore = 7
} MessageBoxResult;

typedef enum {
    MsgBoxIconNone,
    MsgBoxIconInfo,
    MsgBoxIconWarning,
    MsgBoxIconError,
    MsgBoxIconQuestion
} MessageBoxIcon;

typedef struct {
    ITuiMessageBox Interface;
    WIDGET_STATE State;
    CHAR8 Title[128];
    CHAR8 Message[1024];
    MessageBoxButtons Buttons;
    MessageBoxIcon Icon;
    MessageBoxResult Result;
    INT32 SelectedButton;
    UINT32 Width;
    UINT32 Height;
} TuiMessageBoxImpl;

/* IUnknown methods */
static HRESULT ANXAPI MessageBox_QueryInterface(
    ITuiMessageBox *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI MessageBox_AddRef(ITuiMessageBox *This)
{
    TuiMessageBoxImpl *impl = (TuiMessageBoxImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI MessageBox_Release(ITuiMessageBox *This)
{
    TuiMessageBoxImpl *impl = (TuiMessageBoxImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* ITuiMessageBox methods */
static HRESULT ANXAPI MessageBox_SetTitle(
    ITuiMessageBox *This,
    CONST CHAR8 *Title
)
{
    TuiMessageBoxImpl *impl = (TuiMessageBoxImpl *)This;
    if (Title == NULL) return E_POINTER;

    strncpy(impl->Title, Title, sizeof(impl->Title) - 1);
    impl->Title[sizeof(impl->Title) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI MessageBox_SetMessage(
    ITuiMessageBox *This,
    CONST CHAR8 *Message
)
{
    TuiMessageBoxImpl *impl = (TuiMessageBoxImpl *)This;
    if (Message == NULL) return E_POINTER;

    strncpy(impl->Message, Message, sizeof(impl->Message) - 1);
    impl->Message[sizeof(impl->Message) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI MessageBox_SetButtons(
    ITuiMessageBox *This,
    UINT32 Buttons
)
{
    TuiMessageBoxImpl *impl = (TuiMessageBoxImpl *)This;
    impl->Buttons = (MessageBoxButtons)Buttons;
    impl->SelectedButton = 0;
    return S_OK;
}

static HRESULT ANXAPI MessageBox_SetIcon(
    ITuiMessageBox *This,
    UINT32 Icon
)
{
    TuiMessageBoxImpl *impl = (TuiMessageBoxImpl *)This;
    impl->Icon = (MessageBoxIcon)Icon;
    return S_OK;
}

static HRESULT ANXAPI MessageBox_Show(
    ITuiMessageBox *This,
    ITuiScreen *Screen
)
{
    TuiMessageBoxImpl *impl = (TuiMessageBoxImpl *)This;

    /* Reset result */
    impl->Result = MsgBoxResultNone;

    /* Compute box size */
    UINT32 msgLen = strlen(impl->Message);
    impl->Width = msgLen + 8;
    if (impl->Width < 40) impl->Width = 40;
    if (impl->Width > 70) impl->Width = 70;
    impl->Height = 10;  /* Adjust based on wrapped text */

    /* Enter modal loop (simplified - actual implementation would need
     * proper event loop integration) */

    /* For now, just render once */
    UINT32 screenWidth, screenHeight;
    Screen->Vtbl->GetDimensions(Screen, &screenWidth, &screenHeight);

    INT32 x = (screenWidth - impl->Width) / 2;
    INT32 y = (screenHeight - impl->Height) / 2;

    MessageBox_Render(This, Screen, x, y);
    Screen->Vtbl->Refresh(Screen);

    return S_OK;
}

static HRESULT ANXAPI MessageBox_GetResult(
    ITuiMessageBox *This,
    UINT32 *Result
)
{
    TuiMessageBoxImpl *impl = (TuiMessageBoxImpl *)This;
    if (Result == NULL) return E_POINTER;
    *Result = impl->Result;
    return S_OK;
}

/* Helper: Get icon character */
static CONST CHAR8 *GetIconChar(MessageBoxIcon Icon)
{
    switch (Icon) {
        case MsgBoxIconInfo:     return "ℹ";
        case MsgBoxIconWarning:  return "⚠";
        case MsgBoxIconError:    return "✖";
        case MsgBoxIconQuestion: return "?";
        default:                 return "";
    }
}

/* Helper: Word wrap text */
static VOID WrapText(
    CONST CHAR8 *Text,
    UINT32 MaxWidth,
    CHAR8 Lines[][256],
    UINT32 *LineCount
)
{
    UINT32 lineIdx = 0;
    UINT32 linePos = 0;
    UINT32 i;

    *LineCount = 0;

    for (i = 0; Text[i] != '\0' && lineIdx < 32; i++) {
        if (Text[i] == '\n' || linePos >= MaxWidth - 1) {
            Lines[lineIdx][linePos] = '\0';
            lineIdx++;
            linePos = 0;
            if (Text[i] == '\n') continue;
        }
        Lines[lineIdx][linePos++] = Text[i];
    }

    if (linePos > 0) {
        Lines[lineIdx][linePos] = '\0';
        lineIdx++;
    }

    *LineCount = lineIdx;
}

static HRESULT ANXAPI MessageBox_Render(
    ITuiMessageBox *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiMessageBoxImpl *impl = (TuiMessageBoxImpl *)This;
    CHAR8 lines[32][256];
    UINT32 lineCount;
    UINT32 i;
    CHAR8 display[256];
    INT32 buttonY;

    if (!impl->State.Visible) return S_OK;

    /* Draw shadow (offset by 2,1) */
    for (i = 0; i < impl->Height + 1; i++) {
        ClearRect(Screen, X + 2, Y + i + 1, impl->Width, 1, TuiColorBlack);
    }

    /* Draw box */
    DrawBoxSingle(Screen, X, Y, impl->Width, impl->Height,
                  TuiColorBlack, TuiColorWhite);

    /* Draw title bar */
    snprintf(display, sizeof(display), " %s ", impl->Title);
    Screen->Vtbl->WriteText(Screen, X + 2, Y, display,
                            TuiColorWhite, TuiColorBlue);

    /* Draw icon and message */
    WrapText(impl->Message, impl->Width - 6, lines, &lineCount);

    INT32 iconX = X + 3;
    INT32 textX = X + 5;
    if (impl->Icon != MsgBoxIconNone) {
        Screen->Vtbl->WriteText(Screen, iconX, Y + 2, GetIconChar(impl->Icon),
                                TuiColorRed, TuiColorWhite);
        textX += 2;
    }

    for (i = 0; i < lineCount && i < 5; i++) {
        Screen->Vtbl->WriteText(Screen, textX, Y + 2 + i, lines[i],
                                TuiColorBlack, TuiColorWhite);
    }

    /* Draw buttons */
    buttonY = Y + impl->Height - 2;
    INT32 buttonX = X + impl->Width / 2;

    switch (impl->Buttons) {
        case MsgBoxOK:
            /* [  OK  ] */
            buttonX -= 4;
            if (impl->SelectedButton == 0) {
                Screen->Vtbl->WriteText(Screen, buttonX, buttonY, "[  OK  ]",
                                        TuiColorBlack, TuiColorYellow);
            } else {
                Screen->Vtbl->WriteText(Screen, buttonX, buttonY, "   OK   ",
                                        TuiColorBlack, TuiColorWhite);
            }
            break;

        case MsgBoxOKCancel:
            /* [  OK  ] [ Cancel ] */
            buttonX -= 10;
            if (impl->SelectedButton == 0) {
                Screen->Vtbl->WriteText(Screen, buttonX, buttonY, "[  OK  ]",
                                        TuiColorBlack, TuiColorYellow);
            } else {
                Screen->Vtbl->WriteText(Screen, buttonX, buttonY, "   OK   ",
                                        TuiColorBlack, TuiColorWhite);
            }

            buttonX += 10;
            if (impl->SelectedButton == 1) {
                Screen->Vtbl->WriteText(Screen, buttonX, buttonY, "[ Cancel ]",
                                        TuiColorBlack, TuiColorYellow);
            } else {
                Screen->Vtbl->WriteText(Screen, buttonX, buttonY, "  Cancel  ",
                                        TuiColorBlack, TuiColorWhite);
            }
            break;

        case MsgBoxYesNo:
            /* [ Yes ] [ No ] */
            buttonX -= 8;
            if (impl->SelectedButton == 0) {
                Screen->Vtbl->WriteText(Screen, buttonX, buttonY, "[ Yes ]",
                                        TuiColorBlack, TuiColorYellow);
            } else {
                Screen->Vtbl->WriteText(Screen, buttonX, buttonY, "  Yes  ",
                                        TuiColorBlack, TuiColorWhite);
            }

            buttonX += 9;
            if (impl->SelectedButton == 1) {
                Screen->Vtbl->WriteText(Screen, buttonX, buttonY, "[ No ]",
                                        TuiColorBlack, TuiColorYellow);
            } else {
                Screen->Vtbl->WriteText(Screen, buttonX, buttonY, "  No  ",
                                        TuiColorBlack, TuiColorWhite);
            }
            break;

        /* Add more button combinations as needed */
    }

    return S_OK;
}

static HRESULT ANXAPI MessageBox_HandleKey(
    ITuiMessageBox *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiMessageBoxImpl *impl = (TuiMessageBoxImpl *)This;
    INT32 maxButton = 1;  /* Depends on button type */

    switch (impl->Buttons) {
        case MsgBoxOK:
            maxButton = 0;
            break;
        case MsgBoxOKCancel:
        case MsgBoxYesNo:
        case MsgBoxRetryCancel:
            maxButton = 1;
            break;
        case MsgBoxYesNoCancel:
        case MsgBoxAbortRetryIgnore:
            maxButton = 2;
            break;
    }

    switch (Key) {
        case TuiKeyLeft:
            if (impl->SelectedButton > 0) {
                impl->SelectedButton--;
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyRight:
        case TuiKeyTab:
            if (impl->SelectedButton < maxButton) {
                impl->SelectedButton++;
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyEnter:
            /* Set result based on selected button and button type */
            if (impl->Buttons == MsgBoxOK) {
                impl->Result = MsgBoxResultOK;
            } else if (impl->Buttons == MsgBoxOKCancel) {
                impl->Result = (impl->SelectedButton == 0) ?
                               MsgBoxResultOK : MsgBoxResultCancel;
            } else if (impl->Buttons == MsgBoxYesNo) {
                impl->Result = (impl->SelectedButton == 0) ?
                               MsgBoxResultYes : MsgBoxResultNo;
            }
            /* Close message box (actual implementation would exit modal loop) */
            *Handled = TRUE;
            return S_OK;

        case TuiKeyEsc:
            /* Cancel */
            impl->Result = MsgBoxResultCancel;
            *Handled = TRUE;
            return S_OK;
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiMessageBox_Vtbl MessageBoxVtbl = {
    MessageBox_QueryInterface,
    MessageBox_AddRef,
    MessageBox_Release,
    MessageBox_SetTitle,
    MessageBox_SetMessage,
    MessageBox_SetButtons,
    MessageBox_SetIcon,
    MessageBox_Show,
    MessageBox_GetResult,
    MessageBox_Render,
    MessageBox_HandleKey
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateMessageBox(
    IN  CONST CHAR8 *Title,
    IN  CONST CHAR8 *Message,
    IN  UINT32 Buttons,
    OUT ITuiMessageBox **MessageBox
)
{
    TuiMessageBoxImpl *impl;

    if (MessageBox == NULL) return E_POINTER;

    impl = (TuiMessageBoxImpl *)calloc(1, sizeof(TuiMessageBoxImpl));
    if (impl == NULL) {
        *MessageBox = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &MessageBoxVtbl;
    InitWidgetState(&impl->State);

    if (Title != NULL) {
        strncpy(impl->Title, Title, sizeof(impl->Title) - 1);
        impl->Title[sizeof(impl->Title) - 1] = '\0';
    } else {
        strcpy(impl->Title, "Message");
    }

    if (Message != NULL) {
        strncpy(impl->Message, Message, sizeof(impl->Message) - 1);
        impl->Message[sizeof(impl->Message) - 1] = '\0';
    } else {
        impl->Message[0] = '\0';
    }

    impl->Buttons = (MessageBoxButtons)Buttons;
    impl->Icon = MsgBoxIconNone;
    impl->Result = MsgBoxResultNone;
    impl->SelectedButton = 0;
    impl->Width = 40;
    impl->Height = 10;

    *MessageBox = &impl->Interface;
    return S_OK;
}

/* Helper function: Show simple message box */
HRESULT ANXAPI AnxTuiShowMessageBox(
    ITuiScreen *Screen,
    CONST CHAR8 *Title,
    CONST CHAR8 *Message,
    UINT32 Buttons,
    UINT32 *Result
)
{
    ITuiMessageBox *msgBox;
    HRESULT hr;

    hr = AnxTuiCreateMessageBox(Title, Message, Buttons, &msgBox);
    if (FAILED(hr)) return hr;

    hr = msgBox->Vtbl->Show(msgBox, Screen);
    if (FAILED(hr)) {
        msgBox->Vtbl->Release(msgBox);
        return hr;
    }

    hr = msgBox->Vtbl->GetResult(msgBox, Result);
    msgBox->Vtbl->Release(msgBox);

    return hr;
}
