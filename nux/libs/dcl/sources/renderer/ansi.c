/**
 * @file ansi.c
 * @brief ANSI escape code renderer implementation
 */

#include <dcl/internal.h>
#include <nux/nux.h>

/**
 * Forward declarations
 */
static HRESULT STDMETHODCALLTYPE AnsiRenderer_QueryInterface(
    IDclRenderer *This,
    IID *riid,
    VOID **ppvObject);

static ULONG STDMETHODCALLTYPE AnsiRenderer_AddRef(IDclRenderer *This);

static ULONG STDMETHODCALLTYPE AnsiRenderer_Release(IDclRenderer *This);

static HRESULT STDMETHODCALLTYPE AnsiRenderer_Initialize(IDclRenderer *This);

static HRESULT STDMETHODCALLTYPE AnsiRenderer_Shutdown(IDclRenderer *This);

static HRESULT STDMETHODCALLTYPE AnsiRenderer_SetColor(
    IDclRenderer *This,
    DCL_COLOR Foreground,
    DCL_COLOR Background);

static HRESULT STDMETHODCALLTYPE AnsiRenderer_ResetColor(IDclRenderer *This);

static HRESULT STDMETHODCALLTYPE AnsiRenderer_WriteText(
    IDclRenderer *This,
    const CHAR8 *Text,
    UINTN Length);

static HRESULT STDMETHODCALLTYPE AnsiRenderer_WriteToken(
    IDclRenderer *This,
    const DCL_SYNTAX_TOKEN *Token);

static HRESULT STDMETHODCALLTYPE AnsiRenderer_NewLine(IDclRenderer *This);

static HRESULT STDMETHODCALLTYPE AnsiRenderer_ClearScreen(IDclRenderer *This);

static HRESULT STDMETHODCALLTYPE AnsiRenderer_MoveCursor(
    IDclRenderer *This,
    UINTN X,
    UINTN Y);

/**
 * VTable for ANSI renderer
 */
static IDclRendererVtbl gAnsiRendererVtbl = {
    AnsiRenderer_QueryInterface,
    AnsiRenderer_AddRef,
    AnsiRenderer_Release,
    AnsiRenderer_Initialize,
    AnsiRenderer_Shutdown,
    AnsiRenderer_SetColor,
    AnsiRenderer_ResetColor,
    AnsiRenderer_WriteText,
    AnsiRenderer_WriteToken,
    AnsiRenderer_NewLine,
    AnsiRenderer_ClearScreen,
    AnsiRenderer_MoveCursor
};

/**
 * Helper function to write a character via HAL or NUX
 */
static VOID AnsiRenderer_PutChar(CHAR8 c)
{
    /* Use HAL CPU interface to output character */
    if (gpHal != NULL) {
        IHalCpu *pCpu = NULL;
        HRESULT hr = gpHal->lpVtbl->GetCpuInterface(gpHal, &pCpu);
        if (SUCCEEDED(hr) && pCpu != NULL) {
            pCpu->lpVtbl->PutChar(pCpu, c);
            pCpu->lpVtbl->Release((IUnknown *)pCpu);
        }
    }
}

/**
 * Helper function to write a string
 */
static VOID AnsiRenderer_PutString(const CHAR8 *s)
{
    if (s == NULL) return;
    while (*s) {
        AnsiRenderer_PutChar(*s++);
    }
}

/**
 * Helper function to write an ANSI escape sequence
 */
static VOID AnsiRenderer_WriteEscape(const CHAR8 *seq)
{
    AnsiRenderer_PutChar('\x1B');
    AnsiRenderer_PutChar('[');
    AnsiRenderer_PutString(seq);
}

/**
 * Convert DCL_COLOR to ANSI color code
 */
static UINT32 AnsiRenderer_ColorToAnsi(DCL_COLOR Color, BOOLEAN IsBackground)
{
    UINT32 base = IsBackground ? 40 : 30;

    if (Color >= DCL_COLOR_BRIGHT_BLACK && Color <= DCL_COLOR_BRIGHT_WHITE) {
        /* Bright colors */
        return base + 60 + (Color - DCL_COLOR_BRIGHT_BLACK);
    } else if (Color == DCL_COLOR_DEFAULT) {
        return base + 9;
    } else if (Color <= DCL_COLOR_WHITE) {
        return base + Color;
    }

    return base + 9; /* Default */
}

/**
 * Write a decimal number as string
 */
static VOID AnsiRenderer_PutDecimal(UINT32 num)
{
    CHAR8 buf[16];
    INT32 i = 0;

    if (num == 0) {
        AnsiRenderer_PutChar('0');
        return;
    }

    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    while (i > 0) {
        AnsiRenderer_PutChar(buf[--i]);
    }
}

/**
 * QueryInterface implementation
 */
static HRESULT STDMETHODCALLTYPE AnsiRenderer_QueryInterface(
    IDclRenderer *This,
    IID *riid,
    VOID **ppvObject)
{
    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDclRenderer)) {
        *ppvObject = This;
        This->lpVtbl->AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

/**
 * AddRef implementation
 */
static ULONG STDMETHODCALLTYPE AnsiRenderer_AddRef(IDclRenderer *This)
{
    DCL_ANSI_RENDERER *pRenderer = (DCL_ANSI_RENDERER *)This;
    return AtomicIncrement(&pRenderer->RefCount);
}

/**
 * Release implementation
 */
static ULONG STDMETHODCALLTYPE AnsiRenderer_Release(IDclRenderer *This)
{
    DCL_ANSI_RENDERER *pRenderer = (DCL_ANSI_RENDERER *)This;
    ULONG refCount = AtomicDecrement(&pRenderer->RefCount);

    if (refCount == 0) {
        /* Free the renderer - would use kernel allocator in full implementation */
        /* For now, this is a static/global object */
    }

    return refCount;
}

/**
 * Initialize implementation
 */
static HRESULT STDMETHODCALLTYPE AnsiRenderer_Initialize(IDclRenderer *This)
{
    DCL_ANSI_RENDERER *pRenderer = (DCL_ANSI_RENDERER *)This;

    pRenderer->CurrentForeground = DCL_COLOR_DEFAULT;
    pRenderer->CurrentBackground = DCL_COLOR_DEFAULT;

    /* Clear screen and reset colors */
    AnsiRenderer_ClearScreen(This);
    AnsiRenderer_ResetColor(This);

    return S_OK;
}

/**
 * Shutdown implementation
 */
static HRESULT STDMETHODCALLTYPE AnsiRenderer_Shutdown(IDclRenderer *This)
{
    /* Reset colors */
    AnsiRenderer_ResetColor(This);
    return S_OK;
}

/**
 * SetColor implementation
 */
static HRESULT STDMETHODCALLTYPE AnsiRenderer_SetColor(
    IDclRenderer *This,
    DCL_COLOR Foreground,
    DCL_COLOR Background)
{
    DCL_ANSI_RENDERER *pRenderer = (DCL_ANSI_RENDERER *)This;
    UINT32 fgCode, bgCode;

    pRenderer->CurrentForeground = Foreground;
    pRenderer->CurrentBackground = Background;

    fgCode = AnsiRenderer_ColorToAnsi(Foreground, FALSE);
    bgCode = AnsiRenderer_ColorToAnsi(Background, TRUE);

    /* Write ANSI escape sequence: ESC[<fg>;<bg>m */
    AnsiRenderer_PutChar('\x1B');
    AnsiRenderer_PutChar('[');
    AnsiRenderer_PutDecimal(fgCode);
    AnsiRenderer_PutChar(';');
    AnsiRenderer_PutDecimal(bgCode);
    AnsiRenderer_PutChar('m');

    return S_OK;
}

/**
 * ResetColor implementation
 */
static HRESULT STDMETHODCALLTYPE AnsiRenderer_ResetColor(IDclRenderer *This)
{
    DCL_ANSI_RENDERER *pRenderer = (DCL_ANSI_RENDERER *)This;

    pRenderer->CurrentForeground = DCL_COLOR_DEFAULT;
    pRenderer->CurrentBackground = DCL_COLOR_DEFAULT;

    /* Write ANSI reset sequence: ESC[0m */
    AnsiRenderer_WriteEscape("0m");

    return S_OK;
}

/**
 * WriteText implementation
 */
static HRESULT STDMETHODCALLTYPE AnsiRenderer_WriteText(
    IDclRenderer *This,
    const CHAR8 *Text,
    UINTN Length)
{
    UINTN i;

    if (Text == NULL) {
        return E_POINTER;
    }

    for (i = 0; i < Length; i++) {
        AnsiRenderer_PutChar(Text[i]);
    }

    return S_OK;
}

/**
 * WriteToken implementation
 */
static HRESULT STDMETHODCALLTYPE AnsiRenderer_WriteToken(
    IDclRenderer *This,
    const DCL_SYNTAX_TOKEN *Token)
{
    if (Token == NULL) {
        return E_POINTER;
    }

    /* Set color based on token type */
    AnsiRenderer_SetColor(This, Token->ForegroundColor, Token->BackgroundColor);

    /* Write token text */
    AnsiRenderer_WriteText(This, Token->Text, Token->Length);

    /* Reset color */
    AnsiRenderer_ResetColor(This);

    return S_OK;
}

/**
 * NewLine implementation
 */
static HRESULT STDMETHODCALLTYPE AnsiRenderer_NewLine(IDclRenderer *This)
{
    AnsiRenderer_PutChar('\r');
    AnsiRenderer_PutChar('\n');
    return S_OK;
}

/**
 * ClearScreen implementation
 */
static HRESULT STDMETHODCALLTYPE AnsiRenderer_ClearScreen(IDclRenderer *This)
{
    /* ESC[2J - Clear entire screen */
    AnsiRenderer_WriteEscape("2J");
    /* ESC[H - Move cursor to home position */
    AnsiRenderer_WriteEscape("H");
    return S_OK;
}

/**
 * MoveCursor implementation
 */
static HRESULT STDMETHODCALLTYPE AnsiRenderer_MoveCursor(
    IDclRenderer *This,
    UINTN X,
    UINTN Y)
{
    /* ESC[<Y>;<X>H - Move cursor to position */
    AnsiRenderer_PutChar('\x1B');
    AnsiRenderer_PutChar('[');
    AnsiRenderer_PutDecimal((UINT32)Y);
    AnsiRenderer_PutChar(';');
    AnsiRenderer_PutDecimal((UINT32)X);
    AnsiRenderer_PutChar('H');
    return S_OK;
}

/**
 * Factory function to create ANSI renderer
 */
HRESULT DclCreateAnsiRenderer(IDclRenderer **Renderer)
{
    static DCL_ANSI_RENDERER gAnsiRenderer;

    if (Renderer == NULL) {
        return E_POINTER;
    }

    /* Initialize renderer on first use */
    if (gAnsiRenderer.Interface.lpVtbl == NULL) {
        gAnsiRenderer.Interface.lpVtbl = &gAnsiRendererVtbl;
        gAnsiRenderer.RefCount = 1;
        gAnsiRenderer.CurrentForeground = DCL_COLOR_DEFAULT;
        gAnsiRenderer.CurrentBackground = DCL_COLOR_DEFAULT;
    }

    *Renderer = &gAnsiRenderer.Interface;
    gAnsiRenderer.Interface.lpVtbl->AddRef(*Renderer);

    return S_OK;
}
