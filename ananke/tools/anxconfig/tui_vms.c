/*
 * OpenVMS Console TUI Backend
 *
 * Implements the Ananke TUI interfaces using VMS SMG$ (Screen Management) facility.
 * Used on OpenVMS systems (VAX, Alpha, Itanium, x86-64).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ananke/tui.h>
#include <ananke/anxconfig.h>

#ifdef __VMS
#include <smgdef.h>
#include <smg$routines.h>
#include <descrip.h>
#include <iodef.h>
#include <ttdef.h>
#include <tt2def.h>
#include <starlet.h>

/* TuiScreen structure */
typedef struct {
    ITuiScreen Interface;
    UINTN RefCount;
    BOOLEAN Initialized;
    unsigned long PasteBoardId;
    unsigned long DisplayId;
    unsigned long KeyboardId;
} TuiScreen;

/* Create VMS descriptor */
#define MAKE_DESC(str) { strlen(str), DSC$K_DTYPE_T, DSC$K_CLASS_S, str }

/* Color mapping to VMS SMG attributes */
static unsigned long MapColor(TUI_COLOR Fg, TUI_COLOR Bg)
{
    unsigned long attr = 0;

    /* VMS uses rendition sets - simplified mapping */
    switch (Fg) {
        case TUI_COLOR_BLACK:   attr |= SMG$M_USER1; break;
        case TUI_COLOR_BLUE:    attr |= SMG$M_BLUE; break;
        case TUI_COLOR_GREEN:   attr |= SMG$M_GREEN; break;
        case TUI_COLOR_CYAN:    attr |= SMG$M_CYAN; break;
        case TUI_COLOR_RED:     attr |= SMG$M_RED; break;
        case TUI_COLOR_MAGENTA: attr |= SMG$M_MAGENTA; break;
        case TUI_COLOR_YELLOW:  attr |= SMG$M_YELLOW; break;
        case TUI_COLOR_WHITE:   attr |= SMG$M_BOLD; break;
        default:                break;
    }

    /* Background color (limited support) */
    if (Bg == TUI_COLOR_BLUE) {
        attr |= SMG$M_REVERSE;
    }

    return attr;
}

/* Key mapping from VMS terminal */
static TUI_KEY MapVMSKey(unsigned short keycode, char ch)
{
    /* ASCII characters */
    if (ch >= 32 && ch <= 126) {
        return (TUI_KEY)ch;
    }

    switch (ch) {
        case 13:  return TUI_KEY_ENTER;
        case 27:  return TUI_KEY_ESC;
        case 127: return TUI_KEY_BACKSPACE;
        case 9:   return TUI_KEY_TAB;
    }

    /* VMS keypad and function keys */
    switch (keycode) {
        case SMG$K_TRM_UP:       return TUI_KEY_UP;
        case SMG$K_TRM_DOWN:     return TUI_KEY_DOWN;
        case SMG$K_TRM_LEFT:     return TUI_KEY_LEFT;
        case SMG$K_TRM_RIGHT:    return TUI_KEY_RIGHT;
        case SMG$K_TRM_F6:       return TUI_KEY_F1;
        case SMG$K_TRM_F7:       return TUI_KEY_F2;
        case SMG$K_TRM_F8:       return TUI_KEY_F3;
        case SMG$K_TRM_F9:       return TUI_KEY_F4;
        case SMG$K_TRM_F10:      return TUI_KEY_F5;
        case SMG$K_TRM_F11:      return TUI_KEY_F6;
        case SMG$K_TRM_F12:      return TUI_KEY_F7;
        case SMG$K_TRM_F13:      return TUI_KEY_F8;
        case SMG$K_TRM_F14:      return TUI_KEY_F9;
        case SMG$K_TRM_HELP:     return TUI_KEY_F10;
        case SMG$K_TRM_DO:       return TUI_KEY_F11;
        case SMG$K_TRM_F17:      return TUI_KEY_F12;
        case SMG$K_TRM_REMOVE:   return TUI_KEY_DELETE;
        case SMG$K_TRM_INSERT_HERE: return TUI_KEY_INSERT;
        case SMG$K_TRM_FIND:     return TUI_KEY_HOME;
        case SMG$K_TRM_SELECT:   return TUI_KEY_END;
        case SMG$K_TRM_PREV_SCREEN: return TUI_KEY_PAGEUP;
        case SMG$K_TRM_NEXT_SCREEN: return TUI_KEY_PAGEDOWN;
        default:                 return TUI_KEY_UNKNOWN;
    }
}

/* IUnknown methods */
static HRESULT ANXAPI Screen_QueryInterface(
    ITuiScreen *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ITuiScreen)) {
        *ppvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI Screen_AddRef(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;
    return ++screen->RefCount;
}

static UINTN ANXAPI Screen_Release(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;
    UINTN refCount = --screen->RefCount;

    if (refCount == 0) {
        if (screen->Initialized) {
            /* Delete virtual display and pasteboard */
            smg$delete_virtual_display(&screen->DisplayId);
            smg$delete_pasteboard(&screen->PasteBoardId);
            smg$delete_virtual_keyboard(&screen->KeyboardId);
        }
        free(screen);
    }

    return refCount;
}

/* ITuiScreen methods */
static HRESULT ANXAPI Screen_Initialize(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;
    unsigned long status;
    unsigned long rows, cols;

    if (screen->Initialized) {
        return S_OK;
    }

    /* Create pasteboard (physical screen) */
    status = smg$create_pasteboard(&screen->PasteBoardId, 0, 0, 0, 0);
    if (!(status & 1)) {
        return E_FAIL;
    }

    /* Get screen dimensions */
    rows = 24;
    cols = 80;

    /* Create virtual display */
    status = smg$create_virtual_display(&rows, &cols, &screen->DisplayId, 0, 0, 0);
    if (!(status & 1)) {
        smg$delete_pasteboard(&screen->PasteBoardId);
        return E_FAIL;
    }

    /* Create virtual keyboard */
    status = smg$create_virtual_keyboard(&screen->KeyboardId, 0, 0, 0, 0);
    if (!(status & 1)) {
        smg$delete_virtual_display(&screen->DisplayId);
        smg$delete_pasteboard(&screen->PasteBoardId);
        return E_FAIL;
    }

    /* Paste virtual display onto pasteboard */
    status = smg$paste_virtual_display(&screen->DisplayId, &screen->PasteBoardId, 0, 0);
    if (!(status & 1)) {
        smg$delete_virtual_keyboard(&screen->KeyboardId);
        smg$delete_virtual_display(&screen->DisplayId);
        smg$delete_pasteboard(&screen->PasteBoardId);
        return E_FAIL;
    }

    screen->Initialized = TRUE;
    return S_OK;
}

static HRESULT ANXAPI Screen_GetDimensions(
    ITuiScreen *This,
    UINT32 *Width,
    UINT32 *Height
)
{
    TuiScreen *screen = (TuiScreen *)This;
    unsigned long height, width;
    unsigned long status;

    if (Width == NULL || Height == NULL) {
        return E_POINTER;
    }

    if (!screen->Initialized) {
        return E_FAIL;
    }

    /* Get virtual display dimensions */
    status = smg$get_display_attr(&screen->DisplayId, &height, &width, 0, 0, 0);
    if (!(status & 1)) {
        return E_FAIL;
    }

    *Width = (UINT32)width;
    *Height = (UINT32)height;

    return S_OK;
}

static HRESULT ANXAPI Screen_Clear(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;
    unsigned long status;

    if (!screen->Initialized) {
        return E_FAIL;
    }

    /* Erase display */
    status = smg$erase_display(&screen->DisplayId, 0, 0, 0, 0);
    if (!(status & 1)) {
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT ANXAPI Screen_WriteText(
    ITuiScreen *This,
    INT32 X,
    INT32 Y,
    CONST CHAR8 *Text,
    TUI_COLOR Fg,
    TUI_COLOR Bg
)
{
    TuiScreen *screen = (TuiScreen *)This;
    struct dsc$descriptor_s textDesc;
    unsigned long status;
    unsigned long attr;
    long row, col;

    if (!screen->Initialized || Text == NULL) {
        return E_POINTER;
    }

    /* Create descriptor for text */
    textDesc.dsc$w_length = strlen(Text);
    textDesc.dsc$b_dtype = DSC$K_DTYPE_T;
    textDesc.dsc$b_class = DSC$K_CLASS_S;
    textDesc.dsc$a_pointer = (char *)Text;

    attr = MapColor(Fg, Bg);
    row = Y + 1;  /* VMS uses 1-based coordinates */
    col = X + 1;

    /* Write text to virtual display */
    status = smg$put_chars(&screen->DisplayId, &textDesc, &row, &col, 0, &attr, 0, 0);
    if (!(status & 1)) {
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT ANXAPI Screen_DrawBox(
    ITuiScreen *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    TUI_COLOR Fg,
    TUI_COLOR Bg
)
{
    TuiScreen *screen = (TuiScreen *)This;
    unsigned long status;
    long startRow, startCol, endRow, endCol;

    if (!screen->Initialized) {
        return E_FAIL;
    }

    /* VMS uses 1-based coordinates */
    startRow = Y + 1;
    startCol = X + 1;
    endRow = Y + Height;
    endCol = X + Width;

    /* Draw border using SMG$ routine */
    status = smg$draw_rectangle(&screen->DisplayId, &startRow, &startCol,
                                &endRow, &endCol, 0, 0);
    if (!(status & 1)) {
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT ANXAPI Screen_GetKey(ITuiScreen *This, TUI_KEY *Key)
{
    TuiScreen *screen = (TuiScreen *)This;
    unsigned long status;
    unsigned short keycode;
    struct dsc$descriptor_s termDesc;
    char termBuffer[256];

    if (!screen->Initialized || Key == NULL) {
        return E_POINTER;
    }

    termDesc.dsc$w_length = sizeof(termBuffer);
    termDesc.dsc$b_dtype = DSC$K_DTYPE_T;
    termDesc.dsc$b_class = DSC$K_CLASS_S;
    termDesc.dsc$a_pointer = termBuffer;

    /* Read key from keyboard */
    status = smg$read_keystroke(&screen->KeyboardId, &keycode, 0, 0, &screen->DisplayId, 0, 0);
    if (!(status & 1)) {
        return E_FAIL;
    }

    /* Map key */
    if (keycode < 256) {
        *Key = MapVMSKey(keycode, (char)keycode);
    } else {
        *Key = MapVMSKey(keycode, 0);
    }

    return S_OK;
}

static HRESULT ANXAPI Screen_Refresh(ITuiScreen *This)
{
    /* SMG automatically updates pasteboard */
    (void)This;
    return S_OK;
}

/* Vtable */
static CONST ITuiScreen_Vtbl ScreenVtbl = {
    Screen_QueryInterface,
    Screen_AddRef,
    Screen_Release,
    Screen_Initialize,
    Screen_GetDimensions,
    Screen_Clear,
    Screen_WriteText,
    Screen_DrawBox,
    Screen_GetKey,
    Screen_Refresh
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateScreen(OUT ITuiScreen **Screen)
{
    TuiScreen *screen;

    if (Screen == NULL) {
        return E_POINTER;
    }

    screen = (TuiScreen *)calloc(1, sizeof(TuiScreen));
    if (screen == NULL) {
        *Screen = NULL;
        return E_OUTOFMEMORY;
    }

    screen->Interface.Vtbl = &ScreenVtbl;
    screen->RefCount = 1;
    screen->Initialized = FALSE;

    *Screen = &screen->Interface;
    return S_OK;
}

#endif /* __VMS */
