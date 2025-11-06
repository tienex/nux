/**
 * @file ioregistry.c
 * @brief IORegistry implementation - Device tree management
 *
 * Provides the reference implementation of IIORegistry interface.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/iokit.h>
#include <iokit/ioservice.h>
#include <iokit/ioregistry.h>
#include <ananke/ntrtl.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief Maximum number of services in the registry
 */
#define IO_REGISTRY_MAX_SERVICES    256

/**
 * @brief Registry entry structure
 */
typedef struct _IO_REGISTRY_ENTRY {
    IIOService *pService;           /**< Service pointer */
    IIOService *pParent;            /**< Parent service */
    BOOLEAN     bUsed;              /**< Entry in use */
} IO_REGISTRY_ENTRY;

/**
 * @brief IORegistry implementation structure
 */
typedef struct _IO_REGISTRY_IMPL {
    IIORegistry             Vtbl;                       /**< Virtual function table */
    ULONG                   uRefCount;                  /**< Reference count */
    IIOService             *pRootService;               /**< Root service */
    IO_REGISTRY_ENTRY       Entries[IO_REGISTRY_MAX_SERVICES]; /**< Registry entries */
    UINT32                  uEntryCount;                /**< Number of entries */
} IO_REGISTRY_IMPL;

/**
 * @brief Registry iterator structure
 */
typedef struct _IO_REGISTRY_ITERATOR {
    IO_REGISTRY_IMPL   *pRegistry;      /**< Registry reference */
    UINT32              uCurrentIndex;  /**< Current iteration index */
    IIOService         *pRoot;          /**< Root for iteration */
    UINT32              uOptions;       /**< Iterator options */
} IO_REGISTRY_ITERATOR;

// Forward declarations
static RETCODE STDMETHODCALLTYPE IORegistry_QueryInterface(
    IIORegistry *pThis,
    REFIID riid,
    void **ppvObject
);

static ULONG STDMETHODCALLTYPE IORegistry_AddRef(
    IIORegistry *pThis
);

static ULONG STDMETHODCALLTYPE IORegistry_Release(
    IIORegistry *pThis
);

static IO_RETURN STDMETHODCALLTYPE IORegistry_GetRootService(
    IIORegistry *pThis,
    IIOService **ppRoot
);

static IO_RETURN STDMETHODCALLTYPE IORegistry_RegisterService(
    IIORegistry *pThis,
    IIOService *pService,
    IIOService *pParent
);

static IO_RETURN STDMETHODCALLTYPE IORegistry_UnregisterService(
    IIORegistry *pThis,
    IIOService *pService
);

static IO_RETURN STDMETHODCALLTYPE IORegistry_FindServicesByName(
    IIORegistry *pThis,
    CONST CHAR8 *pszName,
    CONST CHAR8 *pszPlane,
    UINT32 uOptions,
    IIOService **ppServices,
    UINT32 *puCount
);

static IO_RETURN STDMETHODCALLTYPE IORegistry_FindServicesByProperty(
    IIORegistry *pThis,
    CONST CHAR8 *pszKey,
    CONST VOID *pValue,
    UINTN cbSize,
    CONST CHAR8 *pszPlane,
    IIOService **ppServices,
    UINT32 *puCount
);

static IO_RETURN STDMETHODCALLTYPE IORegistry_GetServicePath(
    IIORegistry *pThis,
    IIOService *pService,
    CONST CHAR8 *pszPlane,
    CHAR8 *pszPath,
    UINTN cbSize
);

static IO_RETURN STDMETHODCALLTYPE IORegistry_GetServiceByPath(
    IIORegistry *pThis,
    CONST CHAR8 *pszPath,
    IIOService **ppService
);

static IO_RETURN STDMETHODCALLTYPE IORegistry_CreateIterator(
    IIORegistry *pThis,
    IIOService *pRoot,
    CONST CHAR8 *pszPlane,
    UINT32 uOptions,
    VOID **ppIterator
);

static IO_RETURN STDMETHODCALLTYPE IORegistry_IteratorNext(
    IIORegistry *pThis,
    VOID *pIterator,
    IIOService **ppService
);

static IO_RETURN STDMETHODCALLTYPE IORegistry_DestroyIterator(
    IIORegistry *pThis,
    VOID *pIterator
);

static IO_RETURN STDMETHODCALLTYPE IORegistry_DumpRegistry(
    IIORegistry *pThis,
    IIOService *pRoot,
    CONST CHAR8 *pszPlane,
    UINT32 uDepth
);

/**
 * @brief IORegistry virtual function table
 */
static IIORegistryVtbl g_IORegistryVtbl = {
    .QueryInterface         = IORegistry_QueryInterface,
    .AddRef                 = IORegistry_AddRef,
    .Release                = IORegistry_Release,
    .GetRootService         = IORegistry_GetRootService,
    .RegisterService        = IORegistry_RegisterService,
    .UnregisterService      = IORegistry_UnregisterService,
    .FindServicesByName     = IORegistry_FindServicesByName,
    .FindServicesByProperty = IORegistry_FindServicesByProperty,
    .GetServicePath         = IORegistry_GetServicePath,
    .GetServiceByPath       = IORegistry_GetServiceByPath,
    .CreateIterator         = IORegistry_CreateIterator,
    .IteratorNext           = IORegistry_IteratorNext,
    .DestroyIterator        = IORegistry_DestroyIterator,
    .DumpRegistry           = IORegistry_DumpRegistry,
};

/**
 * @brief Global registry instance
 */
static IO_REGISTRY_IMPL *g_pGlobalRegistry = NULL;

/**
 * @brief Create a new IORegistry instance
 *
 * @param ppRegistry    Receives pointer to new registry
 *
 * @retval IO_SUCCESS   Registry created successfully
 * @retval IO_NO_MEMORY Insufficient memory
 */
IO_RETURN
IORegistryCreate(
    IIORegistry **ppRegistry
    )
{
    IO_REGISTRY_IMPL *pImpl;
    IO_RETURN Status;

    if (ppRegistry == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Allocate registry instance
    pImpl = (IO_REGISTRY_IMPL *)malloc(sizeof(IO_REGISTRY_IMPL));
    if (pImpl == NULL) {
        return IO_NO_MEMORY;
    }

    // Initialize structure
    memset(pImpl, 0, sizeof(IO_REGISTRY_IMPL));
    pImpl->Vtbl.lpVtbl = &g_IORegistryVtbl;
    pImpl->uRefCount = 1;

    // Create root service
    Status = IOServiceCreate("Root", &pImpl->pRootService);
    if (Status != IO_SUCCESS) {
        free(pImpl);
        return Status;
    }

    // Register root service
    pImpl->Entries[0].pService = pImpl->pRootService;
    pImpl->Entries[0].pParent = NULL;
    pImpl->Entries[0].bUsed = TRUE;
    pImpl->uEntryCount = 1;

    *ppRegistry = (IIORegistry *)pImpl;
    return IO_SUCCESS;
}

/**
 * @brief Get the global registry instance
 *
 * @param ppRegistry    Receives pointer to global registry
 *
 * @retval IO_SUCCESS   Registry retrieved successfully
 * @retval IO_ERROR     Registry not initialized
 */
IO_RETURN
IORegistryGetGlobal(
    IIORegistry **ppRegistry
    )
{
    if (ppRegistry == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (g_pGlobalRegistry == NULL) {
        // Create global registry on first access
        IO_RETURN Status = IORegistryCreate((IIORegistry **)&g_pGlobalRegistry);
        if (Status != IO_SUCCESS) {
            return Status;
        }
    }

    *ppRegistry = (IIORegistry *)g_pGlobalRegistry;
    IIORegistry_AddRef(*ppRegistry);
    return IO_SUCCESS;
}

// Implementation of vtable functions

static RETCODE STDMETHODCALLTYPE
IORegistry_QueryInterface(
    IIORegistry *pThis,
    REFIID riid,
    void **ppvObject
    )
{
    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IIORegistry)) {
        *ppvObject = pThis;
        IIORegistry_AddRef(pThis);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE
IORegistry_AddRef(
    IIORegistry *pThis
    )
{
    IO_REGISTRY_IMPL *pImpl = (IO_REGISTRY_IMPL *)pThis;
    return ++pImpl->uRefCount;
}

static ULONG STDMETHODCALLTYPE
IORegistry_Release(
    IIORegistry *pThis
    )
{
    IO_REGISTRY_IMPL *pImpl = (IO_REGISTRY_IMPL *)pThis;
    ULONG uRefCount;
    UINT32 i;

    uRefCount = --pImpl->uRefCount;
    if (uRefCount == 0) {
        // Release all services
        for (i = 0; i < pImpl->uEntryCount; i++) {
            if (pImpl->Entries[i].bUsed && pImpl->Entries[i].pService != NULL) {
                IIOService_Release(pImpl->Entries[i].pService);
            }
        }

        // Free memory
        free(pImpl);
    }

    return uRefCount;
}

static IO_RETURN STDMETHODCALLTYPE
IORegistry_GetRootService(
    IIORegistry *pThis,
    IIOService **ppRoot
    )
{
    IO_REGISTRY_IMPL *pImpl = (IO_REGISTRY_IMPL *)pThis;

    if (ppRoot == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (pImpl->pRootService == NULL) {
        return IO_NO_DEVICE;
    }

    *ppRoot = pImpl->pRootService;
    IIOService_AddRef(pImpl->pRootService);
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
IORegistry_RegisterService(
    IIORegistry *pThis,
    IIOService *pService,
    IIOService *pParent
    )
{
    IO_REGISTRY_IMPL *pImpl = (IO_REGISTRY_IMPL *)pThis;
    UINT32 i;

    if (pService == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Use root as parent if none specified
    if (pParent == NULL) {
        pParent = pImpl->pRootService;
    }

    // Find empty slot
    for (i = 0; i < IO_REGISTRY_MAX_SERVICES; i++) {
        if (!pImpl->Entries[i].bUsed) {
            pImpl->Entries[i].pService = pService;
            pImpl->Entries[i].pParent = pParent;
            pImpl->Entries[i].bUsed = TRUE;

            IIOService_AddRef(pService);
            if (pParent != NULL) {
                IOServiceAddChild(pParent, pService);
            }

            if (i >= pImpl->uEntryCount) {
                pImpl->uEntryCount = i + 1;
            }

            return IO_SUCCESS;
        }
    }

    return IO_NO_MEMORY;
}

static IO_RETURN STDMETHODCALLTYPE
IORegistry_UnregisterService(
    IIORegistry *pThis,
    IIOService *pService
    )
{
    IO_REGISTRY_IMPL *pImpl = (IO_REGISTRY_IMPL *)pThis;
    UINT32 i;
    UINT32 uChildCount;
    IIOService *pChild;

    if (pService == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Check for children
    for (i = 0; ; i++) {
        if (IIOService_GetChildService(pService, i, &pChild) != IO_SUCCESS) {
            break;
        }
        IIOService_Release(pChild);
        return IO_BUSY;
    }

    // Find and remove entry
    for (i = 0; i < pImpl->uEntryCount; i++) {
        if (pImpl->Entries[i].bUsed && pImpl->Entries[i].pService == pService) {
            pImpl->Entries[i].bUsed = FALSE;
            IIOService_Release(pService);
            return IO_SUCCESS;
        }
    }

    return IO_NO_MATCH;
}

static IO_RETURN STDMETHODCALLTYPE
IORegistry_FindServicesByName(
    IIORegistry *pThis,
    CONST CHAR8 *pszName,
    CONST CHAR8 *pszPlane,
    UINT32 uOptions,
    IIOService **ppServices,
    UINT32 *puCount
    )
{
    IO_REGISTRY_IMPL *pImpl = (IO_REGISTRY_IMPL *)pThis;
    CHAR8 szServiceName[64];
    UINT32 i;
    UINT32 uFound = 0;
    UINT32 uMaxCount;

    if (pszName == NULL || ppServices == NULL || puCount == NULL) {
        return IO_BAD_ARGUMENT;
    }

    uMaxCount = *puCount;
    *puCount = 0;

    // Search all services
    for (i = 0; i < pImpl->uEntryCount && uFound < uMaxCount; i++) {
        if (pImpl->Entries[i].bUsed && pImpl->Entries[i].pService != NULL) {
            IIOService_GetServiceName(pImpl->Entries[i].pService, szServiceName, sizeof(szServiceName));
            if (strcmp(szServiceName, pszName) == 0) {
                ppServices[uFound] = pImpl->Entries[i].pService;
                IIOService_AddRef(ppServices[uFound]);
                uFound++;
            }
        }
    }

    *puCount = uFound;
    return (uFound > 0) ? IO_SUCCESS : IO_NO_MATCH;
}

static IO_RETURN STDMETHODCALLTYPE
IORegistry_FindServicesByProperty(
    IIORegistry *pThis,
    CONST CHAR8 *pszKey,
    CONST VOID *pValue,
    UINTN cbSize,
    CONST CHAR8 *pszPlane,
    IIOService **ppServices,
    UINT32 *puCount
    )
{
    IO_REGISTRY_IMPL *pImpl = (IO_REGISTRY_IMPL *)pThis;
    UINT8 PropertyData[256];
    UINTN cbPropertySize;
    UINT32 i;
    UINT32 uFound = 0;
    UINT32 uMaxCount;
    IO_RETURN Status;

    if (pszKey == NULL || ppServices == NULL || puCount == NULL) {
        return IO_BAD_ARGUMENT;
    }

    uMaxCount = *puCount;
    *puCount = 0;

    // Search all services
    for (i = 0; i < pImpl->uEntryCount && uFound < uMaxCount; i++) {
        if (pImpl->Entries[i].bUsed && pImpl->Entries[i].pService != NULL) {
            cbPropertySize = sizeof(PropertyData);
            Status = IIOService_GetProperty(pImpl->Entries[i].pService, pszKey,
                                           PropertyData, &cbPropertySize, NULL);
            if (Status == IO_SUCCESS) {
                // Check if value matches (if specified)
                if (pValue == NULL ||
                    (cbPropertySize == cbSize && memcmp(PropertyData, pValue, cbSize) == 0)) {
                    ppServices[uFound] = pImpl->Entries[i].pService;
                    IIOService_AddRef(ppServices[uFound]);
                    uFound++;
                }
            }
        }
    }

    *puCount = uFound;
    return (uFound > 0) ? IO_SUCCESS : IO_NO_MATCH;
}

static IO_RETURN STDMETHODCALLTYPE
IORegistry_GetServicePath(
    IIORegistry *pThis,
    IIOService *pService,
    CONST CHAR8 *pszPlane,
    CHAR8 *pszPath,
    UINTN cbSize
    )
{
    CHAR8 szName[64];

    if (pService == NULL || pszPath == NULL || cbSize == 0) {
        return IO_BAD_ARGUMENT;
    }

    // Simple implementation: just return service name
    IIOService_GetServiceName(pService, szName, sizeof(szName));
    snprintf(pszPath, cbSize, "IOService:/%s", szName);

    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
IORegistry_GetServiceByPath(
    IIORegistry *pThis,
    CONST CHAR8 *pszPath,
    IIOService **ppService
    )
{
    if (pszPath == NULL || ppService == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Not implemented in basic version
    return IO_UNSUPPORTED;
}

static IO_RETURN STDMETHODCALLTYPE
IORegistry_CreateIterator(
    IIORegistry *pThis,
    IIOService *pRoot,
    CONST CHAR8 *pszPlane,
    UINT32 uOptions,
    VOID **ppIterator
    )
{
    IO_REGISTRY_IMPL *pImpl = (IO_REGISTRY_IMPL *)pThis;
    IO_REGISTRY_ITERATOR *pIterator;

    if (ppIterator == NULL) {
        return IO_BAD_ARGUMENT;
    }

    pIterator = (IO_REGISTRY_ITERATOR *)malloc(sizeof(IO_REGISTRY_ITERATOR));
    if (pIterator == NULL) {
        return IO_NO_MEMORY;
    }

    pIterator->pRegistry = pImpl;
    pIterator->uCurrentIndex = 0;
    pIterator->pRoot = (pRoot != NULL) ? pRoot : pImpl->pRootService;
    pIterator->uOptions = uOptions;

    *ppIterator = pIterator;
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
IORegistry_IteratorNext(
    IIORegistry *pThis,
    VOID *pIterator,
    IIOService **ppService
    )
{
    IO_REGISTRY_ITERATOR *pIter = (IO_REGISTRY_ITERATOR *)pIterator;

    if (pIterator == NULL || ppService == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Find next service
    while (pIter->uCurrentIndex < pIter->pRegistry->uEntryCount) {
        if (pIter->pRegistry->Entries[pIter->uCurrentIndex].bUsed) {
            *ppService = pIter->pRegistry->Entries[pIter->uCurrentIndex].pService;
            IIOService_AddRef(*ppService);
            pIter->uCurrentIndex++;
            return IO_SUCCESS;
        }
        pIter->uCurrentIndex++;
    }

    return IO_NO_MATCH;
}

static IO_RETURN STDMETHODCALLTYPE
IORegistry_DestroyIterator(
    IIORegistry *pThis,
    VOID *pIterator
    )
{
    if (pIterator == NULL) {
        return IO_BAD_ARGUMENT;
    }

    free(pIterator);
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
IORegistry_DumpRegistry(
    IIORegistry *pThis,
    IIOService *pRoot,
    CONST CHAR8 *pszPlane,
    UINT32 uDepth
    )
{
    IO_REGISTRY_IMPL *pImpl = (IO_REGISTRY_IMPL *)pThis;
    CHAR8 szName[64];
    UINT32 i;

    printf("IORegistry Dump:\n");
    printf("===============\n");

    for (i = 0; i < pImpl->uEntryCount; i++) {
        if (pImpl->Entries[i].bUsed && pImpl->Entries[i].pService != NULL) {
            IIOService_GetServiceName(pImpl->Entries[i].pService, szName, sizeof(szName));
            printf("  [%u] %s\n", i, szName);
        }
    }

    return IO_SUCCESS;
}
