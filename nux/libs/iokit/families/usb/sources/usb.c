/**
 * @file usb.c
 * @brief USB Family Implementation - USB 1.x/2.0/3.x/4.0 Host Controller Driver
 *
 * Implements support for:
 * - UHCI (USB 1.x, Intel)
 * - OHCI (USB 1.x, Compaq/MS/NatSemi)
 * - EHCI (USB 2.0)
 * - xHCI (USB 3.x/4.0)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/usb/usb.h>
#include <iokit/families/pcie/pcie.h>
#include <ananke/ntrtl.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief Known USB Controller IDs
 */
typedef struct _USB_CONTROLLER_ID {
    UINT16              VendorID;
    UINT16              DeviceID;
    USB_CONTROLLER_TYPE Type;
    USB_VERSION         Version;
    CONST CHAR8        *pszName;
} USB_CONTROLLER_ID;

/**
 * @brief USB Controller Database
 */
static CONST USB_CONTROLLER_ID g_USBControllers[] = {
    // Intel UHCI Controllers (USB 1.1)
    { 0x8086, 0x2830, USB_CONTROLLER_UHCI, USB_VERSION_1_1, "Intel 82801H UHCI" },
    { 0x8086, 0x2831, USB_CONTROLLER_UHCI, USB_VERSION_1_1, "Intel 82801H UHCI" },
    { 0x8086, 0x2832, USB_CONTROLLER_UHCI, USB_VERSION_1_1, "Intel 82801H UHCI" },
    { 0x8086, 0x27C8, USB_CONTROLLER_UHCI, USB_VERSION_1_1, "Intel 82801G UHCI" },
    { 0x8086, 0x27C9, USB_CONTROLLER_UHCI, USB_VERSION_1_1, "Intel 82801G UHCI" },
    { 0x8086, 0x27CA, USB_CONTROLLER_UHCI, USB_VERSION_1_1, "Intel 82801G UHCI" },
    { 0x8086, 0x27CB, USB_CONTROLLER_UHCI, USB_VERSION_1_1, "Intel 82801G UHCI" },

    // Intel EHCI Controllers (USB 2.0)
    { 0x8086, 0x27CC, USB_CONTROLLER_EHCI, USB_VERSION_2_0, "Intel 82801G EHCI" },
    { 0x8086, 0x2836, USB_CONTROLLER_EHCI, USB_VERSION_2_0, "Intel 82801H EHCI" },
    { 0x8086, 0x293A, USB_CONTROLLER_EHCI, USB_VERSION_2_0, "Intel 82801I EHCI" },
    { 0x8086, 0x3A3A, USB_CONTROLLER_EHCI, USB_VERSION_2_0, "Intel 82801JI EHCI" },
    { 0x8086, 0x3A3C, USB_CONTROLLER_EHCI, USB_VERSION_2_0, "Intel 82801JI EHCI" },

    // Intel xHCI Controllers (USB 3.x)
    { 0x8086, 0x1E31, USB_CONTROLLER_XHCI, USB_VERSION_3_0, "Intel 7 Series xHCI" },
    { 0x8086, 0x8C31, USB_CONTROLLER_XHCI, USB_VERSION_3_0, "Intel 8 Series xHCI" },
    { 0x8086, 0x9C31, USB_CONTROLLER_XHCI, USB_VERSION_3_0, "Intel 8 Series LP xHCI" },
    { 0x8086, 0xA12F, USB_CONTROLLER_XHCI, USB_VERSION_3_1, "Intel 100 Series xHCI" },
    { 0x8086, 0xA2AF, USB_CONTROLLER_XHCI, USB_VERSION_3_1, "Intel 200 Series xHCI" },
    { 0x8086, 0xA36D, USB_CONTROLLER_XHCI, USB_VERSION_3_1, "Intel 300 Series xHCI" },
    { 0x8086, 0x02ED, USB_CONTROLLER_XHCI, USB_VERSION_3_2, "Intel 400 Series xHCI" },
    { 0x8086, 0x43ED, USB_CONTROLLER_XHCI, USB_VERSION_3_2, "Intel 500 Series xHCI" },
    { 0x8086, 0x51ED, USB_CONTROLLER_XHCI, USB_VERSION_3_2, "Intel 600 Series xHCI" },
    { 0x8086, 0x7AE0, USB_CONTROLLER_XHCI, USB_VERSION_4_0, "Intel 700 Series xHCI (USB4)" },
    { 0x8086, 0x7A60, USB_CONTROLLER_XHCI, USB_VERSION_4_0, "Intel Meteor Lake xHCI (USB4)" },

    // AMD OHCI Controllers (USB 1.1)
    { 0x1022, 0x7464, USB_CONTROLLER_OHCI, USB_VERSION_1_1, "AMD 756 OHCI" },
    { 0x1022, 0x7469, USB_CONTROLLER_OHCI, USB_VERSION_1_1, "AMD 766 OHCI" },
    { 0x1022, 0x740C, USB_CONTROLLER_OHCI, USB_VERSION_1_1, "AMD 755 OHCI" },

    // AMD EHCI Controllers (USB 2.0)
    { 0x1022, 0x7808, USB_CONTROLLER_EHCI, USB_VERSION_2_0, "AMD FCH EHCI" },
    { 0x1022, 0x7809, USB_CONTROLLER_EHCI, USB_VERSION_2_0, "AMD FCH EHCI" },

    // AMD xHCI Controllers (USB 3.x)
    { 0x1022, 0x7814, USB_CONTROLLER_XHCI, USB_VERSION_3_0, "AMD FCH xHCI" },
    { 0x1022, 0x43D0, USB_CONTROLLER_XHCI, USB_VERSION_3_1, "AMD 400 Series xHCI" },
    { 0x1022, 0x43D5, USB_CONTROLLER_XHCI, USB_VERSION_3_1, "AMD 400 Series xHCI" },
    { 0x1022, 0x15E0, USB_CONTROLLER_XHCI, USB_VERSION_3_2, "AMD Ryzen xHCI" },
    { 0x1022, 0x15E1, USB_CONTROLLER_XHCI, USB_VERSION_3_2, "AMD Ryzen xHCI" },

    // VIA UHCI Controllers (USB 1.1)
    { 0x1106, 0x3038, USB_CONTROLLER_UHCI, USB_VERSION_1_1, "VIA VT82xxxxx UHCI" },
    { 0x1106, 0x3104, USB_CONTROLLER_UHCI, USB_VERSION_1_1, "VIA VT6202 UHCI" },

    // VIA EHCI Controllers (USB 2.0)
    { 0x1106, 0x3104, USB_CONTROLLER_EHCI, USB_VERSION_2_0, "VIA VT6202 EHCI" },

    // NEC/Renesas xHCI Controllers (USB 3.0)
    { 0x1033, 0x0194, USB_CONTROLLER_XHCI, USB_VERSION_3_0, "NEC/Renesas uPD720200 xHCI" },
    { 0x1033, 0x0195, USB_CONTROLLER_XHCI, USB_VERSION_3_0, "NEC/Renesas uPD720200 xHCI" },

    // Terminus
};

#define USB_CONTROLLER_COUNT (sizeof(g_USBControllers) / sizeof(g_USBControllers[0]))

/**
 * @brief USB Controller implementation structure
 */
typedef struct _USB_CONTROLLER_IMPL {
    IIOUSBController    Interface;
    volatile LONG       RefCount;
    IIOService         *pService;
    IIOPCIDevice       *pPCIDevice;
    USB_CONTROLLER_INFO ControllerInfo;
    UINT64              MMIOBase;
    UINT32              MMIOSize;
    BOOLEAN             bInitialized;
} USB_CONTROLLER_IMPL;

// Forward declarations
static IO_RETURN STDMETHODCALLTYPE USBController_GetControllerInfo(
    IIOUSBController *This, USB_CONTROLLER_INFO *pInfo);
static IO_RETURN STDMETHODCALLTYPE USBController_EnumerateDevices(
    IIOUSBController *This, IIOUSBDevice **ppDevices, UINT32 *puCount);
static IO_RETURN STDMETHODCALLTYPE USBController_ResetController(
    IIOUSBController *This);
static IO_RETURN STDMETHODCALLTYPE USBController_Start(
    IIOService *This, IIOService *pProvider);

/**
 * @brief USB Controller vtable
 */
static IIOUSBControllerVtbl g_USBControllerVtbl = {
    // IUnknown methods
    (void*)IIOService_QueryInterface_Impl,
    (void*)IIOService_AddRef_Impl,
    (void*)IIOService_Release_Impl,

    // IIOService methods
    (void*)IIOService_Probe_Impl,
    USBController_Start,
    (void*)IIOService_Stop_Impl,
    (void*)IIOService_Terminate_Impl,
    (void*)IIOService_GetServiceName_Impl,
    (void*)IIOService_SetProperty_Impl,
    (void*)IIOService_GetProperty_Impl,
    (void*)IIOService_GetServiceState_Impl,

    // IIOUSBController methods
    USBController_GetControllerInfo,
    USBController_EnumerateDevices,
    USBController_ResetController,
    (void*)0, // GetPortStatus
    (void*)0, // ResetPort
    (void*)0, // SetPortPower
    (void*)0, // SubmitTransfer
    (void*)0, // AbortTransfers
    (void*)0, // SetPowerProfile
};

/**
 * @brief USB Controller Start method
 */
static IO_RETURN STDMETHODCALLTYPE
USBController_Start(
    IIOService         *This,
    IIOService         *pProvider
    )
{
    USB_CONTROLLER_IMPL *pController = CONTAINING_RECORD(This, USB_CONTROLLER_IMPL, Interface);
    IO_RETURN Status;
    PCI_DEVICE_INFO PCIInfo;
    PCI_BAR BAR0;
    CONST USB_CONTROLLER_ID *pCtrlID = NULL;

    // Get PCI device interface
    Status = IIOService_QueryInterface(pProvider, &IID_IIOPCIDevice, (VOID**)&pController->pPCIDevice);
    if (Status != IO_SUCCESS) {
        return Status;
    }

    // Get PCI device info
    Status = IIOPCIDevice_GetDeviceInfo(pController->pPCIDevice, &PCIInfo);
    if (Status != IO_SUCCESS) {
        IIOPCIDevice_Release(pController->pPCIDevice);
        return Status;
    }

    // Detect controller type
    for (UINT32 i = 0; i < USB_CONTROLLER_COUNT; i++) {
        if (g_USBControllers[i].VendorID == PCIInfo.VendorID &&
            g_USBControllers[i].DeviceID == PCIInfo.DeviceID) {
            pCtrlID = &g_USBControllers[i];
            break;
        }
    }

    if (!pCtrlID) {
        // Unknown controller - try to detect by class code
        if (PCIInfo.ClassCode == 0x0C && PCIInfo.SubClass == 0x03) {
            // Serial bus controller, USB
            switch (PCIInfo.ProgIF) {
                case 0x00: pController->ControllerInfo.Type = USB_CONTROLLER_UHCI; break;
                case 0x10: pController->ControllerInfo.Type = USB_CONTROLLER_OHCI; break;
                case 0x20: pController->ControllerInfo.Type = USB_CONTROLLER_EHCI; break;
                case 0x30: pController->ControllerInfo.Type = USB_CONTROLLER_XHCI; break;
                default:
                    IIOPCIDevice_Release(pController->pPCIDevice);
                    return IO_ERR_UNSUPPORTED;
            }
            pController->ControllerInfo.Version = USB_VERSION_UNKNOWN;
            snprintf(pController->ControllerInfo.ControllerName, sizeof(pController->ControllerInfo.ControllerName),
                     "Unknown USB Controller %04X:%04X", PCIInfo.VendorID, PCIInfo.DeviceID);
        } else {
            IIOPCIDevice_Release(pController->pPCIDevice);
            return IO_ERR_UNSUPPORTED;
        }
    } else {
        pController->ControllerInfo.Type = pCtrlID->Type;
        pController->ControllerInfo.Version = pCtrlID->Version;
        strncpy(pController->ControllerInfo.ControllerName, pCtrlID->pszName,
                sizeof(pController->ControllerInfo.ControllerName) - 1);
    }

    pController->ControllerInfo.VendorID = PCIInfo.VendorID;
    pController->ControllerInfo.DeviceID = PCIInfo.DeviceID;

    // Read BAR0 (MMIO base for xHCI/EHCI, I/O base for UHCI/OHCI)
    Status = IIOPCIDevice_GetBAR(pController->pPCIDevice, 0, &BAR0);
    if (Status != IO_SUCCESS) {
        IIOPCIDevice_Release(pController->pPCIDevice);
        return Status;
    }

    pController->MMIOBase = BAR0.PhysicalAddress;
    pController->MMIOSize = (UINT32)BAR0.Size;
    pController->ControllerInfo.MMIOBase = BAR0.PhysicalAddress;
    pController->ControllerInfo.MMIOSize = (UINT32)BAR0.Size;

    // Set capabilities based on controller type
    switch (pController->ControllerInfo.Type) {
        case USB_CONTROLLER_UHCI:
        case USB_CONTROLLER_OHCI:
            pController->ControllerInfo.Capabilities =
                USB_CAP_LOW_SPEED | USB_CAP_FULL_SPEED;
            pController->ControllerInfo.NumPorts = 2;
            break;

        case USB_CONTROLLER_EHCI:
            pController->ControllerInfo.Capabilities =
                USB_CAP_LOW_SPEED | USB_CAP_FULL_SPEED | USB_CAP_HIGH_SPEED;
            pController->ControllerInfo.NumPorts = 6;
            break;

        case USB_CONTROLLER_XHCI:
            pController->ControllerInfo.Capabilities =
                USB_CAP_LOW_SPEED | USB_CAP_FULL_SPEED | USB_CAP_HIGH_SPEED |
                USB_CAP_SUPER_SPEED;

            // Add advanced capabilities for USB 3.1+
            if (pController->ControllerInfo.Version >= USB_VERSION_3_1) {
                pController->ControllerInfo.Capabilities |=
                    USB_CAP_SUPER_SPEED_PLUS | USB_CAP_POWER_DELIVERY |
                    USB_CAP_LPM | USB_CAP_STREAMS | USB_CAP_BURST;
            }

            // USB4 capabilities
            if (pController->ControllerInfo.Version >= USB_VERSION_4_0) {
                pController->ControllerInfo.Capabilities |=
                    USB_CAP_USB4 | USB_CAP_DISPLAYPORT | USB_CAP_THUNDERBOLT;
            }

            pController->ControllerInfo.NumPorts = 10;
            break;
    }

    // Enable bus mastering
    Status = IIOPCIDevice_EnableBusMaster(pController->pPCIDevice, TRUE);
    if (Status != IO_SUCCESS) {
        IIOPCIDevice_Release(pController->pPCIDevice);
        return Status;
    }

    pController->bInitialized = TRUE;

    // Print controller information
    printf("USB:       %s\n", pController->ControllerInfo.ControllerName);
    printf("USB:       Type: ");
    switch (pController->ControllerInfo.Type) {
        case USB_CONTROLLER_UHCI: printf("UHCI (USB 1.1)\n"); break;
        case USB_CONTROLLER_OHCI: printf("OHCI (USB 1.1)\n"); break;
        case USB_CONTROLLER_EHCI: printf("EHCI (USB 2.0)\n"); break;
        case USB_CONTROLLER_XHCI: printf("xHCI (USB 3.x/4.0)\n"); break;
        default: printf("Unknown\n");
    }
    printf("USB:       Ports: %u\n", pController->ControllerInfo.NumPorts);
    printf("USB:       MMIO: 0x%016llX (size: 0x%X)\n",
           (unsigned long long)pController->MMIOBase, pController->MMIOSize);
    printf("USB:       Capabilities: 0x%08X\n", pController->ControllerInfo.Capabilities);

    return IO_SUCCESS;
}

/**
 * @brief Get controller information
 */
static IO_RETURN STDMETHODCALLTYPE
USBController_GetControllerInfo(
    IIOUSBController   *This,
    USB_CONTROLLER_INFO *pInfo
    )
{
    USB_CONTROLLER_IMPL *pController = CONTAINING_RECORD(This, USB_CONTROLLER_IMPL, Interface);

    if (!pInfo) {
        return IO_ERR_INVALID_PARAM;
    }

    memcpy(pInfo, &pController->ControllerInfo, sizeof(USB_CONTROLLER_INFO));
    return IO_SUCCESS;
}

/**
 * @brief Enumerate USB devices (stub)
 */
static IO_RETURN STDMETHODCALLTYPE
USBController_EnumerateDevices(
    IIOUSBController *This,
    IIOUSBDevice    **ppDevices,
    UINT32           *puCount
    )
{
    if (!ppDevices || !puCount) {
        return IO_ERR_INVALID_PARAM;
    }

    // TODO: Implement device enumeration
    *puCount = 0;
    return IO_SUCCESS;
}

/**
 * @brief Reset controller (stub)
 */
static IO_RETURN STDMETHODCALLTYPE
USBController_ResetController(
    IIOUSBController *This
    )
{
    // TODO: Implement controller reset
    return IO_SUCCESS;
}

/**
 * @brief Initialize USB subsystem
 */
IO_RETURN
USBInitialize(
    VOID
    )
{
    printf("USB: Subsystem initializing...\n");
    return IO_SUCCESS;
}

/**
 * @brief Shutdown USB subsystem
 */
IO_RETURN
USBShutdown(
    VOID
    )
{
    printf("USB: Subsystem shutting down...\n");
    return IO_SUCCESS;
}

/**
 * @brief Create a USB controller instance
 */
IO_RETURN
IOUSBControllerCreate(
    CONST CHAR8        *pszName,
    IIOUSBController  **ppController
    )
{
    USB_CONTROLLER_IMPL *pController;
    IO_RETURN Status;

    if (!pszName || !ppController) {
        return IO_ERR_INVALID_PARAM;
    }

    // Allocate controller structure
    pController = (USB_CONTROLLER_IMPL*)malloc(sizeof(USB_CONTROLLER_IMPL));
    if (!pController) {
        return IO_ERR_NO_MEMORY;
    }

    memset(pController, 0, sizeof(USB_CONTROLLER_IMPL));
    pController->Interface.lpVtbl = &g_USBControllerVtbl;
    pController->RefCount = 1;

    // Create base service
    Status = IOServiceCreate(pszName, &pController->pService);
    if (Status != IO_SUCCESS) {
        free(pController);
        return Status;
    }

    *ppController = &pController->Interface;
    return IO_SUCCESS;
}

/**
 * @brief Create a USB device instance (stub)
 */
IO_RETURN
IOUSBDeviceCreate(
    CONST CHAR8    *pszName,
    IIOUSBDevice  **ppDevice
    )
{
    if (!pszName || !ppDevice) {
        return IO_ERR_INVALID_PARAM;
    }

    // TODO: Implement device creation
    return IO_ERR_NOT_IMPLEMENTED;
}
