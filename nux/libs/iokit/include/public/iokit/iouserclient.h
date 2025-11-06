/**
 * @file iouserclient.h
 * @brief IOUserClient Interface - User-space to kernel-space communication
 *
 * IIOUserClient provides a mechanism for user-space applications and drivers
 * to communicate with kernel-space drivers. It supports method calls, shared
 * memory, and asynchronous notifications.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOUSERCLIENT_H
#define IOUSERCLIENT_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOUserClient interface GUID
 * {9F6D4E8A-3C7B-4A5E-8D9F-6E4B3A8C7D5F}
 */
DEFINE_GUID(IID_IIOUserClient,
    0x9F6D4E8A, 0x3C7B, 0x4A5E, 0x8D, 0x9F, 0x6E, 0x4B, 0x3A, 0x8C, 0x7D, 0x5F);

/**
 * @brief External method dispatch structure
 */
typedef struct _IO_EXTERNAL_METHOD {
    UINT32  uSelector;              /**< Method selector */
    UINT32  uInputScalarCount;      /**< Number of scalar input arguments */
    UINT32  uInputStructureSize;    /**< Size of input structure */
    UINT32  uOutputScalarCount;     /**< Number of scalar output arguments */
    UINT32  uOutputStructureSize;   /**< Size of output structure */
} IO_EXTERNAL_METHOD;

/**
 * @brief Async callback structure
 */
typedef struct _IO_ASYNC_CALLBACK {
    VOID    *pRefCon;               /**< Reference constant */
    VOID    (*pfnCallback)(         /**< Callback function */
        VOID *pRefCon,
        IO_RETURN Status,
        VOID *pArgs,
        UINT32 uArgCount
    );
} IO_ASYNC_CALLBACK;

/**
 * @brief Notification type flags
 */
typedef enum _IO_NOTIFICATION_TYPE {
    IO_NOTIFY_TERMINATION       = 0x00000001,   /**< Service terminated */
    IO_NOTIFY_FIRST_PUBLISH     = 0x00000002,   /**< First service published */
    IO_NOTIFY_MATCHED           = 0x00000004,   /**< Service matched */
    IO_NOTIFY_FIRST_MATCH       = 0x00000008,   /**< First match occurred */
} IO_NOTIFICATION_TYPE;

/**
 * @brief IIOUserClient - User-space to kernel-space communication interface
 *
 * This interface enables user-space applications and drivers to communicate
 * with kernel-space services through method calls, shared memory, and notifications.
 */
#undef INTERFACE
#define INTERFACE IIOUserClient

DECLARE_INTERFACE_(IIOUserClient, IIOService)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)(THIS_
        REFIID riid,
        void **ppvObject
        ) PURE;

    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IIOService methods (inherited)
    STDMETHOD_(IO_RETURN, Probe)(THIS_
        IIOService *pProvider,
        UINT32 *puProbeScore
        ) PURE;

    STDMETHOD_(IO_RETURN, Start)(THIS_
        IIOService *pProvider
        ) PURE;

    STDMETHOD_(IO_RETURN, Stop)(THIS_
        IIOService *pProvider
        ) PURE;

    STDMETHOD_(IO_RETURN, Terminate)(THIS_
        UINT32 uOptions
        ) PURE;

    STDMETHOD_(IO_RETURN, GetProperty)(THIS_
        CONST CHAR8 *pszKey,
        VOID *pValue,
        UINTN *pcbSize,
        UINT32 *puType
        ) PURE;

    STDMETHOD_(IO_RETURN, SetProperty)(THIS_
        CONST CHAR8 *pszKey,
        CONST VOID *pValue,
        UINTN cbSize,
        UINT32 uType
        ) PURE;

    STDMETHOD_(IO_RETURN, GetParentService)(THIS_
        IIOService **ppParent
        ) PURE;

    STDMETHOD_(IO_RETURN, GetChildService)(THIS_
        UINT32 uIndex,
        IIOService **ppChild
        ) PURE;

    STDMETHOD_(IO_RETURN, GetServiceState)(THIS_
        UINT32 *puState
        ) PURE;

    STDMETHOD_(IO_RETURN, GetServiceName)(THIS_
        CHAR8 *pszName,
        UINTN cbSize
        ) PURE;

    STDMETHOD_(IO_RETURN, RegisterService)(THIS_
        UINT32 uOptions
        ) PURE;

    // IIOUserClient methods

    /**
     * @brief Open user client
     *
     * Opens a connection from user space to the kernel service.
     *
     * @param uOptions      Open options (exclusive, shared, etc.)
     *
     * @retval IO_SUCCESS           Client opened successfully
     * @retval IO_EXCLUSIVE_ACCESS  Service already opened exclusively
     * @retval IO_ERROR             Failed to open client
     */
    STDMETHOD_(IO_RETURN, ClientOpen)(THIS_
        UINT32 uOptions
        ) PURE;

    /**
     * @brief Close user client
     *
     * Closes the connection and releases all associated resources.
     *
     * @param uOptions      Close options (synchronous, etc.)
     *
     * @retval IO_SUCCESS           Client closed successfully
     */
    STDMETHOD_(IO_RETURN, ClientClose)(THIS_
        UINT32 uOptions
        ) PURE;

    /**
     * @brief Call external method (scalar inputs/outputs)
     *
     * Invokes a method on the kernel service with scalar arguments.
     *
     * @param uSelector         Method selector
     * @param pInputScalars     Array of input scalar values
     * @param uInputScalarCount Number of input scalars
     * @param pOutputScalars    Array to receive output scalar values
     * @param puOutputScalarCount On input: max outputs; On output: actual count
     *
     * @retval IO_SUCCESS           Method executed successfully
     * @retval IO_BAD_ARGUMENT      Invalid selector or arguments
     * @retval IO_NOT_OPEN          Client not opened
     */
    STDMETHOD_(IO_RETURN, CallScalarMethod)(THIS_
        UINT32 uSelector,
        CONST UINT64 *pInputScalars,
        UINT32 uInputScalarCount,
        UINT64 *pOutputScalars,
        UINT32 *puOutputScalarCount
        ) PURE;

    /**
     * @brief Call external method (structure inputs/outputs)
     *
     * Invokes a method on the kernel service with structure arguments.
     *
     * @param uSelector             Method selector
     * @param pInputStructure       Input structure data
     * @param cbInputStructureSize  Size of input structure
     * @param pOutputStructure      Output structure buffer
     * @param pcbOutputStructureSize On input: buffer size; On output: actual size
     *
     * @retval IO_SUCCESS           Method executed successfully
     * @retval IO_BAD_ARGUMENT      Invalid selector or arguments
     * @retval IO_NOT_OPEN          Client not opened
     */
    STDMETHOD_(IO_RETURN, CallStructMethod)(THIS_
        UINT32 uSelector,
        CONST VOID *pInputStructure,
        UINTN cbInputStructureSize,
        VOID *pOutputStructure,
        UINTN *pcbOutputStructureSize
        ) PURE;

    /**
     * @brief Map memory into user space
     *
     * Maps a kernel memory region into the user-space address space.
     *
     * @param uMemoryType       Type of memory to map
     * @param uMapFlags         Mapping flags (read-only, write-combine, etc.)
     * @param ppAddress         Receives mapped user-space address
     * @param pcbSize           Receives size of mapped region
     *
     * @retval IO_SUCCESS           Memory mapped successfully
     * @retval IO_VM_ERROR          Virtual memory error
     * @retval IO_NOT_OPEN          Client not opened
     */
    STDMETHOD_(IO_RETURN, MapMemory)(THIS_
        UINT32 uMemoryType,
        UINT32 uMapFlags,
        VOID **ppAddress,
        UINTN *pcbSize
        ) PURE;

    /**
     * @brief Unmap memory from user space
     *
     * Unmaps a previously mapped kernel memory region.
     *
     * @param uMemoryType       Type of memory to unmap
     * @param pAddress          User-space address to unmap
     *
     * @retval IO_SUCCESS           Memory unmapped successfully
     * @retval IO_BAD_ARGUMENT      Invalid address
     */
    STDMETHOD_(IO_RETURN, UnmapMemory)(THIS_
        UINT32 uMemoryType,
        VOID *pAddress
        ) PURE;

    /**
     * @brief Register for asynchronous notifications
     *
     * Registers a callback for asynchronous notifications from the kernel service.
     *
     * @param uNotificationType Type of notification to register for
     * @param pCallback         Callback structure
     *
     * @retval IO_SUCCESS           Notification registered successfully
     * @retval IO_BAD_ARGUMENT      Invalid notification type
     * @retval IO_NOT_OPEN          Client not opened
     */
    STDMETHOD_(IO_RETURN, RegisterNotification)(THIS_
        UINT32 uNotificationType,
        IO_ASYNC_CALLBACK *pCallback
        ) PURE;

    /**
     * @brief Unregister from asynchronous notifications
     *
     * Unregisters a previously registered notification callback.
     *
     * @param uNotificationType Type of notification to unregister
     *
     * @retval IO_SUCCESS           Notification unregistered successfully
     */
    STDMETHOD_(IO_RETURN, UnregisterNotification)(THIS_
        UINT32 uNotificationType
        ) PURE;

    /**
     * @brief Get the owning task
     *
     * Retrieves the task (process) that owns this user client connection.
     *
     * @param ppTask            Receives task handle
     *
     * @retval IO_SUCCESS           Task retrieved successfully
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetOwningTask)(THIS_
        VOID **ppTask
        ) PURE;

    /**
     * @brief Set properties from user space
     *
     * Allows user space to set properties on the kernel service.
     *
     * @param pProperties       Array of properties to set
     * @param uPropertyCount    Number of properties
     *
     * @retval IO_SUCCESS           Properties set successfully
     * @retval IO_NOT_PRIVILEGED    Insufficient privileges
     * @retval IO_BAD_ARGUMENT      Invalid properties
     */
    STDMETHOD_(IO_RETURN, SetPropertiesFromUser)(THIS_
        CONST IO_PROPERTY *pProperties,
        UINT32 uPropertyCount
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOUserClient methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOUserClient_QueryInterface(p,a,b)                     (p)->lpVtbl->QueryInterface(p,a,b)
#define IIOUserClient_AddRef(p)                                  (p)->lpVtbl->AddRef(p)
#define IIOUserClient_Release(p)                                 (p)->lpVtbl->Release(p)
#define IIOUserClient_ClientOpen(p,a)                            (p)->lpVtbl->ClientOpen(p,a)
#define IIOUserClient_ClientClose(p,a)                           (p)->lpVtbl->ClientClose(p,a)
#define IIOUserClient_CallScalarMethod(p,a,b,c,d,e)              (p)->lpVtbl->CallScalarMethod(p,a,b,c,d,e)
#define IIOUserClient_CallStructMethod(p,a,b,c,d,e)              (p)->lpVtbl->CallStructMethod(p,a,b,c,d,e)
#define IIOUserClient_MapMemory(p,a,b,c,d)                       (p)->lpVtbl->MapMemory(p,a,b,c,d)
#define IIOUserClient_UnmapMemory(p,a,b)                         (p)->lpVtbl->UnmapMemory(p,a,b)
#define IIOUserClient_RegisterNotification(p,a,b)                (p)->lpVtbl->RegisterNotification(p,a,b)
#define IIOUserClient_UnregisterNotification(p,a)                (p)->lpVtbl->UnregisterNotification(p,a)
#define IIOUserClient_GetOwningTask(p,a)                         (p)->lpVtbl->GetOwningTask(p,a)
#define IIOUserClient_SetPropertiesFromUser(p,a,b)               (p)->lpVtbl->SetPropertiesFromUser(p,a,b)

#endif

#ifdef __cplusplus
}
#endif

#endif /* IOUSERCLIENT_H */
