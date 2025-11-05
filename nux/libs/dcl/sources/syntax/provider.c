/**
 * @file provider.c
 * @brief Syntax provider implementation for client/server syntax registration
 */

#include <dcl/internal.h>

/**
 * Forward declarations
 */
static HRESULT STDMETHODCALLTYPE SyntaxProvider_QueryInterface(
    IDclSyntaxProvider *This,
    IID *riid,
    VOID **ppvObject);

static ULONG STDMETHODCALLTYPE SyntaxProvider_AddRef(IDclSyntaxProvider *This);

static ULONG STDMETHODCALLTYPE SyntaxProvider_Release(IDclSyntaxProvider *This);

static HRESULT STDMETHODCALLTYPE SyntaxProvider_RegisterSyntax(
    IDclSyntaxProvider *This,
    const CHAR8 *Name,
    VOID *SyntaxData);

static HRESULT STDMETHODCALLTYPE SyntaxProvider_UnregisterSyntax(
    IDclSyntaxProvider *This,
    const CHAR8 *Name);

static HRESULT STDMETHODCALLTYPE SyntaxProvider_GetSyntax(
    IDclSyntaxProvider *This,
    const CHAR8 *Name,
    VOID **SyntaxData);

static HRESULT STDMETHODCALLTYPE SyntaxProvider_EnumerateSyntaxes(
    IDclSyntaxProvider *This,
    UINTN *Count,
    CHAR8 ***Names);

/**
 * VTable for syntax provider
 */
static IDclSyntaxProviderVtbl gSyntaxProviderVtbl = {
    SyntaxProvider_QueryInterface,
    SyntaxProvider_AddRef,
    SyntaxProvider_Release,
    SyntaxProvider_RegisterSyntax,
    SyntaxProvider_UnregisterSyntax,
    SyntaxProvider_GetSyntax,
    SyntaxProvider_EnumerateSyntaxes
};

/**
 * QueryInterface implementation
 */
static HRESULT STDMETHODCALLTYPE SyntaxProvider_QueryInterface(
    IDclSyntaxProvider *This,
    IID *riid,
    VOID **ppvObject)
{
    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDclSyntaxProvider)) {
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
static ULONG STDMETHODCALLTYPE SyntaxProvider_AddRef(IDclSyntaxProvider *This)
{
    DCL_SYNTAX_PROVIDER *pProvider = (DCL_SYNTAX_PROVIDER *)This;
    return AtomicIncrement(&pProvider->RefCount);
}

/**
 * Release implementation
 */
static ULONG STDMETHODCALLTYPE SyntaxProvider_Release(IDclSyntaxProvider *This)
{
    DCL_SYNTAX_PROVIDER *pProvider = (DCL_SYNTAX_PROVIDER *)This;
    ULONG refCount = AtomicDecrement(&pProvider->RefCount);

    if (refCount == 0) {
        /* Free the provider - would use kernel allocator in full implementation */
        /* For now, this is a static/global object */
    }

    return refCount;
}

/**
 * RegisterSyntax implementation
 */
static HRESULT STDMETHODCALLTYPE SyntaxProvider_RegisterSyntax(
    IDclSyntaxProvider *This,
    const CHAR8 *Name,
    VOID *SyntaxData)
{
    DCL_SYNTAX_PROVIDER *pProvider = (DCL_SYNTAX_PROVIDER *)This;
    UINTN i;

    if (Name == NULL) {
        return E_POINTER;
    }

    /* Check if syntax already registered */
    for (i = 0; i < DCL_MAX_SYNTAX_PROVIDERS; i++) {
        if (pProvider->Entries[i].InUse) {
            if (DclStrNCmp(pProvider->Entries[i].Name, Name, 64) == 0) {
                /* Update existing syntax */
                pProvider->Entries[i].SyntaxData = SyntaxData;
                return S_OK;
            }
        }
    }

    /* Find free slot */
    for (i = 0; i < DCL_MAX_SYNTAX_PROVIDERS; i++) {
        if (!pProvider->Entries[i].InUse) {
            DclStrCpy(pProvider->Entries[i].Name, Name, 64);
            pProvider->Entries[i].SyntaxData = SyntaxData;
            pProvider->Entries[i].InUse = TRUE;
            pProvider->EntryCount++;
            return S_OK;
        }
    }

    return E_OUTOFMEMORY;
}

/**
 * UnregisterSyntax implementation
 */
static HRESULT STDMETHODCALLTYPE SyntaxProvider_UnregisterSyntax(
    IDclSyntaxProvider *This,
    const CHAR8 *Name)
{
    DCL_SYNTAX_PROVIDER *pProvider = (DCL_SYNTAX_PROVIDER *)This;
    UINTN i;

    if (Name == NULL) {
        return E_POINTER;
    }

    for (i = 0; i < DCL_MAX_SYNTAX_PROVIDERS; i++) {
        if (pProvider->Entries[i].InUse) {
            if (DclStrNCmp(pProvider->Entries[i].Name, Name, 64) == 0) {
                pProvider->Entries[i].InUse = FALSE;
                pProvider->Entries[i].SyntaxData = NULL;
                pProvider->EntryCount--;
                return S_OK;
            }
        }
    }

    return E_NOTFOUND;
}

/**
 * GetSyntax implementation
 */
static HRESULT STDMETHODCALLTYPE SyntaxProvider_GetSyntax(
    IDclSyntaxProvider *This,
    const CHAR8 *Name,
    VOID **SyntaxData)
{
    DCL_SYNTAX_PROVIDER *pProvider = (DCL_SYNTAX_PROVIDER *)This;
    UINTN i;

    if (Name == NULL || SyntaxData == NULL) {
        return E_POINTER;
    }

    for (i = 0; i < DCL_MAX_SYNTAX_PROVIDERS; i++) {
        if (pProvider->Entries[i].InUse) {
            if (DclStrNCmp(pProvider->Entries[i].Name, Name, 64) == 0) {
                *SyntaxData = pProvider->Entries[i].SyntaxData;
                return S_OK;
            }
        }
    }

    *SyntaxData = NULL;
    return E_NOTFOUND;
}

/**
 * EnumerateSyntaxes implementation
 */
static HRESULT STDMETHODCALLTYPE SyntaxProvider_EnumerateSyntaxes(
    IDclSyntaxProvider *This,
    UINTN *Count,
    CHAR8 ***Names)
{
    DCL_SYNTAX_PROVIDER *pProvider = (DCL_SYNTAX_PROVIDER *)This;

    if (Count == NULL) {
        return E_POINTER;
    }

    *Count = pProvider->EntryCount;

    /* In a full implementation, would allocate and return array of names */
    if (Names != NULL) {
        *Names = NULL;
    }

    return S_OK;
}

/**
 * Factory function to create syntax provider
 */
HRESULT DclCreateSyntaxProvider(IDclSyntaxProvider **Provider)
{
    static DCL_SYNTAX_PROVIDER gSyntaxProvider;
    UINTN i;

    if (Provider == NULL) {
        return E_POINTER;
    }

    /* Initialize provider on first use */
    if (gSyntaxProvider.Interface.lpVtbl == NULL) {
        gSyntaxProvider.Interface.lpVtbl = &gSyntaxProviderVtbl;
        gSyntaxProvider.RefCount = 1;
        gSyntaxProvider.EntryCount = 0;

        /* Clear all entries */
        for (i = 0; i < DCL_MAX_SYNTAX_PROVIDERS; i++) {
            gSyntaxProvider.Entries[i].InUse = FALSE;
            gSyntaxProvider.Entries[i].SyntaxData = NULL;
        }
    }

    *Provider = &gSyntaxProvider.Interface;
    gSyntaxProvider.Interface.lpVtbl->AddRef(*Provider);

    return S_OK;
}
