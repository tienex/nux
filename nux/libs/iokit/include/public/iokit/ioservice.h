/**
 * @file ioservice.h
 * @brief IOService Interface - Base interface for all I/O Kit drivers
 *
 * IIOService is the base interface for all drivers and devices in the I/O Kit framework.
 * It provides the fundamental lifecycle methods (probe, start, stop, terminate) and
 * property management capabilities.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOSERVICE_H
#define IOSERVICE_H

#include <iokit/iokit.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOService interface GUID
 * {8A5F9C2D-1E3B-4F7A-9D6E-2C8B4A7F1E9D}
 */
DEFINE_GUID(IID_IIOService,
    0x8A5F9C2D, 0x1E3B, 0x4F7A, 0x9D, 0x6E, 0x2C, 0x8B, 0x4A, 0x7F, 0x1E, 0x9D);

/**
 * @brief IIOService - Base interface for all I/O Kit services
 *
 * This interface provides the core functionality for driver lifecycle management,
 * property handling, and parent-child relationships in the device tree.
 */
#undef INTERFACE
#define INTERFACE IIOService

DECLARE_INTERFACE_(IIOService, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)(THIS_
        REFIID riid,
        void **ppvObject
        ) PURE;

    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IIOService methods

    /**
     * @brief Probe for device support
     *
     * Called to determine if this driver can support the specified provider.
     * The driver should examine the provider's properties and return a probe
     * score indicating its suitability.
     *
     * @param pProvider     Pointer to the provider service
     * @param puProbeScore  Receives the probe score (higher values have priority)
     *
     * @retval IO_SUCCESS           Driver can support this device
     * @retval IO_NO_MATCH          Driver cannot support this device
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, Probe)(THIS_
        IIOService *pProvider,
        UINT32 *puProbeScore
        ) PURE;

    /**
     * @brief Start the service
     *
     * Called to initialize the driver after a successful probe. The driver should
     * allocate resources, configure hardware, and register any child services.
     *
     * @param pProvider     Pointer to the provider service
     *
     * @retval IO_SUCCESS           Service started successfully
     * @retval IO_ERROR             Failed to start service
     * @retval IO_NO_RESOURCES      Insufficient resources
     */
    STDMETHOD_(IO_RETURN, Start)(THIS_
        IIOService *pProvider
        ) PURE;

    /**
     * @brief Stop the service
     *
     * Called to quiesce the driver before termination. The driver should stop
     * all I/O operations and prepare for resource deallocation.
     *
     * @param pProvider     Pointer to the provider service
     *
     * @retval IO_SUCCESS           Service stopped successfully
     * @retval IO_ERROR             Failed to stop service
     */
    STDMETHOD_(IO_RETURN, Stop)(THIS_
        IIOService *pProvider
        ) PURE;

    /**
     * @brief Terminate the service
     *
     * Called to deallocate all resources and remove the service from the registry.
     * This is the final cleanup before the service is destroyed.
     *
     * @param uOptions      Termination options (synchronous/asynchronous)
     *
     * @retval IO_SUCCESS           Service terminated successfully
     * @retval IO_BUSY              Service still has active clients
     */
    STDMETHOD_(IO_RETURN, Terminate)(THIS_
        UINT32 uOptions
        ) PURE;

    /**
     * @brief Get a property value
     *
     * Retrieves the value of a named property from the service's property table.
     *
     * @param pszKey        Property key name (null-terminated string)
     * @param pValue        Buffer to receive property value
     * @param pcbSize       On input: size of buffer; On output: actual size
     * @param puType        Receives the property type
     *
     * @retval IO_SUCCESS           Property retrieved successfully
     * @retval IO_NO_MATCH          Property not found
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetProperty)(THIS_
        CONST CHAR8 *pszKey,
        VOID *pValue,
        UINTN *pcbSize,
        UINT32 *puType
        ) PURE;

    /**
     * @brief Set a property value
     *
     * Sets the value of a named property in the service's property table.
     *
     * @param pszKey        Property key name (null-terminated string)
     * @param pValue        Property value to set
     * @param cbSize        Size of property value in bytes
     * @param uType         Property type
     *
     * @retval IO_SUCCESS           Property set successfully
     * @retval IO_NO_MEMORY         Insufficient memory
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, SetProperty)(THIS_
        CONST CHAR8 *pszKey,
        CONST VOID *pValue,
        UINTN cbSize,
        UINT32 uType
        ) PURE;

    /**
     * @brief Get the parent service
     *
     * Retrieves the parent service in the device hierarchy.
     *
     * @param ppParent      Receives pointer to parent service interface
     *
     * @retval IO_SUCCESS           Parent retrieved successfully
     * @retval IO_NO_DEVICE         No parent service
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetParentService)(THIS_
        IIOService **ppParent
        ) PURE;

    /**
     * @brief Get a child service by index
     *
     * Retrieves a child service from the device hierarchy by index.
     *
     * @param uIndex        Index of child service (zero-based)
     * @param ppChild       Receives pointer to child service interface
     *
     * @retval IO_SUCCESS           Child retrieved successfully
     * @retval IO_NO_DEVICE         No child at specified index
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetChildService)(THIS_
        UINT32 uIndex,
        IIOService **ppChild
        ) PURE;

    /**
     * @brief Get the service state
     *
     * Retrieves the current state of the service (inactive, registered, matched,
     * started, busy, terminated).
     *
     * @param puState       Receives the service state flags
     *
     * @retval IO_SUCCESS           State retrieved successfully
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetServiceState)(THIS_
        UINT32 *puState
        ) PURE;

    /**
     * @brief Get the service name
     *
     * Retrieves the name of this service.
     *
     * @param pszName       Buffer to receive service name
     * @param cbSize        Size of buffer in bytes
     *
     * @retval IO_SUCCESS           Name retrieved successfully
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetServiceName)(THIS_
        CHAR8 *pszName,
        UINTN cbSize
        ) PURE;

    /**
     * @brief Register the service
     *
     * Registers this service with the I/O Registry and triggers matching.
     *
     * @param uOptions      Registration options
     *
     * @retval IO_SUCCESS           Service registered successfully
     * @retval IO_ERROR             Failed to register service
     */
    STDMETHOD_(IO_RETURN, RegisterService)(THIS_
        UINT32 uOptions
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOService methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOService_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IIOService_AddRef(p)                     (p)->lpVtbl->AddRef(p)
#define IIOService_Release(p)                    (p)->lpVtbl->Release(p)
#define IIOService_Probe(p,a,b)                  (p)->lpVtbl->Probe(p,a,b)
#define IIOService_Start(p,a)                    (p)->lpVtbl->Start(p,a)
#define IIOService_Stop(p,a)                     (p)->lpVtbl->Stop(p,a)
#define IIOService_Terminate(p,a)                (p)->lpVtbl->Terminate(p,a)
#define IIOService_GetProperty(p,a,b,c,d)        (p)->lpVtbl->GetProperty(p,a,b,c,d)
#define IIOService_SetProperty(p,a,b,c,d)        (p)->lpVtbl->SetProperty(p,a,b,c,d)
#define IIOService_GetParentService(p,a)         (p)->lpVtbl->GetParentService(p,a)
#define IIOService_GetChildService(p,a,b)        (p)->lpVtbl->GetChildService(p,a,b)
#define IIOService_GetServiceState(p,a)          (p)->lpVtbl->GetServiceState(p,a)
#define IIOService_GetServiceName(p,a,b)         (p)->lpVtbl->GetServiceName(p,a,b)
#define IIOService_RegisterService(p,a)          (p)->lpVtbl->RegisterService(p,a)

#endif

#ifdef __cplusplus
}
#endif

#endif /* IOSERVICE_H */
