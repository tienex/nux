/**
 * @file pcie_thunderbolt_example.c
 * @brief PCIe and Thunderbolt Driver Usage Example
 *
 * This example demonstrates:
 * - PCIe bus initialization and device enumeration
 * - PCI configuration space access
 * - BAR mapping and device initialization
 * - Thunderbolt controller detection
 * - Thunderbolt device management
 * - Hot-plug event handling
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/pcie.h>
#include <iokit/families/thunderbolt.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Example 1: Initialize PCIe subsystem and enumerate devices
 */
static int
Example_PCIe_Initialization(void)
{
    IO_RETURN Status;
    IIOPCIDevice *pDevices[256];
    UINT32 uDeviceCount;
    UINT32 i;

    printf("\n");
    printf("========================================\n");
    printf("  Example 1: PCIe Initialization\n");
    printf("========================================\n\n");

    // Initialize PCI subsystem
    printf("Initializing PCI subsystem...\n");
    Status = PCIInitialize();
    if (Status != IO_SUCCESS) {
        printf("ERROR: PCI initialization failed (status=0x%08X)\n", Status);
        return -1;
    }
    printf("PCI subsystem initialized successfully\n\n");

    // Scan bus 0 (primary PCI bus)
    printf("Scanning PCI bus 0...\n");
    uDeviceCount = 256;
    Status = PCIScanBus(0, pDevices, &uDeviceCount);
    if (Status != IO_SUCCESS) {
        printf("ERROR: Bus scan failed (status=0x%08X)\n", Status);
        return -1;
    }

    printf("Found %u PCI device(s)\n\n", uDeviceCount);

    // Display information about each device
    for (i = 0; i < uDeviceCount; i++) {
        PCI_DEVICE_INFO DeviceInfo;

        Status = IIOPCIDevice_GetDeviceInfo(pDevices[i], &DeviceInfo);
        if (Status == IO_SUCCESS) {
            printf("Device %u:\n", i);
            printf("  Location:    %02X:%02X.%X\n",
                   DeviceInfo.Location.Bus,
                   DeviceInfo.Location.Device,
                   DeviceInfo.Location.Function);
            printf("  Vendor:      0x%04X\n", DeviceInfo.VendorID);
            printf("  Device:      0x%04X\n", DeviceInfo.DeviceID);
            printf("  Class:       0x%02X (Subclass 0x%02X)\n",
                   DeviceInfo.ClassCode, DeviceInfo.SubClass);
            printf("  Header Type: 0x%02X\n", DeviceInfo.HeaderType);

            // Display BARs
            for (UINT32 j = 0; j < 6; j++) {
                if (DeviceInfo.BARs[j].Size > 0) {
                    printf("  BAR%u:        0x%016llX (size=0x%llX, %s, %s)\n",
                           j,
                           DeviceInfo.BARs[j].PhysicalAddress,
                           DeviceInfo.BARs[j].Size,
                           DeviceInfo.BARs[j].bIsMem ? "Memory" : "I/O",
                           DeviceInfo.BARs[j].bIs64Bit ? "64-bit" : "32-bit");
                }
            }

            printf("\n");
        }

        // Release device reference
        IIOPCIDevice_Release(pDevices[i]);
    }

    return 0;
}

/**
 * @brief Example 2: Work with a specific PCI device
 */
static int
Example_PCIe_DeviceAccess(void)
{
    IO_RETURN Status;
    PCI_LOCATION Location;
    IIOPCIDevice *pDevice;
    UINT32 uValue;
    VOID *pMappedBAR;
    UINT64 cbBARSize;

    printf("\n");
    printf("========================================\n");
    printf("  Example 2: PCIe Device Access\n");
    printf("========================================\n\n");

    // Create a device at a specific location (00:02.0 - typically Intel GPU)
    Location.Segment = 0;
    Location.Bus = 0;
    Location.Device = 2;
    Location.Function = 0;

    printf("Creating PCI device at 00:02.0...\n");
    Status = PCIDeviceCreate(&Location, &pDevice);
    if (Status != IO_SUCCESS) {
        printf("Device not found or unavailable\n");
        return 0;  // Not an error, just not present
    }

    printf("Device created successfully\n\n");

    // Read vendor/device ID
    Status = IIOPCIDevice_ConfigRead(pDevice, PCI_CFG_VENDOR_ID, 2, &uValue);
    if (Status == IO_SUCCESS) {
        printf("Vendor ID: 0x%04X\n", uValue);
    }

    Status = IIOPCIDevice_ConfigRead(pDevice, PCI_CFG_DEVICE_ID, 2, &uValue);
    if (Status == IO_SUCCESS) {
        printf("Device ID: 0x%04X\n", uValue);
    }

    // Read command register
    Status = IIOPCIDevice_ConfigRead(pDevice, PCI_CFG_COMMAND, 2, &uValue);
    if (Status == IO_SUCCESS) {
        printf("Command:   0x%04X", uValue);
        if (uValue & PCI_CMD_IO_SPACE) printf(" [IO]");
        if (uValue & PCI_CMD_MEMORY_SPACE) printf(" [MEM]");
        if (uValue & PCI_CMD_BUS_MASTER) printf(" [MASTER]");
        printf("\n\n");
    }

    // Enable memory space and bus mastering
    printf("Enabling memory space and bus mastering...\n");
    IIOPCIDevice_SetMemoryIOEnable(pDevice, TRUE, FALSE);
    IIOPCIDevice_SetBusMaster(pDevice, TRUE);

    // Try to map BAR0
    printf("Mapping BAR0...\n");
    Status = IIOPCIDevice_MapBAR(pDevice, 0, &pMappedBAR, &cbBARSize);
    if (Status == IO_SUCCESS) {
        printf("BAR0 mapped at %p (size=0x%llX)\n", pMappedBAR, cbBARSize);

        // In a real driver, you would now access device registers
        // For example:
        // volatile UINT32 *pRegs = (volatile UINT32 *)pMappedBAR;
        // UINT32 uDeviceStatus = pRegs[0];  // Read register 0

        // Unmap when done
        IIOPCIDevice_UnmapBAR(pDevice, 0);
        printf("BAR0 unmapped\n");
    } else {
        printf("Failed to map BAR0 (status=0x%08X)\n", Status);
    }

    printf("\n");

    // Look for MSI capability
    UINT32 uMSIOffset;
    Status = IIOPCIDevice_FindCapability(pDevice, PCI_CAP_ID_MSI, &uMSIOffset);
    if (Status == IO_SUCCESS) {
        printf("MSI capability found at offset 0x%02X\n", uMSIOffset);

        // In a real driver, you would setup MSI interrupts here
        // IIOPCIDevice_SetupMSI(pDevice, 1, MyInterruptHandler, pContext);
    } else {
        printf("MSI capability not found\n");
    }

    // Look for MSI-X capability
    UINT32 uMSIXOffset;
    Status = IIOPCIDevice_FindCapability(pDevice, PCI_CAP_ID_MSIX, &uMSIXOffset);
    if (Status == IO_SUCCESS) {
        printf("MSI-X capability found at offset 0x%02X\n", uMSIXOffset);
    }

    // Clean up
    IIOPCIDevice_Release(pDevice);

    return 0;
}

/**
 * @brief Example 3: Detect Thunderbolt controllers
 */
static int
Example_Thunderbolt_Detection(void)
{
    IO_RETURN Status;
    IIOThunderboltController *pControllers[8];
    UINT32 uControllerCount;
    UINT32 i;

    printf("\n");
    printf("========================================\n");
    printf("  Example 3: Thunderbolt Detection\n");
    printf("========================================\n\n");

    // Initialize Thunderbolt subsystem
    printf("Initializing Thunderbolt subsystem...\n");
    Status = ThunderboltInitialize();
    if (Status != IO_SUCCESS) {
        printf("ERROR: Thunderbolt initialization failed (status=0x%08X)\n", Status);
        return -1;
    }
    printf("Thunderbolt subsystem initialized\n\n");

    // Detect Thunderbolt controllers
    printf("Detecting Thunderbolt controllers...\n");
    uControllerCount = 8;
    Status = ThunderboltDetectControllers(pControllers, &uControllerCount);
    if (Status != IO_SUCCESS) {
        printf("ERROR: Controller detection failed (status=0x%08X)\n", Status);
        return -1;
    }

    if (uControllerCount == 0) {
        printf("No Thunderbolt controllers found\n");
        return 0;
    }

    printf("Found %u Thunderbolt controller(s)\n\n", uControllerCount);

    // Display information about each controller
    for (i = 0; i < uControllerCount; i++) {
        TB_CONTROLLER_INFO ControllerInfo;

        Status = IIOThunderboltController_GetControllerInfo(pControllers[i],
                                                            &ControllerInfo);
        if (Status == IO_SUCCESS) {
            printf("Controller %u:\n", i);
            printf("  Name:         %s\n", ControllerInfo.ControllerName);
            printf("  Vendor:       0x%04X\n", ControllerInfo.VendorID);
            printf("  Device:       0x%04X\n", ControllerInfo.DeviceID);
            printf("  Generation:   Thunderbolt %u\n", ControllerInfo.Generation);
            printf("  NVM Version:  0x%08X\n", ControllerInfo.NVMVersion);
            printf("  Security:     Level %u\n", ControllerInfo.SecurityLevel);
            printf("  Max Depth:    %u hops\n", ControllerInfo.MaxDepth);
            printf("  Max Ports:    %u\n", ControllerInfo.MaxPortCount);

            printf("  Capabilities:");
            if (ControllerInfo.Capabilities & TB_CAP_HOTPLUG)
                printf(" Hot-Plug");
            if (ControllerInfo.Capabilities & TB_CAP_DAISY_CHAIN)
                printf(" Daisy-Chain");
            if (ControllerInfo.Capabilities & TB_CAP_POWER_DELIVERY)
                printf(" PD");
            if (ControllerInfo.Capabilities & TB_CAP_DISPLAYPORT)
                printf(" DP");
            if (ControllerInfo.Capabilities & TB_CAP_USB3)
                printf(" USB3");
            if (ControllerInfo.Capabilities & TB_CAP_CHARGING)
                printf(" Charging");
            if (ControllerInfo.Capabilities & TB_CAP_WAKE)
                printf(" Wake");
            if (ControllerInfo.Capabilities & TB_CAP_IOMMU)
                printf(" IOMMU");
            printf("\n\n");
        }

        // Release controller reference
        IIOThunderboltController_Release(pControllers[i]);
    }

    return 0;
}

/**
 * @brief Example 4: Manage Thunderbolt devices
 */
static int
Example_Thunderbolt_DeviceManagement(void)
{
    IO_RETURN Status;
    IIOThunderboltController *pController = NULL;
    IIOThunderboltDevice *pDevices[64];
    UINT32 uDeviceCount;
    UINT32 i;

    printf("\n");
    printf("========================================\n");
    printf("  Example 4: Thunderbolt Devices\n");
    printf("========================================\n\n");

    // Note: This example assumes a controller was found in Example 3
    // In a real implementation, you would get the controller reference

    if (pController == NULL) {
        printf("No Thunderbolt controller available for this example\n");
        return 0;
    }

    // Enumerate connected devices
    printf("Enumerating Thunderbolt devices...\n");
    uDeviceCount = 64;
    Status = IIOThunderboltController_EnumerateDevices(pController, pDevices,
                                                       &uDeviceCount);
    if (Status != IO_SUCCESS) {
        printf("ERROR: Device enumeration failed (status=0x%08X)\n", Status);
        return -1;
    }

    printf("Found %u Thunderbolt device(s)\n\n", uDeviceCount);

    // Display information about each device
    for (i = 0; i < uDeviceCount; i++) {
        TB_DEVICE_INFO DeviceInfo;
        TB_CONNECTION_STATE State;

        Status = IIOThunderboltDevice_GetDeviceInfo(pDevices[i], &DeviceInfo);
        if (Status == IO_SUCCESS) {
            printf("Device %u:\n", i);
            printf("  Name:         %s\n", DeviceInfo.DeviceName);
            printf("  Unique ID:    0x%016llX\n", DeviceInfo.UniqueID);
            printf("  Vendor:       0x%04X\n", DeviceInfo.VendorID);
            printf("  Device:       0x%04X\n", DeviceInfo.DeviceID);
            printf("  Generation:   Thunderbolt %u\n", DeviceInfo.Generation);
            printf("  Depth:        %u hop(s)\n", DeviceInfo.Depth);
            printf("  Authorized:   %s\n", DeviceInfo.bAuthorized ? "Yes" : "No");

            // Get connection state
            IIOThunderboltDevice_GetConnectionState(pDevices[i], &State);
            printf("  State:        ");
            switch (State) {
                case TB_STATE_DISCONNECTED: printf("Disconnected\n"); break;
                case TB_STATE_CONNECTING: printf("Connecting\n"); break;
                case TB_STATE_CONNECTED: printf("Connected\n"); break;
                case TB_STATE_AUTHORIZING: printf("Authorizing\n"); break;
                case TB_STATE_AUTHORIZED: printf("Authorized\n"); break;
                case TB_STATE_ERROR: printf("Error\n"); break;
                default: printf("Unknown\n"); break;
            }

            // If device needs authorization
            if (!DeviceInfo.bAuthorized &&
                (State == TB_STATE_CONNECTED || State == TB_STATE_AUTHORIZING)) {
                printf("  -> Authorizing device...\n");
                Status = IIOThunderboltController_AuthorizeDevice(pController,
                                                                 pDevices[i],
                                                                 FALSE);
                if (Status == IO_SUCCESS) {
                    printf("  -> Device authorized\n");
                } else {
                    printf("  -> Authorization failed (status=0x%08X)\n", Status);
                }
            }

            printf("\n");
        }

        // Release device reference
        IIOThunderboltDevice_Release(pDevices[i]);
    }

    return 0;
}

/**
 * @brief Example 5: Find specific device types
 */
static int
Example_Find_SpecificDevices(void)
{
    IO_RETURN Status;
    IIOPCIDevice *pDevices[256];
    UINT32 uDeviceCount;
    UINT32 i;
    UINT32 uNVMeCount = 0;
    UINT32 uNetworkCount = 0;
    UINT32 uUSBCount = 0;

    printf("\n");
    printf("========================================\n");
    printf("  Example 5: Find Specific Devices\n");
    printf("========================================\n\n");

    // Scan for all devices
    uDeviceCount = 256;
    Status = PCIScanBus(0, pDevices, &uDeviceCount);
    if (Status != IO_SUCCESS) {
        return -1;
    }

    printf("Scanning for specific device types...\n\n");

    // Categorize devices by class
    for (i = 0; i < uDeviceCount; i++) {
        PCI_DEVICE_INFO DeviceInfo;

        if (IIOPCIDevice_GetDeviceInfo(pDevices[i], &DeviceInfo) == IO_SUCCESS) {
            // NVMe controller (Storage controller, Non-Volatile memory, NVMHCI)
            if (DeviceInfo.ClassCode == PCI_CLASS_STORAGE &&
                DeviceInfo.SubClass == 0x08 && DeviceInfo.ProgIf == 0x02) {
                printf("NVMe Controller: %04X:%04X at %02X:%02X.%X\n",
                       DeviceInfo.VendorID, DeviceInfo.DeviceID,
                       DeviceInfo.Location.Bus, DeviceInfo.Location.Device,
                       DeviceInfo.Location.Function);
                uNVMeCount++;
            }

            // Network controller
            if (DeviceInfo.ClassCode == PCI_CLASS_NETWORK) {
                printf("Network Controller: %04X:%04X at %02X:%02X.%X (Subclass 0x%02X)\n",
                       DeviceInfo.VendorID, DeviceInfo.DeviceID,
                       DeviceInfo.Location.Bus, DeviceInfo.Location.Device,
                       DeviceInfo.Location.Function, DeviceInfo.SubClass);
                uNetworkCount++;
            }

            // USB controller
            if (DeviceInfo.ClassCode == PCI_CLASS_SERIAL_BUS &&
                DeviceInfo.SubClass == PCI_SUBCLASS_SERIAL_USB) {
                printf("USB Controller: %04X:%04X at %02X:%02X.%X (ProgIf 0x%02X)\n",
                       DeviceInfo.VendorID, DeviceInfo.DeviceID,
                       DeviceInfo.Location.Bus, DeviceInfo.Location.Device,
                       DeviceInfo.Location.Function, DeviceInfo.ProgIf);
                uUSBCount++;
            }
        }

        IIOPCIDevice_Release(pDevices[i]);
    }

    printf("\nSummary:\n");
    printf("  NVMe Controllers:    %u\n", uNVMeCount);
    printf("  Network Controllers: %u\n", uNetworkCount);
    printf("  USB Controllers:     %u\n", uUSBCount);

    return 0;
}

/**
 * @brief Main function - Run all examples
 */
int
main(void)
{
    printf("\n");
    printf("================================================\n");
    printf("  PCIe and Thunderbolt Driver Examples\n");
    printf("================================================\n");

    // Run all examples
    Example_PCIe_Initialization();
    Example_PCIe_DeviceAccess();
    Example_Thunderbolt_Detection();
    Example_Thunderbolt_DeviceManagement();
    Example_Find_SpecificDevices();

    printf("\n");
    printf("================================================\n");
    printf("  All examples completed\n");
    printf("================================================\n");
    printf("\n");

    return 0;
}
