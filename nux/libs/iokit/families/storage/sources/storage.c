/**
 * @file storage.c
 * @brief Storage Family Implementation - Unified Block Storage Abstraction
 *
 * Provides a protocol-agnostic abstraction layer for all block storage devices.
 * This implementation wraps protocol-specific drivers (NVMe, SATA, SCSI, SAS)
 * and exposes a unified interface for upper layers.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/iokit.h>
#include <iokit/families/storage/storage.h>
#include <ananke/ntrtl.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief Storage device implementation structure
 */
typedef struct _STORAGE_DEVICE_IMPL {
    IIOStorageDevice        Vtbl;               /**< Virtual function table */
    ULONG                   RefCount;           /**< Reference count */
    IIOService             *pProtocolDevice;    /**< Underlying protocol device */
    STORAGE_PROTOCOL        Protocol;           /**< Storage protocol */
    STORAGE_DEVICE_INFO     DeviceInfo;         /**< Cached device information */
    STORAGE_IO_STATS        IOStats;            /**< I/O statistics */
    BOOLEAN                 bInitialized;       /**< Initialization flag */
} STORAGE_DEVICE_IMPL;

/**
 * @brief Storage controller implementation structure
 */
typedef struct _STORAGE_CONTROLLER_IMPL {
    IIOStorageController    Vtbl;               /**< Virtual function table */
    ULONG                   RefCount;           /**< Reference count */
    IIOService             *pProtocolController;/**< Underlying protocol controller */
    STORAGE_PROTOCOL        Protocol;           /**< Storage protocol */
    STORAGE_CONTROLLER_INFO ControllerInfo;     /**< Cached controller information */
    IIOStorageDevice      **ppDevices;          /**< Array of attached devices */
    UINT32                  uNumDevices;        /**< Number of attached devices */
    UINT32                  uMaxDevices;        /**< Maximum devices capacity */
    BOOLEAN                 bInitialized;       /**< Initialization flag */
} STORAGE_CONTROLLER_IMPL;

//
// Forward declarations - IIOStorageDevice
//
static HRESULT STDMETHODCALLTYPE StorageDevice_QueryInterface(
    IIOStorageDevice *pThis,
    REFIID riid,
    void **ppvObject);
static ULONG STDMETHODCALLTYPE StorageDevice_AddRef(IIOStorageDevice *pThis);
static ULONG STDMETHODCALLTYPE StorageDevice_Release(IIOStorageDevice *pThis);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_Probe(IIOStorageDevice *pThis, IIOService *pProvider, UINT32 *puProbeScore);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_Start(IIOStorageDevice *pThis, IIOService *pProvider);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_Stop(IIOStorageDevice *pThis, IIOService *pProvider);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_Terminate(IIOStorageDevice *pThis, UINT32 uOptions);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_GetProperty(IIOStorageDevice *pThis, CONST CHAR8 *pszKey, VOID *pValue, UINTN *pcbSize, UINT32 *puType);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_SetProperty(IIOStorageDevice *pThis, CONST CHAR8 *pszKey, CONST VOID *pValue, UINTN cbSize, UINT32 uType);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_GetParentService(IIOStorageDevice *pThis, IIOService **ppParent);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_GetChildService(IIOStorageDevice *pThis, UINT32 uIndex, IIOService **ppChild);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_GetServiceState(IIOStorageDevice *pThis, UINT32 *puState);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_GetServiceName(IIOStorageDevice *pThis, CHAR8 *pszName, UINTN cbSize);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_RegisterService(IIOStorageDevice *pThis, UINT32 uOptions);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_GetDeviceInfo(IIOStorageDevice *pThis, STORAGE_DEVICE_INFO *pDeviceInfo);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_Read(IIOStorageDevice *pThis, UINT64 uStartBlock, UINT32 uNumBlocks, VOID *pBuffer, UINTN cbBuffer, UINT32 uFlags, UINTN *puBytesRead);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_Write(IIOStorageDevice *pThis, UINT64 uStartBlock, UINT32 uNumBlocks, CONST VOID *pBuffer, UINTN cbBuffer, UINT32 uFlags, UINTN *puBytesWritten);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_Flush(IIOStorageDevice *pThis);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_Trim(IIOStorageDevice *pThis, UINT64 uStartBlock, UINT32 uNumBlocks);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_WriteZeroes(IIOStorageDevice *pThis, UINT64 uStartBlock, UINT32 uNumBlocks, UINT32 uFlags);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_GetIOStats(IIOStorageDevice *pThis, STORAGE_IO_STATS *pStats, BOOLEAN bReset);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_GetSMARTInfo(IIOStorageDevice *pThis, STORAGE_SMART_INFO *pSmartInfo);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_SecureErase(IIOStorageDevice *pThis, BOOLEAN bEnhanced);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_Sanitize(IIOStorageDevice *pThis);
static IO_RETURN STDMETHODCALLTYPE StorageDevice_SetPowerState(IIOStorageDevice *pThis, UINT32 uPowerState);

//
// Forward declarations - IIOStorageController
//
static HRESULT STDMETHODCALLTYPE StorageController_QueryInterface(IIOStorageController *pThis, REFIID riid, void **ppvObject);
static ULONG STDMETHODCALLTYPE StorageController_AddRef(IIOStorageController *pThis);
static ULONG STDMETHODCALLTYPE StorageController_Release(IIOStorageController *pThis);
static IO_RETURN STDMETHODCALLTYPE StorageController_Probe(IIOStorageController *pThis, IIOService *pProvider, UINT32 *puProbeScore);
static IO_RETURN STDMETHODCALLTYPE StorageController_Start(IIOStorageController *pThis, IIOService *pProvider);
static IO_RETURN STDMETHODCALLTYPE StorageController_Stop(IIOStorageController *pThis, IIOService *pProvider);
static IO_RETURN STDMETHODCALLTYPE StorageController_Terminate(IIOStorageController *pThis, UINT32 uOptions);
static IO_RETURN STDMETHODCALLTYPE StorageController_GetProperty(IIOStorageController *pThis, CONST CHAR8 *pszKey, VOID *pValue, UINTN *pcbSize, UINT32 *puType);
static IO_RETURN STDMETHODCALLTYPE StorageController_SetProperty(IIOStorageController *pThis, CONST CHAR8 *pszKey, CONST VOID *pValue, UINTN cbSize, UINT32 uType);
static IO_RETURN STDMETHODCALLTYPE StorageController_GetParentService(IIOStorageController *pThis, IIOService **ppParent);
static IO_RETURN STDMETHODCALLTYPE StorageController_GetChildService(IIOStorageController *pThis, UINT32 uIndex, IIOService **ppChild);
static IO_RETURN STDMETHODCALLTYPE StorageController_GetServiceState(IIOStorageController *pThis, UINT32 *puState);
static IO_RETURN STDMETHODCALLTYPE StorageController_GetServiceName(IIOStorageController *pThis, CHAR8 *pszName, UINTN cbSize);
static IO_RETURN STDMETHODCALLTYPE StorageController_RegisterService(IIOStorageController *pThis, UINT32 uOptions);
static IO_RETURN STDMETHODCALLTYPE StorageController_GetControllerInfo(IIOStorageController *pThis, STORAGE_CONTROLLER_INFO *pControllerInfo);
static IO_RETURN STDMETHODCALLTYPE StorageController_EnumerateDevices(IIOStorageController *pThis);
static IO_RETURN STDMETHODCALLTYPE StorageController_GetDeviceCount(IIOStorageController *pThis, UINT32 *puCount);
static IO_RETURN STDMETHODCALLTYPE StorageController_GetDevice(IIOStorageController *pThis, UINT32 uIndex, IIOStorageDevice **ppDevice);
static IO_RETURN STDMETHODCALLTYPE StorageController_ResetController(IIOStorageController *pThis);
static IO_RETURN STDMETHODCALLTYPE StorageController_ResetPort(IIOStorageController *pThis, UINT32 uPortIndex);
static IO_RETURN STDMETHODCALLTYPE StorageController_GetPortStatus(IIOStorageController *pThis, UINT32 uPortIndex, UINT32 *puStatus);
static IO_RETURN STDMETHODCALLTYPE StorageController_SetHotPlugEnable(IIOStorageController *pThis, BOOLEAN bEnable);

//
// IIOStorageDevice VTable
//
static CONST IIOStorageDeviceVtbl g_StorageDeviceVtbl = {
    // IUnknown
    StorageDevice_QueryInterface,
    StorageDevice_AddRef,
    StorageDevice_Release,
    // IIOService
    StorageDevice_Probe,
    StorageDevice_Start,
    StorageDevice_Stop,
    StorageDevice_Terminate,
    StorageDevice_GetProperty,
    StorageDevice_SetProperty,
    StorageDevice_GetParentService,
    StorageDevice_GetChildService,
    StorageDevice_GetServiceState,
    StorageDevice_GetServiceName,
    StorageDevice_RegisterService,
    // IIOStorageDevice
    StorageDevice_GetDeviceInfo,
    StorageDevice_Read,
    StorageDevice_Write,
    StorageDevice_Flush,
    StorageDevice_Trim,
    StorageDevice_WriteZeroes,
    StorageDevice_GetIOStats,
    StorageDevice_GetSMARTInfo,
    StorageDevice_SecureErase,
    StorageDevice_Sanitize,
    StorageDevice_SetPowerState,
};

//
// IIOStorageController VTable
//
static CONST IIOStorageControllerVtbl g_StorageControllerVtbl = {
    // IUnknown
    StorageController_QueryInterface,
    StorageController_AddRef,
    StorageController_Release,
    // IIOService
    StorageController_Probe,
    StorageController_Start,
    StorageController_Stop,
    StorageController_Terminate,
    StorageController_GetProperty,
    StorageController_SetProperty,
    StorageController_GetParentService,
    StorageController_GetChildService,
    StorageController_GetServiceState,
    StorageController_GetServiceName,
    StorageController_RegisterService,
    // IIOStorageController
    StorageController_GetControllerInfo,
    StorageController_EnumerateDevices,
    StorageController_GetDeviceCount,
    StorageController_GetDevice,
    StorageController_ResetController,
    StorageController_ResetPort,
    StorageController_GetPortStatus,
    StorageController_SetHotPlugEnable,
};

//
// Global state
//
static BOOLEAN g_bStorageInitialized = FALSE;

//
// Helper functions
//

/**
 * @brief Get protocol name string
 */
static CONST CHAR8*
StorageGetProtocolName(STORAGE_PROTOCOL Protocol)
{
    switch (Protocol) {
        case STORAGE_PROTOCOL_NVME:     return "NVMe";
        case STORAGE_PROTOCOL_SATA:     return "SATA";
        case STORAGE_PROTOCOL_SCSI:     return "SCSI";
        case STORAGE_PROTOCOL_SAS:      return "SAS";
        case STORAGE_PROTOCOL_PATA:     return "PATA";
        case STORAGE_PROTOCOL_USB:      return "USB";
        case STORAGE_PROTOCOL_VIRTIO:   return "VirtIO";
        case STORAGE_PROTOCOL_EMMC:     return "eMMC";
        case STORAGE_PROTOCOL_SD:       return "SD";
        case STORAGE_PROTOCOL_NVMEOF:   return "NVMe-oF";
        case STORAGE_PROTOCOL_ISCSI:    return "iSCSI";
        default:                        return "Unknown";
    }
}

//
// IIOStorageDevice Implementation
//

static HRESULT STDMETHODCALLTYPE
StorageDevice_QueryInterface(
    IIOStorageDevice *pThis,
    REFIID riid,
    void **ppvObject)
{
    STORAGE_DEVICE_IMPL *pImpl = (STORAGE_DEVICE_IMPL*)pThis;

    if (!ppvObject) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IIOService) ||
        IsEqualGUID(riid, &IID_IIOStorageDevice)) {
        *ppvObject = pThis;
        StorageDevice_AddRef(pThis);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE
StorageDevice_AddRef(IIOStorageDevice *pThis)
{
    STORAGE_DEVICE_IMPL *pImpl = (STORAGE_DEVICE_IMPL*)pThis;
    return ++pImpl->RefCount;
}

static ULONG STDMETHODCALLTYPE
StorageDevice_Release(IIOStorageDevice *pThis)
{
    STORAGE_DEVICE_IMPL *pImpl = (STORAGE_DEVICE_IMPL*)pThis;
    ULONG uRef = --pImpl->RefCount;

    if (uRef == 0) {
        printf("[Storage] Releasing storage device (%s)\n",
               StorageGetProtocolName(pImpl->Protocol));

        if (pImpl->pProtocolDevice) {
            pImpl->pProtocolDevice->lpVtbl->Release(pImpl->pProtocolDevice);
        }

        free(pImpl);
    }

    return uRef;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_Probe(IIOStorageDevice *pThis, IIOService *pProvider, UINT32 *puProbeScore)
{
    printf("[Storage] StorageDevice_Probe: stub implementation\n");
    if (puProbeScore) {
        *puProbeScore = 5000;  // Default probe score
    }
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_Start(IIOStorageDevice *pThis, IIOService *pProvider)
{
    STORAGE_DEVICE_IMPL *pImpl = (STORAGE_DEVICE_IMPL*)pThis;
    printf("[Storage] Starting storage device (%s)\n",
           StorageGetProtocolName(pImpl->Protocol));
    pImpl->bInitialized = TRUE;
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_Stop(IIOStorageDevice *pThis, IIOService *pProvider)
{
    STORAGE_DEVICE_IMPL *pImpl = (STORAGE_DEVICE_IMPL*)pThis;
    printf("[Storage] Stopping storage device (%s)\n",
           StorageGetProtocolName(pImpl->Protocol));
    pImpl->bInitialized = FALSE;
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_Terminate(IIOStorageDevice *pThis, UINT32 uOptions)
{
    printf("[Storage] StorageDevice_Terminate: stub implementation\n");
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_GetProperty(IIOStorageDevice *pThis, CONST CHAR8 *pszKey, VOID *pValue, UINTN *pcbSize, UINT32 *puType)
{
    STORAGE_DEVICE_IMPL *pImpl = (STORAGE_DEVICE_IMPL*)pThis;

    // Delegate to protocol device
    if (pImpl->pProtocolDevice) {
        return pImpl->pProtocolDevice->lpVtbl->GetProperty(pImpl->pProtocolDevice, pszKey, pValue, pcbSize, puType);
    }

    return IO_NO_MATCH;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_SetProperty(IIOStorageDevice *pThis, CONST CHAR8 *pszKey, CONST VOID *pValue, UINTN cbSize, UINT32 uType)
{
    STORAGE_DEVICE_IMPL *pImpl = (STORAGE_DEVICE_IMPL*)pThis;

    // Delegate to protocol device
    if (pImpl->pProtocolDevice) {
        return pImpl->pProtocolDevice->lpVtbl->SetProperty(pImpl->pProtocolDevice, pszKey, pValue, cbSize, uType);
    }

    return IO_UNSUPPORTED;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_GetParentService(IIOStorageDevice *pThis, IIOService **ppParent)
{
    STORAGE_DEVICE_IMPL *pImpl = (STORAGE_DEVICE_IMPL*)pThis;

    if (!ppParent) {
        return IO_BAD_ARGUMENT;
    }

    if (pImpl->pProtocolDevice) {
        return pImpl->pProtocolDevice->lpVtbl->GetParentService(pImpl->pProtocolDevice, ppParent);
    }

    *ppParent = NULL;
    return IO_NO_DEVICE;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_GetChildService(IIOStorageDevice *pThis, UINT32 uIndex, IIOService **ppChild)
{
    printf("[Storage] StorageDevice_GetChildService: stub implementation\n");
    if (ppChild) {
        *ppChild = NULL;
    }
    return IO_NO_DEVICE;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_GetServiceState(IIOStorageDevice *pThis, UINT32 *puState)
{
    STORAGE_DEVICE_IMPL *pImpl = (STORAGE_DEVICE_IMPL*)pThis;

    if (!puState) {
        return IO_BAD_ARGUMENT;
    }

    *puState = pImpl->bInitialized ? IO_SERVICE_STARTED : IO_SERVICE_INACTIVE;
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_GetServiceName(IIOStorageDevice *pThis, CHAR8 *pszName, UINTN cbSize)
{
    STORAGE_DEVICE_IMPL *pImpl = (STORAGE_DEVICE_IMPL*)pThis;

    if (!pszName || cbSize == 0) {
        return IO_BAD_ARGUMENT;
    }

    snprintf(pszName, cbSize, "StorageDevice (%s)", StorageGetProtocolName(pImpl->Protocol));
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_RegisterService(IIOStorageDevice *pThis, UINT32 uOptions)
{
    printf("[Storage] StorageDevice_RegisterService: stub implementation\n");
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_GetDeviceInfo(IIOStorageDevice *pThis, STORAGE_DEVICE_INFO *pDeviceInfo)
{
    STORAGE_DEVICE_IMPL *pImpl = (STORAGE_DEVICE_IMPL*)pThis;

    if (!pDeviceInfo) {
        return IO_BAD_ARGUMENT;
    }

    // Return cached device info
    memcpy(pDeviceInfo, &pImpl->DeviceInfo, sizeof(STORAGE_DEVICE_INFO));
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_Read(IIOStorageDevice *pThis, UINT64 uStartBlock, UINT32 uNumBlocks, VOID *pBuffer, UINTN cbBuffer, UINT32 uFlags, UINTN *puBytesRead)
{
    STORAGE_DEVICE_IMPL *pImpl = (STORAGE_DEVICE_IMPL*)pThis;

    printf("[Storage] Read: LBA=%llu, Blocks=%u (stub)\n",
           (unsigned long long)uStartBlock, uNumBlocks);

    // Update statistics
    pImpl->IOStats.ReadOperations++;

    // TODO: Delegate to protocol-specific read
    return IO_UNSUPPORTED;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_Write(IIOStorageDevice *pThis, UINT64 uStartBlock, UINT32 uNumBlocks, CONST VOID *pBuffer, UINTN cbBuffer, UINT32 uFlags, UINTN *puBytesWritten)
{
    STORAGE_DEVICE_IMPL *pImpl = (STORAGE_DEVICE_IMPL*)pThis;

    printf("[Storage] Write: LBA=%llu, Blocks=%u (stub)\n",
           (unsigned long long)uStartBlock, uNumBlocks);

    // Update statistics
    pImpl->IOStats.WriteOperations++;

    // TODO: Delegate to protocol-specific write
    return IO_UNSUPPORTED;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_Flush(IIOStorageDevice *pThis)
{
    STORAGE_DEVICE_IMPL *pImpl = (STORAGE_DEVICE_IMPL*)pThis;

    printf("[Storage] Flush (stub)\n");
    pImpl->IOStats.FlushOperations++;

    // TODO: Delegate to protocol-specific flush
    return IO_UNSUPPORTED;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_Trim(IIOStorageDevice *pThis, UINT64 uStartBlock, UINT32 uNumBlocks)
{
    STORAGE_DEVICE_IMPL *pImpl = (STORAGE_DEVICE_IMPL*)pThis;

    printf("[Storage] Trim: LBA=%llu, Blocks=%u (stub)\n",
           (unsigned long long)uStartBlock, uNumBlocks);

    pImpl->IOStats.TrimOperations++;

    // TODO: Delegate to protocol-specific trim
    return IO_UNSUPPORTED;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_WriteZeroes(IIOStorageDevice *pThis, UINT64 uStartBlock, UINT32 uNumBlocks, UINT32 uFlags)
{
    printf("[Storage] WriteZeroes: LBA=%llu, Blocks=%u (stub)\n",
           (unsigned long long)uStartBlock, uNumBlocks);
    return IO_UNSUPPORTED;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_GetIOStats(IIOStorageDevice *pThis, STORAGE_IO_STATS *pStats, BOOLEAN bReset)
{
    STORAGE_DEVICE_IMPL *pImpl = (STORAGE_DEVICE_IMPL*)pThis;

    if (!pStats) {
        return IO_BAD_ARGUMENT;
    }

    memcpy(pStats, &pImpl->IOStats, sizeof(STORAGE_IO_STATS));

    if (bReset) {
        memset(&pImpl->IOStats, 0, sizeof(STORAGE_IO_STATS));
        printf("[Storage] I/O statistics reset\n");
    }

    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_GetSMARTInfo(IIOStorageDevice *pThis, STORAGE_SMART_INFO *pSmartInfo)
{
    printf("[Storage] GetSMARTInfo (stub)\n");
    return IO_UNSUPPORTED;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_SecureErase(IIOStorageDevice *pThis, BOOLEAN bEnhanced)
{
    printf("[Storage] SecureErase (enhanced=%d) (stub)\n", bEnhanced);
    return IO_UNSUPPORTED;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_Sanitize(IIOStorageDevice *pThis)
{
    printf("[Storage] Sanitize (stub)\n");
    return IO_UNSUPPORTED;
}

static IO_RETURN STDMETHODCALLTYPE
StorageDevice_SetPowerState(IIOStorageDevice *pThis, UINT32 uPowerState)
{
    printf("[Storage] SetPowerState: %u (stub)\n", uPowerState);
    return IO_UNSUPPORTED;
}

//
// IIOStorageController Implementation
//

static HRESULT STDMETHODCALLTYPE
StorageController_QueryInterface(IIOStorageController *pThis, REFIID riid, void **ppvObject)
{
    if (!ppvObject) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IIOService) ||
        IsEqualGUID(riid, &IID_IIOStorageController)) {
        *ppvObject = pThis;
        StorageController_AddRef(pThis);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE
StorageController_AddRef(IIOStorageController *pThis)
{
    STORAGE_CONTROLLER_IMPL *pImpl = (STORAGE_CONTROLLER_IMPL*)pThis;
    return ++pImpl->RefCount;
}

static ULONG STDMETHODCALLTYPE
StorageController_Release(IIOStorageController *pThis)
{
    STORAGE_CONTROLLER_IMPL *pImpl = (STORAGE_CONTROLLER_IMPL*)pThis;
    ULONG uRef = --pImpl->RefCount;

    if (uRef == 0) {
        printf("[Storage] Releasing storage controller (%s)\n",
               StorageGetProtocolName(pImpl->Protocol));

        // Release all devices
        if (pImpl->ppDevices) {
            for (UINT32 i = 0; i < pImpl->uNumDevices; i++) {
                if (pImpl->ppDevices[i]) {
                    pImpl->ppDevices[i]->lpVtbl->Release(pImpl->ppDevices[i]);
                }
            }
            free(pImpl->ppDevices);
        }

        if (pImpl->pProtocolController) {
            pImpl->pProtocolController->lpVtbl->Release(pImpl->pProtocolController);
        }

        free(pImpl);
    }

    return uRef;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_Probe(IIOStorageController *pThis, IIOService *pProvider, UINT32 *puProbeScore)
{
    printf("[Storage] StorageController_Probe: stub implementation\n");
    if (puProbeScore) {
        *puProbeScore = 5000;
    }
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_Start(IIOStorageController *pThis, IIOService *pProvider)
{
    STORAGE_CONTROLLER_IMPL *pImpl = (STORAGE_CONTROLLER_IMPL*)pThis;
    printf("[Storage] Starting storage controller (%s)\n",
           StorageGetProtocolName(pImpl->Protocol));
    pImpl->bInitialized = TRUE;
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_Stop(IIOStorageController *pThis, IIOService *pProvider)
{
    STORAGE_CONTROLLER_IMPL *pImpl = (STORAGE_CONTROLLER_IMPL*)pThis;
    printf("[Storage] Stopping storage controller (%s)\n",
           StorageGetProtocolName(pImpl->Protocol));
    pImpl->bInitialized = FALSE;
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_Terminate(IIOStorageController *pThis, UINT32 uOptions)
{
    printf("[Storage] StorageController_Terminate: stub implementation\n");
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_GetProperty(IIOStorageController *pThis, CONST CHAR8 *pszKey, VOID *pValue, UINTN *pcbSize, UINT32 *puType)
{
    STORAGE_CONTROLLER_IMPL *pImpl = (STORAGE_CONTROLLER_IMPL*)pThis;

    if (pImpl->pProtocolController) {
        return pImpl->pProtocolController->lpVtbl->GetProperty(pImpl->pProtocolController, pszKey, pValue, pcbSize, puType);
    }

    return IO_NO_MATCH;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_SetProperty(IIOStorageController *pThis, CONST CHAR8 *pszKey, CONST VOID *pValue, UINTN cbSize, UINT32 uType)
{
    STORAGE_CONTROLLER_IMPL *pImpl = (STORAGE_CONTROLLER_IMPL*)pThis;

    if (pImpl->pProtocolController) {
        return pImpl->pProtocolController->lpVtbl->SetProperty(pImpl->pProtocolController, pszKey, pValue, cbSize, uType);
    }

    return IO_UNSUPPORTED;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_GetParentService(IIOStorageController *pThis, IIOService **ppParent)
{
    STORAGE_CONTROLLER_IMPL *pImpl = (STORAGE_CONTROLLER_IMPL*)pThis;

    if (!ppParent) {
        return IO_BAD_ARGUMENT;
    }

    if (pImpl->pProtocolController) {
        return pImpl->pProtocolController->lpVtbl->GetParentService(pImpl->pProtocolController, ppParent);
    }

    *ppParent = NULL;
    return IO_NO_DEVICE;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_GetChildService(IIOStorageController *pThis, UINT32 uIndex, IIOService **ppChild)
{
    STORAGE_CONTROLLER_IMPL *pImpl = (STORAGE_CONTROLLER_IMPL*)pThis;

    if (!ppChild) {
        return IO_BAD_ARGUMENT;
    }

    if (uIndex >= pImpl->uNumDevices) {
        *ppChild = NULL;
        return IO_NO_DEVICE;
    }

    *ppChild = (IIOService*)pImpl->ppDevices[uIndex];
    if (*ppChild) {
        (*ppChild)->lpVtbl->AddRef(*ppChild);
    }

    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_GetServiceState(IIOStorageController *pThis, UINT32 *puState)
{
    STORAGE_CONTROLLER_IMPL *pImpl = (STORAGE_CONTROLLER_IMPL*)pThis;

    if (!puState) {
        return IO_BAD_ARGUMENT;
    }

    *puState = pImpl->bInitialized ? IO_SERVICE_STARTED : IO_SERVICE_INACTIVE;
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_GetServiceName(IIOStorageController *pThis, CHAR8 *pszName, UINTN cbSize)
{
    STORAGE_CONTROLLER_IMPL *pImpl = (STORAGE_CONTROLLER_IMPL*)pThis;

    if (!pszName || cbSize == 0) {
        return IO_BAD_ARGUMENT;
    }

    snprintf(pszName, cbSize, "StorageController (%s)", StorageGetProtocolName(pImpl->Protocol));
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_RegisterService(IIOStorageController *pThis, UINT32 uOptions)
{
    printf("[Storage] StorageController_RegisterService: stub implementation\n");
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_GetControllerInfo(IIOStorageController *pThis, STORAGE_CONTROLLER_INFO *pControllerInfo)
{
    STORAGE_CONTROLLER_IMPL *pImpl = (STORAGE_CONTROLLER_IMPL*)pThis;

    if (!pControllerInfo) {
        return IO_BAD_ARGUMENT;
    }

    memcpy(pControllerInfo, &pImpl->ControllerInfo, sizeof(STORAGE_CONTROLLER_INFO));
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_EnumerateDevices(IIOStorageController *pThis)
{
    printf("[Storage] EnumerateDevices (stub)\n");
    return IO_UNSUPPORTED;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_GetDeviceCount(IIOStorageController *pThis, UINT32 *puCount)
{
    STORAGE_CONTROLLER_IMPL *pImpl = (STORAGE_CONTROLLER_IMPL*)pThis;

    if (!puCount) {
        return IO_BAD_ARGUMENT;
    }

    *puCount = pImpl->uNumDevices;
    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_GetDevice(IIOStorageController *pThis, UINT32 uIndex, IIOStorageDevice **ppDevice)
{
    STORAGE_CONTROLLER_IMPL *pImpl = (STORAGE_CONTROLLER_IMPL*)pThis;

    if (!ppDevice) {
        return IO_BAD_ARGUMENT;
    }

    if (uIndex >= pImpl->uNumDevices) {
        *ppDevice = NULL;
        return IO_NO_DEVICE;
    }

    *ppDevice = pImpl->ppDevices[uIndex];
    if (*ppDevice) {
        (*ppDevice)->lpVtbl->AddRef(*ppDevice);
    }

    return IO_SUCCESS;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_ResetController(IIOStorageController *pThis)
{
    printf("[Storage] ResetController (stub)\n");
    return IO_UNSUPPORTED;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_ResetPort(IIOStorageController *pThis, UINT32 uPortIndex)
{
    printf("[Storage] ResetPort: %u (stub)\n", uPortIndex);
    return IO_UNSUPPORTED;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_GetPortStatus(IIOStorageController *pThis, UINT32 uPortIndex, UINT32 *puStatus)
{
    printf("[Storage] GetPortStatus: %u (stub)\n", uPortIndex);
    return IO_UNSUPPORTED;
}

static IO_RETURN STDMETHODCALLTYPE
StorageController_SetHotPlugEnable(IIOStorageController *pThis, BOOLEAN bEnable)
{
    printf("[Storage] SetHotPlugEnable: %d (stub)\n", bEnable);
    return IO_UNSUPPORTED;
}

//
// Public API Implementation
//

IO_RETURN
StorageInitialize(VOID)
{
    if (g_bStorageInitialized) {
        printf("[Storage] Storage family already initialized\n");
        return IO_SUCCESS;
    }

    printf("[Storage] Initializing Storage family subsystem\n");

    // TODO: Register with IOKit registry
    // TODO: Register device matching for storage protocols

    g_bStorageInitialized = TRUE;
    printf("[Storage] Storage family initialized successfully\n");

    return IO_SUCCESS;
}

IO_RETURN
StorageShutdown(VOID)
{
    if (!g_bStorageInitialized) {
        return IO_SUCCESS;
    }

    printf("[Storage] Shutting down Storage family subsystem\n");

    // TODO: Unregister from IOKit registry
    // TODO: Clean up any global resources

    g_bStorageInitialized = FALSE;
    printf("[Storage] Storage family shutdown complete\n");

    return IO_SUCCESS;
}

IO_RETURN
StorageDeviceCreate(
    IIOService *pProtocolDevice,
    STORAGE_PROTOCOL Protocol,
    IIOStorageDevice **ppDevice)
{
    STORAGE_DEVICE_IMPL *pImpl;

    if (!pProtocolDevice || !ppDevice) {
        return IO_BAD_ARGUMENT;
    }

    printf("[Storage] Creating storage device (%s)\n", StorageGetProtocolName(Protocol));

    pImpl = (STORAGE_DEVICE_IMPL*)calloc(1, sizeof(STORAGE_DEVICE_IMPL));
    if (!pImpl) {
        return IO_NO_MEMORY;
    }

    pImpl->Vtbl.lpVtbl = &g_StorageDeviceVtbl;
    pImpl->RefCount = 1;
    pImpl->Protocol = Protocol;
    pImpl->pProtocolDevice = pProtocolDevice;
    pImpl->bInitialized = FALSE;

    // Add reference to protocol device
    pProtocolDevice->lpVtbl->AddRef(pProtocolDevice);

    // Initialize device info with defaults
    memset(&pImpl->DeviceInfo, 0, sizeof(STORAGE_DEVICE_INFO));
    pImpl->DeviceInfo.Protocol = Protocol;
    pImpl->DeviceInfo.HealthPercentage = 255;  // Unknown

    // Initialize I/O statistics
    memset(&pImpl->IOStats, 0, sizeof(STORAGE_IO_STATS));

    *ppDevice = (IIOStorageDevice*)pImpl;
    printf("[Storage] Storage device created successfully\n");

    return IO_SUCCESS;
}

IO_RETURN
StorageControllerCreate(
    IIOService *pProtocolController,
    STORAGE_PROTOCOL Protocol,
    IIOStorageController **ppController)
{
    STORAGE_CONTROLLER_IMPL *pImpl;

    if (!pProtocolController || !ppController) {
        return IO_BAD_ARGUMENT;
    }

    printf("[Storage] Creating storage controller (%s)\n", StorageGetProtocolName(Protocol));

    pImpl = (STORAGE_CONTROLLER_IMPL*)calloc(1, sizeof(STORAGE_CONTROLLER_IMPL));
    if (!pImpl) {
        return IO_NO_MEMORY;
    }

    pImpl->Vtbl.lpVtbl = &g_StorageControllerVtbl;
    pImpl->RefCount = 1;
    pImpl->Protocol = Protocol;
    pImpl->pProtocolController = pProtocolController;
    pImpl->bInitialized = FALSE;
    pImpl->uNumDevices = 0;
    pImpl->uMaxDevices = 16;  // Initial capacity

    // Add reference to protocol controller
    pProtocolController->lpVtbl->AddRef(pProtocolController);

    // Allocate device array
    pImpl->ppDevices = (IIOStorageDevice**)calloc(pImpl->uMaxDevices, sizeof(IIOStorageDevice*));
    if (!pImpl->ppDevices) {
        pProtocolController->lpVtbl->Release(pProtocolController);
        free(pImpl);
        return IO_NO_MEMORY;
    }

    // Initialize controller info with defaults
    memset(&pImpl->ControllerInfo, 0, sizeof(STORAGE_CONTROLLER_INFO));
    pImpl->ControllerInfo.Protocol = Protocol;

    *ppController = (IIOStorageController*)pImpl;
    printf("[Storage] Storage controller created successfully\n");

    return IO_SUCCESS;
}

IO_RETURN
StorageDetectProtocol(
    IIOService *pDevice,
    STORAGE_PROTOCOL *pProtocol)
{
    if (!pDevice || !pProtocol) {
        return IO_BAD_ARGUMENT;
    }

    printf("[Storage] Detecting storage protocol (stub)\n");

    // TODO: Query device properties to determine protocol
    // For now, return unknown
    *pProtocol = STORAGE_PROTOCOL_UNKNOWN;

    return IO_NO_MATCH;
}
