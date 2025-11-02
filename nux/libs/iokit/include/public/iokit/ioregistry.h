/**
 * @file ioregistry.h
 * @brief IORegistry Interface - Device tree management
 *
 * IIORegistry provides a hierarchical database of services in the system.
 * It maintains the device tree and provides methods for traversal, searching,
 * and notification of changes.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOREGISTRY_H
#define IOREGISTRY_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIORegistry interface GUID
 * {7B4E8F1C-2D9A-4E6F-8C3D-5A9E7B4F2C1D}
 */
DEFINE_GUID(IID_IIORegistry,
    0x7B4E8F1C, 0x2D9A, 0x4E6F, 0x8C, 0x3D, 0x5A, 0x9E, 0x7B, 0x4F, 0x2C, 0x1D);

/**
 * @brief Registry plane names
 */
#define IO_SERVICE_PLANE        "IOService"      /**< Service plane */
#define IO_POWER_PLANE          "IOPower"        /**< Power management plane */
#define IO_DEVICE_TREE_PLANE    "IODeviceTree"   /**< Device tree plane */
#define IO_AUDIO_PLANE          "IOAudio"        /**< Audio device plane */
#define IO_FIREWIRE_PLANE       "IOFireWire"     /**< FireWire plane */
#define IO_USB_PLANE            "IOUSB"          /**< USB plane */

/**
 * @brief Search options for registry operations
 */
typedef enum _IO_REGISTRY_ITERATOR_OPTIONS {
    IO_REGISTRY_ITERATE_RECURSIVELY = 0x00000001,   /**< Iterate recursively */
    IO_REGISTRY_ITERATE_PARENTS     = 0x00000002,   /**< Iterate parent chain */
} IO_REGISTRY_ITERATOR_OPTIONS;

/**
 * @brief IIORegistry - Device tree management interface
 *
 * This interface provides methods for managing the I/O Registry, which is a
 * hierarchical database of all services (drivers and devices) in the system.
 */
#undef INTERFACE
#define INTERFACE IIORegistry

DECLARE_INTERFACE_(IIORegistry, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)(THIS_
        REFIID riid,
        void **ppvObject
        ) PURE;

    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IIORegistry methods

    /**
     * @brief Get the root service
     *
     * Retrieves the root service of the registry hierarchy. All other services
     * are descendants of the root.
     *
     * @param ppRoot        Receives pointer to root service interface
     *
     * @retval IO_SUCCESS           Root retrieved successfully
     * @retval IO_NO_DEVICE         Registry not initialized
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetRootService)(THIS_
        IIOService **ppRoot
        ) PURE;

    /**
     * @brief Register a service
     *
     * Adds a service to the registry and attaches it to the specified parent.
     *
     * @param pService      Service to register
     * @param pParent       Parent service (NULL for root level)
     *
     * @retval IO_SUCCESS           Service registered successfully
     * @retval IO_NO_MEMORY         Insufficient memory
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, RegisterService)(THIS_
        IIOService *pService,
        IIOService *pParent
        ) PURE;

    /**
     * @brief Unregister a service
     *
     * Removes a service from the registry and detaches it from its parent.
     *
     * @param pService      Service to unregister
     *
     * @retval IO_SUCCESS           Service unregistered successfully
     * @retval IO_BUSY              Service has active children
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, UnregisterService)(THIS_
        IIOService *pService
        ) PURE;

    /**
     * @brief Find services by name
     *
     * Searches the registry for services with the specified name.
     *
     * @param pszName       Service name to search for
     * @param pszPlane      Plane to search (NULL for all planes)
     * @param uOptions      Search options (recursive, parents, etc.)
     * @param ppServices    Receives array of matching services
     * @param puCount       On input: max services; On output: actual count
     *
     * @retval IO_SUCCESS           Services found successfully
     * @retval IO_NO_MATCH          No matching services
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, FindServicesByName)(THIS_
        CONST CHAR8 *pszName,
        CONST CHAR8 *pszPlane,
        UINT32 uOptions,
        IIOService **ppServices,
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Find services by property
     *
     * Searches the registry for services with a matching property.
     *
     * @param pszKey        Property key to match
     * @param pValue        Property value to match (NULL for existence check)
     * @param cbSize        Size of property value
     * @param pszPlane      Plane to search (NULL for all planes)
     * @param ppServices    Receives array of matching services
     * @param puCount       On input: max services; On output: actual count
     *
     * @retval IO_SUCCESS           Services found successfully
     * @retval IO_NO_MATCH          No matching services
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, FindServicesByProperty)(THIS_
        CONST CHAR8 *pszKey,
        CONST VOID *pValue,
        UINTN cbSize,
        CONST CHAR8 *pszPlane,
        IIOService **ppServices,
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Get service path
     *
     * Retrieves the full registry path for a service (e.g., "IOService:/Root/PCI0/USB0").
     *
     * @param pService      Service to get path for
     * @param pszPlane      Plane name (NULL for service plane)
     * @param pszPath       Buffer to receive path string
     * @param cbSize        Size of buffer in bytes
     *
     * @retval IO_SUCCESS           Path retrieved successfully
     * @retval IO_NO_DEVICE         Service not in registry
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetServicePath)(THIS_
        IIOService *pService,
        CONST CHAR8 *pszPlane,
        CHAR8 *pszPath,
        UINTN cbSize
        ) PURE;

    /**
     * @brief Get service by path
     *
     * Retrieves a service from the registry by its full path.
     *
     * @param pszPath       Registry path (e.g., "IOService:/Root/PCI0/USB0")
     * @param ppService     Receives pointer to service interface
     *
     * @retval IO_SUCCESS           Service found successfully
     * @retval IO_NO_MATCH          Service not found
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetServiceByPath)(THIS_
        CONST CHAR8 *pszPath,
        IIOService **ppService
        ) PURE;

    /**
     * @brief Create an iterator for registry traversal
     *
     * Creates an iterator for traversing the registry hierarchy.
     *
     * @param pRoot         Root service for iteration (NULL for registry root)
     * @param pszPlane      Plane to iterate (NULL for service plane)
     * @param uOptions      Iterator options (recursive, parents, etc.)
     * @param ppIterator    Receives iterator handle
     *
     * @retval IO_SUCCESS           Iterator created successfully
     * @retval IO_NO_MEMORY         Insufficient memory
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, CreateIterator)(THIS_
        IIOService *pRoot,
        CONST CHAR8 *pszPlane,
        UINT32 uOptions,
        VOID **ppIterator
        ) PURE;

    /**
     * @brief Get next service from iterator
     *
     * Retrieves the next service from a registry iterator.
     *
     * @param pIterator     Iterator handle
     * @param ppService     Receives pointer to next service
     *
     * @retval IO_SUCCESS           Service retrieved successfully
     * @retval IO_NO_MATCH          No more services
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, IteratorNext)(THIS_
        VOID *pIterator,
        IIOService **ppService
        ) PURE;

    /**
     * @brief Destroy an iterator
     *
     * Releases resources associated with a registry iterator.
     *
     * @param pIterator     Iterator handle to destroy
     *
     * @retval IO_SUCCESS           Iterator destroyed successfully
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, DestroyIterator)(THIS_
        VOID *pIterator
        ) PURE;

    /**
     * @brief Dump registry tree to console
     *
     * Prints the entire registry tree for debugging purposes.
     *
     * @param pRoot         Root service to dump from (NULL for entire tree)
     * @param pszPlane      Plane to dump (NULL for service plane)
     * @param uDepth        Maximum depth to display (0 for unlimited)
     *
     * @retval IO_SUCCESS           Registry dumped successfully
     */
    STDMETHOD_(IO_RETURN, DumpRegistry)(THIS_
        IIOService *pRoot,
        CONST CHAR8 *pszPlane,
        UINT32 uDepth
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIORegistry methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIORegistry_QueryInterface(p,a,b)               (p)->lpVtbl->QueryInterface(p,a,b)
#define IIORegistry_AddRef(p)                            (p)->lpVtbl->AddRef(p)
#define IIORegistry_Release(p)                           (p)->lpVtbl->Release(p)
#define IIORegistry_GetRootService(p,a)                  (p)->lpVtbl->GetRootService(p,a)
#define IIORegistry_RegisterService(p,a,b)               (p)->lpVtbl->RegisterService(p,a,b)
#define IIORegistry_UnregisterService(p,a)               (p)->lpVtbl->UnregisterService(p,a)
#define IIORegistry_FindServicesByName(p,a,b,c,d,e)      (p)->lpVtbl->FindServicesByName(p,a,b,c,d,e)
#define IIORegistry_FindServicesByProperty(p,a,b,c,d,e,f) (p)->lpVtbl->FindServicesByProperty(p,a,b,c,d,e,f)
#define IIORegistry_GetServicePath(p,a,b,c,d)            (p)->lpVtbl->GetServicePath(p,a,b,c,d)
#define IIORegistry_GetServiceByPath(p,a,b)              (p)->lpVtbl->GetServiceByPath(p,a,b)
#define IIORegistry_CreateIterator(p,a,b,c,d)            (p)->lpVtbl->CreateIterator(p,a,b,c,d)
#define IIORegistry_IteratorNext(p,a,b)                  (p)->lpVtbl->IteratorNext(p,a,b)
#define IIORegistry_DestroyIterator(p,a)                 (p)->lpVtbl->DestroyIterator(p,a)
#define IIORegistry_DumpRegistry(p,a,b,c)                (p)->lpVtbl->DumpRegistry(p,a,b,c)

#endif

#ifdef __cplusplus
}
#endif

#endif /* IOREGISTRY_H */
