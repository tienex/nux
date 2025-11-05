/**
 * @file shell.c
 * @brief Main DCL shell implementation
 */

#include <dcl/internal.h>
#include <nux/nux.h>

/**
 * Global DCL shell instance
 */
IDcl *gpDcl = NULL;

/**
 * Forward declarations
 */
static HRESULT STDMETHODCALLTYPE DclShell_QueryInterface(
    IDcl *This,
    IID *riid,
    VOID **ppvObject);

static ULONG STDMETHODCALLTYPE DclShell_AddRef(IDcl *This);

static ULONG STDMETHODCALLTYPE DclShell_Release(IDcl *This);

static HRESULT STDMETHODCALLTYPE DclShell_Initialize(IDcl *This);

static HRESULT STDMETHODCALLTYPE DclShell_Shutdown(IDcl *This);

static HRESULT STDMETHODCALLTYPE DclShell_Run(IDcl *This);

static HRESULT STDMETHODCALLTYPE DclShell_GetRendererInterface(
    IDcl *This,
    IDclRenderer **Renderer);

static HRESULT STDMETHODCALLTYPE DclShell_GetSyntaxProviderInterface(
    IDcl *This,
    IDclSyntaxProvider **Provider);

static HRESULT STDMETHODCALLTYPE DclShell_GetParserInterface(
    IDcl *This,
    IDclParser **Parser);

static HRESULT STDMETHODCALLTYPE DclShell_GetMessageBusInterface(
    IDcl *This,
    IDclMessageBus **MessageBus);

static HRESULT STDMETHODCALLTYPE DclShell_ExecuteCommand(
    IDcl *This,
    const CHAR8 *Command);

/**
 * VTable for DCL shell
 */
static IDclVtbl gDclShellVtbl = {
    DclShell_QueryInterface,
    DclShell_AddRef,
    DclShell_Release,
    DclShell_Initialize,
    DclShell_Shutdown,
    DclShell_Run,
    DclShell_GetRendererInterface,
    DclShell_GetSyntaxProviderInterface,
    DclShell_GetParserInterface,
    DclShell_GetMessageBusInterface,
    DclShell_ExecuteCommand
};

/**
 * QueryInterface implementation
 */
static HRESULT STDMETHODCALLTYPE DclShell_QueryInterface(
    IDcl *This,
    IID *riid,
    VOID **ppvObject)
{
    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDcl)) {
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
static ULONG STDMETHODCALLTYPE DclShell_AddRef(IDcl *This)
{
    DCL_SHELL *pShell = (DCL_SHELL *)This;
    return AtomicIncrement(&pShell->RefCount);
}

/**
 * Release implementation
 */
static ULONG STDMETHODCALLTYPE DclShell_Release(IDcl *This)
{
    DCL_SHELL *pShell = (DCL_SHELL *)This;
    ULONG refCount = AtomicDecrement(&pShell->RefCount);

    if (refCount == 0) {
        /* Free the shell - would use kernel allocator in full implementation */
        /* For now, this is a static/global object */
    }

    return refCount;
}

/**
 * Initialize implementation
 */
static HRESULT STDMETHODCALLTYPE DclShell_Initialize(IDcl *This)
{
    DCL_SHELL *pShell = (DCL_SHELL *)This;
    HRESULT hr;

    /* Create renderer */
    hr = DclCreateAnsiRenderer((IDclRenderer **)&pShell->Renderer);
    if (FAILED(hr)) {
        return hr;
    }

    /* Create syntax provider */
    hr = DclCreateSyntaxProvider((IDclSyntaxProvider **)&pShell->SyntaxProvider);
    if (FAILED(hr)) {
        return hr;
    }

    /* Create parser */
    hr = DclCreateParser((IDclParser **)&pShell->Parser);
    if (FAILED(hr)) {
        return hr;
    }

    /* Create message bus */
    hr = DclCreateMessageBus((IDclMessageBus **)&pShell->MessageBus);
    if (FAILED(hr)) {
        return hr;
    }

    /* Link parser to syntax provider */
    pShell->Parser->Interface.lpVtbl->SetSyntaxProvider(
        &pShell->Parser->Interface,
        &pShell->SyntaxProvider->Interface);

    /* Initialize renderer */
    pShell->Renderer->Interface.lpVtbl->Initialize(&pShell->Renderer->Interface);

    /* Initialize message bus */
    pShell->MessageBus->Interface.lpVtbl->Initialize(&pShell->MessageBus->Interface);

    /* Initialize command buffer */
    pShell->CommandLength = 0;
    pShell->Running = FALSE;

    return S_OK;
}

/**
 * Shutdown implementation
 */
static HRESULT STDMETHODCALLTYPE DclShell_Shutdown(IDcl *This)
{
    DCL_SHELL *pShell = (DCL_SHELL *)This;

    pShell->Running = FALSE;

    /* Shutdown message bus */
    if (pShell->MessageBus != NULL) {
        pShell->MessageBus->Interface.lpVtbl->Shutdown(&pShell->MessageBus->Interface);
    }

    /* Shutdown renderer */
    if (pShell->Renderer != NULL) {
        pShell->Renderer->Interface.lpVtbl->Shutdown(&pShell->Renderer->Interface);
    }

    return S_OK;
}

/**
 * Helper to print a string using renderer
 */
static VOID DclShell_Print(DCL_SHELL *pShell, const CHAR8 *s)
{
    if (s == NULL || pShell->Renderer == NULL) {
        return;
    }

    pShell->Renderer->Interface.lpVtbl->WriteText(
        &pShell->Renderer->Interface,
        s,
        DclStrLen(s));
}

/**
 * Helper to print a newline
 */
static VOID DclShell_PrintNewLine(DCL_SHELL *pShell)
{
    if (pShell->Renderer == NULL) {
        return;
    }

    pShell->Renderer->Interface.lpVtbl->NewLine(&pShell->Renderer->Interface);
}

/**
 * Display prompt
 */
static VOID DclShell_DisplayPrompt(DCL_SHELL *pShell)
{
    /* Set prompt color */
    pShell->Renderer->Interface.lpVtbl->SetColor(
        &pShell->Renderer->Interface,
        DCL_COLOR_BRIGHT_GREEN,
        DCL_COLOR_DEFAULT);

    DclShell_Print(pShell, "DCL> ");

    /* Reset color */
    pShell->Renderer->Interface.lpVtbl->ResetColor(&pShell->Renderer->Interface);
}

/**
 * Read a character from input (stub - would use HAL/console interface)
 */
static CHAR8 DclShell_ReadChar(DCL_SHELL *pShell)
{
    /* In a full implementation, would read from keyboard/serial console */
    /* For now, return 0 to indicate no input */
    return 0;
}

/**
 * Process command line with syntax coloring
 */
static VOID DclShell_ProcessLine(DCL_SHELL *pShell)
{
    DCL_SYNTAX_TOKEN *tokens = NULL;
    UINTN tokenCount = 0;
    HRESULT hr;
    UINTN i;

    /* Parse the line */
    hr = pShell->Parser->Interface.lpVtbl->ParseLine(
        &pShell->Parser->Interface,
        pShell->CommandBuffer,
        pShell->CommandLength,
        &tokens,
        &tokenCount);

    if (FAILED(hr)) {
        DclShell_Print(pShell, "Parse error");
        DclShell_PrintNewLine(pShell);
        return;
    }

    /* Display tokens with syntax coloring */
    DclShell_PrintNewLine(pShell);
    DclShell_Print(pShell, "Parsed (with colors): ");

    for (i = 0; i < tokenCount; i++) {
        pShell->Renderer->Interface.lpVtbl->WriteToken(
            &pShell->Renderer->Interface,
            &tokens[i]);

        /* Add space between tokens */
        if (i < tokenCount - 1) {
            DclShell_Print(pShell, " ");
        }
    }

    DclShell_PrintNewLine(pShell);

    /* Free tokens */
    pShell->Parser->Interface.lpVtbl->FreeTokens(
        &pShell->Parser->Interface,
        tokens,
        tokenCount);

    /* Execute command */
    DclShell_ExecuteCommand(&pShell->Interface, pShell->CommandBuffer);
}

/**
 * Run implementation - main shell loop
 */
static HRESULT STDMETHODCALLTYPE DclShell_Run(IDcl *This)
{
    DCL_SHELL *pShell = (DCL_SHELL *)This;
    CHAR8 c;

    pShell->Running = TRUE;

    /* Display welcome banner */
    pShell->Renderer->Interface.lpVtbl->SetColor(
        &pShell->Renderer->Interface,
        DCL_COLOR_BRIGHT_CYAN,
        DCL_COLOR_DEFAULT);

    DclShell_Print(pShell, "NUX DCL Shell v1.0");
    DclShell_PrintNewLine(pShell);
    DclShell_Print(pShell, "Digital Command Language with Syntax Coloring");
    DclShell_PrintNewLine(pShell);

    pShell->Renderer->Interface.lpVtbl->ResetColor(&pShell->Renderer->Interface);
    DclShell_PrintNewLine(pShell);

    /* Main shell loop */
    while (pShell->Running) {
        /* Display prompt */
        DclShell_DisplayPrompt(pShell);

        /* Read command line */
        pShell->CommandLength = 0;

        while (pShell->Running) {
            c = DclShell_ReadChar(pShell);

            if (c == 0) {
                /* No input available - for demo purposes, run a test command */
                /* In a real implementation, would wait for input */
                const CHAR8 *testCmd = "SHOW DIRECTORY test.txt";
                UINTN len = DclStrLen(testCmd);
                UINTN i;

                for (i = 0; i < len && i < DCL_MAX_COMMAND_LENGTH - 1; i++) {
                    pShell->CommandBuffer[i] = testCmd[i];
                }
                pShell->CommandBuffer[i] = '\0';
                pShell->CommandLength = len;

                /* Echo the command */
                DclShell_Print(pShell, testCmd);
                DclShell_PrintNewLine(pShell);

                /* Process it */
                DclShell_ProcessLine(pShell);

                /* Exit after demo */
                pShell->Running = FALSE;
                break;
            }

            /* Handle backspace */
            if (c == '\b' || c == 127) {
                if (pShell->CommandLength > 0) {
                    pShell->CommandLength--;
                    pShell->CommandBuffer[pShell->CommandLength] = '\0';
                }
                continue;
            }

            /* Handle enter */
            if (c == '\r' || c == '\n') {
                pShell->CommandBuffer[pShell->CommandLength] = '\0';
                DclShell_ProcessLine(pShell);

                /* Reset for next command */
                pShell->CommandLength = 0;
                break;
            }

            /* Add character to buffer */
            if (pShell->CommandLength < DCL_MAX_COMMAND_LENGTH - 1) {
                pShell->CommandBuffer[pShell->CommandLength++] = c;
            }
        }
    }

    return S_OK;
}

/**
 * GetRendererInterface implementation
 */
static HRESULT STDMETHODCALLTYPE DclShell_GetRendererInterface(
    IDcl *This,
    IDclRenderer **Renderer)
{
    DCL_SHELL *pShell = (DCL_SHELL *)This;

    if (Renderer == NULL) {
        return E_POINTER;
    }

    *Renderer = &pShell->Renderer->Interface;
    (*Renderer)->lpVtbl->AddRef(*Renderer);

    return S_OK;
}

/**
 * GetSyntaxProviderInterface implementation
 */
static HRESULT STDMETHODCALLTYPE DclShell_GetSyntaxProviderInterface(
    IDcl *This,
    IDclSyntaxProvider **Provider)
{
    DCL_SHELL *pShell = (DCL_SHELL *)This;

    if (Provider == NULL) {
        return E_POINTER;
    }

    *Provider = &pShell->SyntaxProvider->Interface;
    (*Provider)->lpVtbl->AddRef((IUnknown *)*Provider);

    return S_OK;
}

/**
 * GetParserInterface implementation
 */
static HRESULT STDMETHODCALLTYPE DclShell_GetParserInterface(
    IDcl *This,
    IDclParser **Parser)
{
    DCL_SHELL *pShell = (DCL_SHELL *)This;

    if (Parser == NULL) {
        return E_POINTER;
    }

    *Parser = &pShell->Parser->Interface;
    (*Parser)->lpVtbl->AddRef((IUnknown *)*Parser);

    return S_OK;
}

/**
 * GetMessageBusInterface implementation
 */
static HRESULT STDMETHODCALLTYPE DclShell_GetMessageBusInterface(
    IDcl *This,
    IDclMessageBus **MessageBus)
{
    DCL_SHELL *pShell = (DCL_SHELL *)This;

    if (MessageBus == NULL) {
        return E_POINTER;
    }

    *MessageBus = &pShell->MessageBus->Interface;
    (*MessageBus)->lpVtbl->AddRef((IUnknown *)*MessageBus);

    return S_OK;
}

/**
 * ExecuteCommand implementation
 */
static HRESULT STDMETHODCALLTYPE DclShell_ExecuteCommand(
    IDcl *This,
    const CHAR8 *Command)
{
    DCL_SHELL *pShell = (DCL_SHELL *)This;

    if (Command == NULL) {
        return E_POINTER;
    }

    /* Simple command execution - in a full implementation would dispatch to handlers */
    DclShell_Print(pShell, "Executing: ");
    DclShell_Print(pShell, Command);
    DclShell_PrintNewLine(pShell);

    /* Check for EXIT command */
    if (DclStrNCmp(Command, "EXIT", 4) == 0 ||
        DclStrNCmp(Command, "LOGOUT", 6) == 0) {
        pShell->Running = FALSE;
    }

    return S_OK;
}

/**
 * Factory function to create DCL shell
 */
HRESULT DclCreateShell(IDcl **Shell)
{
    static DCL_SHELL gDclShell;

    if (Shell == NULL) {
        return E_POINTER;
    }

    /* Initialize shell on first use */
    if (gDclShell.Interface.lpVtbl == NULL) {
        gDclShell.Interface.lpVtbl = &gDclShellVtbl;
        gDclShell.RefCount = 1;
        gDclShell.Renderer = NULL;
        gDclShell.SyntaxProvider = NULL;
        gDclShell.Parser = NULL;
        gDclShell.MessageBus = NULL;
        gDclShell.Running = FALSE;
        gDclShell.CommandLength = 0;
    }

    *Shell = &gDclShell.Interface;
    gDclShell.Interface.lpVtbl->AddRef(*Shell);

    return S_OK;
}

/**
 * Legacy function wrappers
 */

HRESULT dcl_initialize(VOID)
{
    HRESULT hr;

    if (gpDcl == NULL) {
        hr = DclCreateShell(&gpDcl);
        if (FAILED(hr)) {
            return hr;
        }
    }

    return gpDcl->lpVtbl->Initialize(gpDcl);
}

HRESULT dcl_shutdown(VOID)
{
    if (gpDcl == NULL) {
        return E_NOT_SET;
    }

    return gpDcl->lpVtbl->Shutdown(gpDcl);
}

HRESULT dcl_run(VOID)
{
    if (gpDcl == NULL) {
        return E_NOT_SET;
    }

    return gpDcl->lpVtbl->Run(gpDcl);
}

HRESULT dcl_execute_command(const CHAR8 *Command)
{
    if (gpDcl == NULL) {
        return E_NOT_SET;
    }

    return gpDcl->lpVtbl->ExecuteCommand(gpDcl, Command);
}
