/**
 * @file pcie_impl.c
 * @brief PCIe Driver Implementation - VTable Methods
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/pcie.h>
#include <ananke/ntrtl.h>
#include <string.h>
#include <stdio.h>

// Include device structure from pcie.c
typedef struct _PCI_DEVICE_IMPL PCI_DEVICE_IMPL;

/**
 * @brief Get device information
 */
static IO_RETURN STDMETHODCALLTYPE
PCIDevice_GetDeviceInfo(
    IIOPCIDevice *pThis,
    PCI_DEVICE_INFO *pDeviceInfo
    )
{
    PCI_DEVICE_IMPL *pImpl = (PCI_DEVICE_IMPL *)pThis;

    if (pDeviceInfo == NULL) {
        return IO_BAD_ARGUMENT;
    }

    memcpy(pDeviceInfo, &pImpl->DeviceInfo, sizeof(PCI_DEVICE_INFO));
    return IO_SUCCESS;
}

/**
 * @brief Read from configuration space
 */
static IO_RETURN STDMETHODCALLTYPE
PCIDevice_ConfigRead(
    IIOPCIDevice *pThis,
    UINT32 uOffset,
    UINT32 uSize,
    UINT32 *puValue
    )
{
    PCI_DEVICE_IMPL *pImpl = (PCI_DEVICE_IMPL *)pThis;

    if (puValue == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Use appropriate configuration mechanism
    switch (pImpl->ConfigMechanism) {
        case PCI_CONFIG_MECHANISM_1:
            return PCIConfigReadMechanism1(&pImpl->DeviceInfo.Location, uOffset, uSize, puValue);

        case PCI_CONFIG_ECAM:
            return PCIConfigReadECAM(&pImpl->DeviceInfo.Location, uOffset, uSize, puValue);

        default:
            return IO_UNSUPPORTED;
    }
}

/**
 * @brief Write to configuration space
 */
static IO_RETURN STDMETHODCALLTYPE
PCIDevice_ConfigWrite(
    IIOPCIDevice *pThis,
    UINT32 uOffset,
    UINT32 uSize,
    UINT32 uValue
    )
{
    PCI_DEVICE_IMPL *pImpl = (PCI_DEVICE_IMPL *)pThis;

    // Use appropriate configuration mechanism
    switch (pImpl->ConfigMechanism) {
        case PCI_CONFIG_MECHANISM_1:
            return PCIConfigWriteMechanism1(&pImpl->DeviceInfo.Location, uOffset, uSize, uValue);

        case PCI_CONFIG_ECAM:
            return PCIConfigWriteECAM(&pImpl->DeviceInfo.Location, uOffset, uSize, uValue);

        default:
            return IO_UNSUPPORTED;
    }
}

/**
 * @brief Map BAR into memory
 */
static IO_RETURN STDMETHODCALLTYPE
PCIDevice_MapBAR(
    IIOPCIDevice *pThis,
    UINT32 uBARIndex,
    VOID **ppAddress,
    UINT64 *pcbSize
    )
{
    PCI_DEVICE_IMPL *pImpl = (PCI_DEVICE_IMPL *)pThis;
    PCI_BAR *pBAR;

    if (uBARIndex >= PCI_MAX_BARS || ppAddress == NULL || pcbSize == NULL) {
        return IO_BAD_ARGUMENT;
    }

    pBAR = &pImpl->DeviceInfo.BARs[uBARIndex];

    // Check if BAR is valid
    if (pBAR->Size == 0) {
        return IO_NO_DEVICE;
    }

    // I/O space cannot be mapped
    if (!pBAR->bIsMem) {
        return IO_UNSUPPORTED;
    }

    // Check if already mapped
    if (pImpl->pMappedBARs[uBARIndex] != NULL) {
        *ppAddress = pImpl->pMappedBARs[uBARIndex];
        *pcbSize = pBAR->Size;
        return IO_SUCCESS;
    }

    // TODO: Implement actual memory mapping through MMU
    // For now, use identity mapping (assumes physical = virtual)
    pImpl->pMappedBARs[uBARIndex] = (VOID *)pBAR->PhysicalAddress;
    *ppAddress = pImpl->pMappedBARs[uBARIndex];
    *pcbSize = pBAR->Size;

    printf("PCI: Mapped BAR%u at 0x%llX (size=0x%llX)\n",
           uBARIndex, pBAR->PhysicalAddress, pBAR->Size);

    return IO_SUCCESS;
}

/**
 * @brief Unmap BAR from memory
 */
static IO_RETURN STDMETHODCALLTYPE
PCIDevice_UnmapBAR(
    IIOPCIDevice *pThis,
    UINT32 uBARIndex
    )
{
    PCI_DEVICE_IMPL *pImpl = (PCI_DEVICE_IMPL *)pThis;

    if (uBARIndex >= PCI_MAX_BARS) {
        return IO_BAD_ARGUMENT;
    }

    if (pImpl->pMappedBARs[uBARIndex] == NULL) {
        return IO_NOT_READY;
    }

    // TODO: Implement actual memory unmapping
    pImpl->pMappedBARs[uBARIndex] = NULL;

    printf("PCI: Unmapped BAR%u\n", uBARIndex);
    return IO_SUCCESS;
}

/**
 * @brief Enable/disable bus mastering
 */
static IO_RETURN STDMETHODCALLTYPE
PCIDevice_SetBusMaster(
    IIOPCIDevice *pThis,
    BOOLEAN bEnable
    )
{
    UINT32 uCommand;
    IO_RETURN Status;

    // Read current command register
    Status = PCIDevice_ConfigRead(pThis, PCI_CFG_COMMAND, 2, &uCommand);
    if (Status != IO_SUCCESS) {
        return Status;
    }

    // Modify bus master bit
    if (bEnable) {
        uCommand |= PCI_CMD_BUS_MASTER;
    } else {
        uCommand &= ~PCI_CMD_BUS_MASTER;
    }

    // Write back
    Status = PCIDevice_ConfigWrite(pThis, PCI_CFG_COMMAND, 2, uCommand);
    if (Status != IO_SUCCESS) {
        return Status;
    }

    printf("PCI: Bus mastering %s\n", bEnable ? "enabled" : "disabled");
    return IO_SUCCESS;
}

/**
 * @brief Enable/disable memory and I/O space
 */
static IO_RETURN STDMETHODCALLTYPE
PCIDevice_SetMemoryIOEnable(
    IIOPCIDevice *pThis,
    BOOLEAN bMemory,
    BOOLEAN bIO
    )
{
    UINT32 uCommand;
    IO_RETURN Status;

    // Read current command register
    Status = PCIDevice_ConfigRead(pThis, PCI_CFG_COMMAND, 2, &uCommand);
    if (Status != IO_SUCCESS) {
        return Status;
    }

    // Modify enable bits
    if (bMemory) {
        uCommand |= PCI_CMD_MEMORY_SPACE;
    } else {
        uCommand &= ~PCI_CMD_MEMORY_SPACE;
    }

    if (bIO) {
        uCommand |= PCI_CMD_IO_SPACE;
    } else {
        uCommand &= ~PCI_CMD_IO_SPACE;
    }

    // Write back
    Status = PCIDevice_ConfigWrite(pThis, PCI_CFG_COMMAND, 2, uCommand);
    if (Status != IO_SUCCESS) {
        return Status;
    }

    printf("PCI: Memory space %s, I/O space %s\n",
           bMemory ? "enabled" : "disabled",
           bIO ? "enabled" : "disabled");

    return IO_SUCCESS;
}

/**
 * @brief Find capability in capability list
 */
static IO_RETURN STDMETHODCALLTYPE
PCIDevice_FindCapability(
    IIOPCIDevice *pThis,
    UINT8 uCapabilityID,
    UINT32 *puOffset
    )
{
    UINT32 uStatus, uCapPtr;
    UINT32 uCapID, uNextPtr;
    IO_RETURN Status;
    UINT32 uIterations = 0;

    if (puOffset == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Check if device has capability list
    Status = PCIDevice_ConfigRead(pThis, PCI_CFG_STATUS, 2, &uStatus);
    if (Status != IO_SUCCESS) {
        return Status;
    }

    if (!(uStatus & PCI_STATUS_CAP_LIST)) {
        return IO_NO_MATCH;
    }

    // Get capabilities pointer
    Status = PCIDevice_ConfigRead(pThis, PCI_CFG_CAPABILITIES_PTR, 1, &uCapPtr);
    if (Status != IO_SUCCESS) {
        return Status;
    }

    uCapPtr &= 0xFC;  // Align to 4-byte boundary

    // Walk capability list
    while (uCapPtr != 0 && uIterations < 48) {
        // Read capability ID and next pointer
        Status = PCIDevice_ConfigRead(pThis, uCapPtr, 1, &uCapID);
        if (Status != IO_SUCCESS) {
            return Status;
        }

        Status = PCIDevice_ConfigRead(pThis, uCapPtr + 1, 1, &uNextPtr);
        if (Status != IO_SUCCESS) {
            return Status;
        }

        // Check if this is the capability we're looking for
        if ((UINT8)uCapID == uCapabilityID) {
            *puOffset = uCapPtr;
            return IO_SUCCESS;
        }

        // Move to next capability
        uCapPtr = uNextPtr & 0xFC;
        uIterations++;
    }

    return IO_NO_MATCH;
}

/**
 * @brief Setup MSI interrupts
 */
static IO_RETURN STDMETHODCALLTYPE
PCIDevice_SetupMSI(
    IIOPCIDevice *pThis,
    UINT32 uNumVectors,
    VOID (*pfnHandler)(VOID *pContext, UINT32 uVector),
    VOID *pContext
    )
{
    PCI_DEVICE_IMPL *pImpl = (PCI_DEVICE_IMPL *)pThis;
    UINT32 uMSIOffset;
    UINT32 uMessageControl;
    IO_RETURN Status;

    // Find MSI capability
    Status = PCIDevice_FindCapability(pThis, PCI_CAP_ID_MSI, &uMSIOffset);
    if (Status != IO_SUCCESS) {
        return IO_UNSUPPORTED;
    }

    // Read message control register
    Status = PCIDevice_ConfigRead(pThis, uMSIOffset + 2, 2, &uMessageControl);
    if (Status != IO_SUCCESS) {
        return Status;
    }

    // Check maximum vectors supported
    UINT32 uMaxVectors = 1 << ((uMessageControl >> 1) & 0x7);
    if (uNumVectors > uMaxVectors) {
        return IO_NO_RESOURCES;
    }

    // TODO: Allocate MSI vector and configure message address/data
    // This requires interaction with the interrupt controller (APIC/MSI-X)

    pImpl->bMSIEnabled = TRUE;
    printf("PCI: MSI enabled with %u vector(s)\n", uNumVectors);

    return IO_SUCCESS;
}

/**
 * @brief Setup MSI-X interrupts
 */
static IO_RETURN STDMETHODCALLTYPE
PCIDevice_SetupMSIX(
    IIOPCIDevice *pThis,
    UINT32 uNumVectors,
    VOID (*pfnHandler)(VOID *pContext, UINT32 uVector),
    VOID *pContext
    )
{
    PCI_DEVICE_IMPL *pImpl = (PCI_DEVICE_IMPL *)pThis;
    UINT32 uMSIXOffset;
    UINT32 uMessageControl;
    IO_RETURN Status;

    // Find MSI-X capability
    Status = PCIDevice_FindCapability(pThis, PCI_CAP_ID_MSIX, &uMSIXOffset);
    if (Status != IO_SUCCESS) {
        return IO_UNSUPPORTED;
    }

    // Read message control register
    Status = PCIDevice_ConfigRead(pThis, uMSIXOffset + 2, 2, &uMessageControl);
    if (Status != IO_SUCCESS) {
        return Status;
    }

    // Get table size
    UINT32 uTableSize = (uMessageControl & 0x7FF) + 1;
    if (uNumVectors > uTableSize) {
        return IO_NO_RESOURCES;
    }

    // TODO: Map MSI-X table BAR and configure vectors
    // This requires mapping the MSI-X table and PBA

    pImpl->bMSIXEnabled = TRUE;
    printf("PCI: MSI-X enabled with %u vector(s) (table size=%u)\n",
           uNumVectors, uTableSize);

    return IO_SUCCESS;
}

/**
 * @brief Initialize PCI subsystem
 *
 * @retval IO_SUCCESS   Subsystem initialized successfully
 */
IO_RETURN
PCIInitialize(
    VOID
    )
{
    IO_RETURN Status;

    if (g_PCISubsystem.bInitialized) {
        return IO_SUCCESS;
    }

    printf("PCI: Initializing PCI/PCI-X/PCIe subsystem...\n");

    // Detect configuration mechanism
    Status = PCIDetectConfigMechanism();
    if (Status != IO_SUCCESS) {
        printf("PCI: Failed to detect configuration mechanism\n");
        return Status;
    }

    g_PCISubsystem.bInitialized = TRUE;
    printf("PCI: Subsystem initialized successfully\n");

    return IO_SUCCESS;
}

/**
 * @brief Scan PCI bus for devices
 *
 * @param uBusNumber    Bus number to scan
 * @param ppDevices     Receives array of discovered devices
 * @param puDeviceCount On input: max devices; On output: actual count
 *
 * @retval IO_SUCCESS   Scan successful
 */
IO_RETURN
PCIScanBus(
    UINT8 uBusNumber,
    IIOPCIDevice **ppDevices,
    UINT32 *puDeviceCount
    )
{
    UINT32 uDevice, uFunction;
    UINT32 uMaxDevices;
    UINT32 uFoundDevices = 0;
    PCI_LOCATION Location;

    if (ppDevices == NULL || puDeviceCount == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!g_PCISubsystem.bInitialized) {
        return IO_NOT_READY;
    }

    uMaxDevices = *puDeviceCount;
    *puDeviceCount = 0;

    printf("PCI: Scanning bus %u...\n", uBusNumber);

    Location.Segment = 0;
    Location.Bus = uBusNumber;

    // Scan all devices
    for (uDevice = 0; uDevice < PCI_MAX_DEVICES; uDevice++) {
        Location.Device = (UINT8)uDevice;
        Location.Function = 0;

        // Check if device exists
        IIOPCIDevice *pDevice;
        IO_RETURN Status = PCIDeviceCreate(&Location, &pDevice);

        if (Status == IO_SUCCESS) {
            if (uFoundDevices < uMaxDevices) {
                ppDevices[uFoundDevices] = pDevice;
                uFoundDevices++;
            } else {
                IIOPCIDevice_Release(pDevice);
            }

            // Check for multi-function device
            PCI_DEVICE_INFO DeviceInfo;
            IIOPCIDevice_GetDeviceInfo(pDevice, &DeviceInfo);

            if (DeviceInfo.HeaderType & PCI_HEADER_TYPE_MULTIFUNCTION) {
                // Scan remaining functions
                for (uFunction = 1; uFunction < PCI_MAX_FUNCTIONS; uFunction++) {
                    Location.Function = (UINT8)uFunction;

                    Status = PCIDeviceCreate(&Location, &pDevice);
                    if (Status == IO_SUCCESS) {
                        if (uFoundDevices < uMaxDevices) {
                            ppDevices[uFoundDevices] = pDevice;
                            uFoundDevices++;
                        } else {
                            IIOPCIDevice_Release(pDevice);
                        }
                    }
                }
            }
        }
    }

    *puDeviceCount = uFoundDevices;
    printf("PCI: Found %u device(s) on bus %u\n", uFoundDevices, uBusNumber);

    return IO_SUCCESS;
}

/**
 * @brief Create a PCI device instance
 *
 * @param pLocation     PCI device location
 * @param ppDevice      Receives pointer to device interface
 *
 * @retval IO_SUCCESS   Device created successfully
 * @retval IO_NO_MEMORY Insufficient memory
 */
IO_RETURN
PCIDeviceCreate(
    CONST PCI_LOCATION *pLocation,
    IIOPCIDevice **ppDevice
    )
{
    PCI_DEVICE_IMPL *pImpl;
    IO_RETURN Status;

    if (pLocation == NULL || ppDevice == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Allocate device instance
    pImpl = (PCI_DEVICE_IMPL *)malloc(sizeof(PCI_DEVICE_IMPL));
    if (pImpl == NULL) {
        return IO_NO_MEMORY;
    }

    memset(pImpl, 0, sizeof(PCI_DEVICE_IMPL));

    // Initialize device location
    memcpy(&pImpl->DeviceInfo.Location, pLocation, sizeof(PCI_LOCATION));

    // Use global configuration mechanism
    pImpl->ConfigMechanism = g_PCISubsystem.ConfigMechanism;

    // Create underlying service
    CHAR8 szName[64];
    snprintf(szName, sizeof(szName), "PCIDevice_%02X_%02X_%X",
             pLocation->Bus, pLocation->Device, pLocation->Function);

    Status = IOServiceCreate(szName, &pImpl->pService);
    if (Status != IO_SUCCESS) {
        free(pImpl);
        return Status;
    }

    // Probe device
    Status = PCIProbeDevice(pImpl);
    if (Status != IO_SUCCESS) {
        IIOService_Release(pImpl->pService);
        free(pImpl);
        return Status;
    }

    // Set up vtable
    pImpl->Vtbl.lpVtbl = &g_PCIDeviceVtbl;

    // Publish device properties
    IIOService_SetProperty(pImpl->pService, "pci-vendor-id",
                          &pImpl->DeviceInfo.VendorID,
                          sizeof(pImpl->DeviceInfo.VendorID),
                          IO_PROPERTY_TYPE_NUMBER);

    IIOService_SetProperty(pImpl->pService, "pci-device-id",
                          &pImpl->DeviceInfo.DeviceID,
                          sizeof(pImpl->DeviceInfo.DeviceID),
                          IO_PROPERTY_TYPE_NUMBER);

    IIOService_SetProperty(pImpl->pService, "pci-class-code",
                          &pImpl->DeviceInfo.ClassCode,
                          sizeof(pImpl->DeviceInfo.ClassCode),
                          IO_PROPERTY_TYPE_NUMBER);

    IIOService_SetProperty(pImpl->pService, "pci-subclass",
                          &pImpl->DeviceInfo.SubClass,
                          sizeof(pImpl->DeviceInfo.SubClass),
                          IO_PROPERTY_TYPE_NUMBER);

    *ppDevice = (IIOPCIDevice *)pImpl;
    g_PCISubsystem.uDeviceCount++;

    return IO_SUCCESS;
}
