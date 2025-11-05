/**
 * @file messagebus.c
 * @brief Message bus implementation for client/server communication
 *
 * This provides a portable abstraction layer for inter-component communication,
 * allowing applications to register custom syntax providers remotely.
 */

#include <dcl/internal.h>

/**
 * Forward declarations
 */
static HRESULT STDMETHODCALLTYPE MessageBus_QueryInterface(
    IDclMessageBus *This,
    IID *riid,
    VOID **ppvObject);

static ULONG STDMETHODCALLTYPE MessageBus_AddRef(IDclMessageBus *This);

static ULONG STDMETHODCALLTYPE MessageBus_Release(IDclMessageBus *This);

static HRESULT STDMETHODCALLTYPE MessageBus_Initialize(IDclMessageBus *This);

static HRESULT STDMETHODCALLTYPE MessageBus_Shutdown(IDclMessageBus *This);

static HRESULT STDMETHODCALLTYPE MessageBus_SendMessage(
    IDclMessageBus *This,
    const DCL_MESSAGE *Message,
    DCL_MESSAGE **Response);

static HRESULT STDMETHODCALLTYPE MessageBus_RegisterHandler(
    IDclMessageBus *This,
    DCL_MESSAGE_TYPE MessageType,
    HRESULT (*Handler)(const DCL_MESSAGE *, DCL_MESSAGE **));

static HRESULT STDMETHODCALLTYPE MessageBus_UnregisterHandler(
    IDclMessageBus *This,
    DCL_MESSAGE_TYPE MessageType);

/**
 * VTable for message bus
 */
static IDclMessageBusVtbl gMessageBusVtbl = {
    MessageBus_QueryInterface,
    MessageBus_AddRef,
    MessageBus_Release,
    MessageBus_Initialize,
    MessageBus_Shutdown,
    MessageBus_SendMessage,
    MessageBus_RegisterHandler,
    MessageBus_UnregisterHandler
};

/**
 * QueryInterface implementation
 */
static HRESULT STDMETHODCALLTYPE MessageBus_QueryInterface(
    IDclMessageBus *This,
    IID *riid,
    VOID **ppvObject)
{
    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDclMessageBus)) {
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
static ULONG STDMETHODCALLTYPE MessageBus_AddRef(IDclMessageBus *This)
{
    DCL_MESSAGE_BUS *pBus = (DCL_MESSAGE_BUS *)This;
    return AtomicIncrement(&pBus->RefCount);
}

/**
 * Release implementation
 */
static ULONG STDMETHODCALLTYPE MessageBus_Release(IDclMessageBus *This)
{
    DCL_MESSAGE_BUS *pBus = (DCL_MESSAGE_BUS *)This;
    ULONG refCount = AtomicDecrement(&pBus->RefCount);

    if (refCount == 0) {
        /* Free the bus - would use kernel allocator in full implementation */
        /* For now, this is a static/global object */
    }

    return refCount;
}

/**
 * Initialize implementation
 */
static HRESULT STDMETHODCALLTYPE MessageBus_Initialize(IDclMessageBus *This)
{
    DCL_MESSAGE_BUS *pBus = (DCL_MESSAGE_BUS *)This;
    UINTN i;

    /* Clear all handlers */
    for (i = 0; i < DCL_MAX_MESSAGE_HANDLERS; i++) {
        pBus->Handlers[i].InUse = FALSE;
        pBus->Handlers[i].Handler = NULL;
    }

    pBus->HandlerCount = 0;

    return S_OK;
}

/**
 * Shutdown implementation
 */
static HRESULT STDMETHODCALLTYPE MessageBus_Shutdown(IDclMessageBus *This)
{
    DCL_MESSAGE_BUS *pBus = (DCL_MESSAGE_BUS *)This;
    UINTN i;

    /* Clear all handlers */
    for (i = 0; i < DCL_MAX_MESSAGE_HANDLERS; i++) {
        pBus->Handlers[i].InUse = FALSE;
        pBus->Handlers[i].Handler = NULL;
    }

    pBus->HandlerCount = 0;

    return S_OK;
}

/**
 * SendMessage implementation
 *
 * This is the core of the client/server architecture. Applications send
 * messages through this interface to register syntaxes, request parsing, etc.
 */
static HRESULT STDMETHODCALLTYPE MessageBus_SendMessage(
    IDclMessageBus *This,
    const DCL_MESSAGE *Message,
    DCL_MESSAGE **Response)
{
    DCL_MESSAGE_BUS *pBus = (DCL_MESSAGE_BUS *)This;
    UINTN i;
    HRESULT hr;

    if (Message == NULL) {
        return E_POINTER;
    }

    /* Find handler for this message type */
    for (i = 0; i < DCL_MAX_MESSAGE_HANDLERS; i++) {
        if (pBus->Handlers[i].InUse &&
            pBus->Handlers[i].MessageType == Message->MessageType) {

            /* Call the handler */
            hr = pBus->Handlers[i].Handler(Message, Response);
            return hr;
        }
    }

    /* No handler found */
    if (Response != NULL) {
        *Response = NULL;
    }

    return E_NOTIMPL;
}

/**
 * RegisterHandler implementation
 */
static HRESULT STDMETHODCALLTYPE MessageBus_RegisterHandler(
    IDclMessageBus *This,
    DCL_MESSAGE_TYPE MessageType,
    HRESULT (*Handler)(const DCL_MESSAGE *, DCL_MESSAGE **))
{
    DCL_MESSAGE_BUS *pBus = (DCL_MESSAGE_BUS *)This;
    UINTN i;

    if (Handler == NULL) {
        return E_POINTER;
    }

    /* Check if handler already registered for this type */
    for (i = 0; i < DCL_MAX_MESSAGE_HANDLERS; i++) {
        if (pBus->Handlers[i].InUse &&
            pBus->Handlers[i].MessageType == MessageType) {
            /* Update existing handler */
            pBus->Handlers[i].Handler = Handler;
            return S_OK;
        }
    }

    /* Find free slot */
    for (i = 0; i < DCL_MAX_MESSAGE_HANDLERS; i++) {
        if (!pBus->Handlers[i].InUse) {
            pBus->Handlers[i].MessageType = MessageType;
            pBus->Handlers[i].Handler = Handler;
            pBus->Handlers[i].InUse = TRUE;
            pBus->HandlerCount++;
            return S_OK;
        }
    }

    return E_OUTOFMEMORY;
}

/**
 * UnregisterHandler implementation
 */
static HRESULT STDMETHODCALLTYPE MessageBus_UnregisterHandler(
    IDclMessageBus *This,
    DCL_MESSAGE_TYPE MessageType)
{
    DCL_MESSAGE_BUS *pBus = (DCL_MESSAGE_BUS *)This;
    UINTN i;

    for (i = 0; i < DCL_MAX_MESSAGE_HANDLERS; i++) {
        if (pBus->Handlers[i].InUse &&
            pBus->Handlers[i].MessageType == MessageType) {
            pBus->Handlers[i].InUse = FALSE;
            pBus->Handlers[i].Handler = NULL;
            pBus->HandlerCount--;
            return S_OK;
        }
    }

    return E_NOTFOUND;
}

/**
 * Factory function to create message bus
 */
HRESULT DclCreateMessageBus(IDclMessageBus **MessageBus)
{
    static DCL_MESSAGE_BUS gMessageBus;
    UINTN i;

    if (MessageBus == NULL) {
        return E_POINTER;
    }

    /* Initialize bus on first use */
    if (gMessageBus.Interface.lpVtbl == NULL) {
        gMessageBus.Interface.lpVtbl = &gMessageBusVtbl;
        gMessageBus.RefCount = 1;
        gMessageBus.HandlerCount = 0;

        /* Clear all handlers */
        for (i = 0; i < DCL_MAX_MESSAGE_HANDLERS; i++) {
            gMessageBus.Handlers[i].InUse = FALSE;
            gMessageBus.Handlers[i].Handler = NULL;
        }
    }

    *MessageBus = &gMessageBus.Interface;
    gMessageBus.Interface.lpVtbl->AddRef(*MessageBus);

    return S_OK;
}
