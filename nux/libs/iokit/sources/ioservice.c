/**
 * @file ioservice.c
 * @brief IOService implementation - Base service for all drivers
 *
 * Provides the reference implementation of IIOService interface.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/iokit.h>
#include <iokit/ioservice.h>
#include <ananke/ntrtl.h>
#include <string.h>

/**
 * @brief Maximum number of properties per service
 */
#define IO_SERVICE_MAX_PROPERTIES   64

/**
 * @brief Maximum number of children per service
 */
#define IO_SERVICE_MAX_CHILDREN     32

/**
 * @brief Property table entry
 */
typedef struct _IO_SERVICE_PROPERTY_ENTRY {
    CHAR8   szKey[64];              /**< Property key */
    UINT32  uType;                  /**< Property type */
    UINT8   Data[256];              /**< Property data */
    UINTN   cbSize;                 /**< Data size */
    BOOLEAN bUsed;                  /**< Entry in use */
} IO_SERVICE_PROPERTY_ENTRY;

/**
 * @brief IOService implementation structure
 */
typedef struct _IO_SERVICE_IMPL {
    IIOService              Vtbl;                       /**< Virtual function table */
    ULONG                   uRefCount;                  /**< Reference count */
    CHAR8                   szName[64];                 /**< Service name */
    UINT32                  uState;                     /**< Service state */
    IIOService             *pProvider;                  /**< Provider (parent) */
    IIOService             *pChildren[IO_SERVICE_MAX_CHILDREN]; /**< Child services */
    UINT32                  uChildCount;                /**< Number of children */
    IO_SERVICE_PROPERTY_ENTRY Properties[IO_SERVICE_MAX_PROPERTIES]; /**< Property table */
    UINT32                  uPropertyCount;             /**< Number of properties */
} IO_SERVICE_IMPL;

// Forward declarations of vtable functions
static RETCODE STDMETHODCALLTYPE IOService_QueryInterface(
    IIOService *pThis,
    REFIID riid,
    void **ppvObject
);

static ULONG STDMETHODCALLTYPE IOService_AddRef(
    IIOService *pThis
);

static ULONG STDMETHODCALLTYPE IOService_Release(
    IIOService *pThis
);

static IO_RETURN STDMETHODCALLTYPE IOService_Probe(
    IIOService *pThis,
    IIOService *pProvider,
    UINT32 *puProbeScore
);

static IO_RETURN STDMETHODCALLTYPE IOService_Start(
    IIOService *pThis,
    IIOService *pProvider
);

static IO_RETURN STDMETHODCALLTYPE IOService_Stop(
    IIOService *pThis,
    IIOService *pProvider
);

static IO_RETURN STDMETHODCALLTYPE IOService_Terminate(
    IIOService *pThis,
    UINT32 uOptions
);

static IO_RETURN STDMETHODCALLTYPE IOService_GetProperty(
    IIOService *pThis,
    CONST CHAR8 *pszKey,
    VOID *pValue,
    UINTN *pcbSize,
    UINT32 *puType
);

static IO_RETURN STDMETHODCALLTYPE IOService_SetProperty(
    IIOService *pThis,
    CONST CHAR8 *pszKey,
    CONST VOID *pValue,
    UINTN cbSize,
    UINT32 uType
);

static IO_RETURN STDMETHODCALLTYPE IOService_GetParentService(
    IIOService *pThis,
    IIOService **ppParent
);

static IO_RETURN STDMETHODCALLTYPE IOService_GetChildService(
    IIOService *pThis,
    UINT32 uIndex,
    IIOService **ppChild
);

static IO_RETURN STDMETHODCALLTYPE IOService_GetServiceState(
    IIOService *pThis,
    UINT32 *puState
);

static IO_RETURN STDMETHODCALLTYPE IOService_GetServiceName(
    IIOService *pThis,
    CHAR8 *pszName,
    UINTN cbSize
);

static IO_RETURN STDMETHODCALLTYPE IOService_RegisterService(
    IIOService *pThis,
    UINT32 uOptions
);

/**
 * @brief IOService virtual function table
 */
static IIOServiceVtbl g_IOServiceVtbl = {
    .QueryInterface     = IOService_QueryInterface,
    .AddRef             = IOService_AddRef,
    .Release            = IOService_Release,
    .Probe              = IOService_Probe,
    .Start              = IOService_Start,
    .Stop               = IOService_Stop,
    .Terminate          = IOService_Terminate,
    .GetProperty        = IOService_GetProperty,
    .SetProperty        = IOService_SetProperty,
    .GetParentService   = IOService_GetParentService,
    .GetChildService    = IOService_GetChildService,
    .GetServiceState    = IOService_GetServiceState,
    .GetServiceName     = IOService_GetServiceName,
    .RegisterService    = IOService_RegisterService,
};

/**
 * @brief Create a new IOService instance
 *
 * @param pszName       Service name
 * @param ppService     Receives pointer to new service
 *
 * @retval IO_SUCCESS   Service created successfully
 * @retval IO_NO_MEMORY Insufficient memory
 */
IO_RETURN
IOServiceCreate(
    CONST CHAR8 *pszName,
    IIOService **ppService
    )
{
    IO_SERVICE_IMPL *pImpl;
    UINTN cbNameLen;

    if (pszName == NULL || ppService == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Allocate service instance
    pImpl = (IO_SERVICE_IMPL *)malloc(sizeof(IO_SERVICE_IMPL));
    if (pImpl == NULL) {
        return IO_NO_MEMORY;
    }

    // Initialize structure
    memset(pImpl, 0, sizeof(IO_SERVICE_IMPL));
    pImpl->Vtbl.lpVtbl = &g_IOServiceVtbl;
    pImpl->uRefCount = 1;
    pImpl->uState = IO_SERVICE_INACTIVE;

    // Copy service name
    cbNameLen = strlen(pszName);
    if (cbNameLen >= sizeof(pImpl->szName)) {
        cbNameLen = sizeof(pImpl->szName) - 1;
    }
    memcpy(pImpl->szName, pszName, cbNameLen);
    pImpl->szName[cbNameLen] = '\0';

    *ppService = (IIOService *)pImpl;
    return IO_SUCCESS;
}

/**
 * @brief Add a child service
 *
 * @param pThis     Service instance
 * @param pChild    Child service to add
 *
 * @retval IO_SUCCESS       Child added successfully
 * @retval IO_NO_RESOURCES  Too many children
 */
IO_RETURN
IOServiceAddChild(
    IIOService *pThis,
    IIOService *pChild
    )
{
    IO_SERVICE_IMPL *pImpl = (IO_SERVICE_IMPL *)pThis;

    if (pThis == NULL || pChild == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (pImpl->uChildCount >= IO_SERVICE_MAX_CHILDREN) {
        return IO_NO_RESOURCES;
    }

    pImpl->pChildren[pImpl->uChildCount] = pChild;
    pImpl->uChildCount++;

    IIOService_AddRef(pChild);
    return IO_SUCCESS;
}

// Implementation of vtable functions

static RETCODE STDMETHODCALLTYPE
IOService_QueryInterface(
    IIOService *pThis,
    REFIID riid,
    void **ppvObject
    )
{
    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IIOService)) {
        *ppvObject = pThis;
        IIOService_AddRef(pThis);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE
IOService_AddRef(
    IIOService *pThis
    )
{
    IO_SERVICE_IMPL *pImpl = (IO_SERVICE_IMPL *)pThis;
    return ++pImpl->uRefCount;
}

static ULONG STDMETHODCALLTYPE
IOService_Release(
    IIOService *pThis
    )
{
    IO_SERVICE_IMPL *pImpl = (IO_SERVICE_IMPL *)pThis;
    ULONG uRefCount;
    UINT32 i;

    uRefCount = --pImpl->uRefCount;
    if (uRefCount == 0) {
        // Release all children
        for (i = 0; i < pImpl->uChildCount; i++) {
            if (pImpl->pChildren[i] != NULL) {
                IIOService_Release(pImpl->pChildren[i]);
            }
        }

        // Release provider
        if (pImpl->pProvider != NULL) {
            IIOService_Release(pImpl->pProvider);
        }

        // Free memory
        free(pImpl);
    }

    return uRefCount;
}

static IO_RETURN STDMETHODCALLTYPE
IOService_Probe(
    IIOService *pThis,
    IIOService *pProvider,
    UINT32 *puProbeScore
    )
{
    // Default implementation: accept any provider
    if (puProbeScore != NULL) {
        *puProbeScore = 0;
    }
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
IOService_Start(
    IIOService *pThis,
    IIOService *pProvider
    )
{
    IO_SERVICE_IMPL *pImpl = (IO_SERVICE_IMPL *)pThis;

    // Store provider reference
    if (pProvider != NULL) {
        pImpl->pProvider = pProvider;
        IIOService_AddRef(pProvider);
    }

    // Update state
    pImpl->uState = IO_SERVICE_STARTED;
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
IOService_Stop(
    IIOService *pThis,
    IIOService *pProvider
    )
{
    IO_SERVICE_IMPL *pImpl = (IO_SERVICE_IMPL *)pThis;

    // Update state
    pImpl->uState = IO_SERVICE_REGISTERED;
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
IOService_Terminate(
    IIOService *pThis,
    UINT32 uOptions
    )
{
    IO_SERVICE_IMPL *pImpl = (IO_SERVICE_IMPL *)pThis;

    // Check for active children
    if (pImpl->uChildCount > 0) {
        return IO_BUSY;
    }

    // Update state
    pImpl->uState = IO_SERVICE_TERMINATED;
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
IOService_GetProperty(
    IIOService *pThis,
    CONST CHAR8 *pszKey,
    VOID *pValue,
    UINTN *pcbSize,
    UINT32 *puType
    )
{
    IO_SERVICE_IMPL *pImpl = (IO_SERVICE_IMPL *)pThis;
    UINT32 i;

    if (pszKey == NULL || pcbSize == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Search for property
    for (i = 0; i < pImpl->uPropertyCount; i++) {
        if (pImpl->Properties[i].bUsed &&
            strcmp(pImpl->Properties[i].szKey, pszKey) == 0) {

            // Found property
            if (puType != NULL) {
                *puType = pImpl->Properties[i].uType;
            }

            if (pValue != NULL && *pcbSize >= pImpl->Properties[i].cbSize) {
                memcpy(pValue, pImpl->Properties[i].Data, pImpl->Properties[i].cbSize);
            }

            *pcbSize = pImpl->Properties[i].cbSize;
            return IO_SUCCESS;
        }
    }

    return IO_NO_MATCH;
}

static IO_RETURN STDMETHODCALLTYPE
IOService_SetProperty(
    IIOService *pThis,
    CONST CHAR8 *pszKey,
    CONST VOID *pValue,
    UINTN cbSize,
    UINT32 uType
    )
{
    IO_SERVICE_IMPL *pImpl = (IO_SERVICE_IMPL *)pThis;
    UINT32 i;
    UINTN cbKeyLen;

    if (pszKey == NULL || pValue == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (cbSize > sizeof(pImpl->Properties[0].Data)) {
        return IO_BAD_ARGUMENT;
    }

    // Search for existing property
    for (i = 0; i < pImpl->uPropertyCount; i++) {
        if (pImpl->Properties[i].bUsed &&
            strcmp(pImpl->Properties[i].szKey, pszKey) == 0) {
            // Update existing property
            memcpy(pImpl->Properties[i].Data, pValue, cbSize);
            pImpl->Properties[i].cbSize = cbSize;
            pImpl->Properties[i].uType = uType;
            return IO_SUCCESS;
        }
    }

    // Find empty slot
    for (i = 0; i < IO_SERVICE_MAX_PROPERTIES; i++) {
        if (!pImpl->Properties[i].bUsed) {
            // Copy key
            cbKeyLen = strlen(pszKey);
            if (cbKeyLen >= sizeof(pImpl->Properties[i].szKey)) {
                cbKeyLen = sizeof(pImpl->Properties[i].szKey) - 1;
            }
            memcpy(pImpl->Properties[i].szKey, pszKey, cbKeyLen);
            pImpl->Properties[i].szKey[cbKeyLen] = '\0';

            // Copy value
            memcpy(pImpl->Properties[i].Data, pValue, cbSize);
            pImpl->Properties[i].cbSize = cbSize;
            pImpl->Properties[i].uType = uType;
            pImpl->Properties[i].bUsed = TRUE;

            if (i >= pImpl->uPropertyCount) {
                pImpl->uPropertyCount = i + 1;
            }

            return IO_SUCCESS;
        }
    }

    return IO_NO_MEMORY;
}

static IO_RETURN STDMETHODCALLTYPE
IOService_GetParentService(
    IIOService *pThis,
    IIOService **ppParent
    )
{
    IO_SERVICE_IMPL *pImpl = (IO_SERVICE_IMPL *)pThis;

    if (ppParent == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (pImpl->pProvider == NULL) {
        *ppParent = NULL;
        return IO_NO_DEVICE;
    }

    *ppParent = pImpl->pProvider;
    IIOService_AddRef(pImpl->pProvider);
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
IOService_GetChildService(
    IIOService *pThis,
    UINT32 uIndex,
    IIOService **ppChild
    )
{
    IO_SERVICE_IMPL *pImpl = (IO_SERVICE_IMPL *)pThis;

    if (ppChild == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (uIndex >= pImpl->uChildCount) {
        *ppChild = NULL;
        return IO_NO_DEVICE;
    }

    *ppChild = pImpl->pChildren[uIndex];
    IIOService_AddRef(pImpl->pChildren[uIndex]);
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
IOService_GetServiceState(
    IIOService *pThis,
    UINT32 *puState
    )
{
    IO_SERVICE_IMPL *pImpl = (IO_SERVICE_IMPL *)pThis;

    if (puState == NULL) {
        return IO_BAD_ARGUMENT;
    }

    *puState = pImpl->uState;
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
IOService_GetServiceName(
    IIOService *pThis,
    CHAR8 *pszName,
    UINTN cbSize
    )
{
    IO_SERVICE_IMPL *pImpl = (IO_SERVICE_IMPL *)pThis;
    UINTN cbCopySize;

    if (pszName == NULL || cbSize == 0) {
        return IO_BAD_ARGUMENT;
    }

    cbCopySize = strlen(pImpl->szName);
    if (cbCopySize >= cbSize) {
        cbCopySize = cbSize - 1;
    }

    memcpy(pszName, pImpl->szName, cbCopySize);
    pszName[cbCopySize] = '\0';
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
IOService_RegisterService(
    IIOService *pThis,
    UINT32 uOptions
    )
{
    IO_SERVICE_IMPL *pImpl = (IO_SERVICE_IMPL *)pThis;

    // Update state
    pImpl->uState = IO_SERVICE_REGISTERED;
    return IO_SUCCESS;
}
