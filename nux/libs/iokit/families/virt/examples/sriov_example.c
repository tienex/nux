/**
 * @file sriov_example.c
 * @brief SR-IOV Example - Demonstrates SR-IOV PF/VF management
 *
 * This example shows how to:
 * - Detect SR-IOV capable devices
 * - Create and manage SR-IOV Physical Functions
 * - Enable/disable SR-IOV
 * - Create and configure Virtual Functions
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <stdio.h>
#include <iokit/IOKit.h>
#include <iokit/families/virt/virt.h>
#include <iokit/families/pcie/pcie.h>

/**
 * @brief Example: Enable SR-IOV on a device
 */
static void
Example_EnableSRIOV(
    IIOPCIDevice *pPCIDevice
    )
{
    IO_RETURN Status;
    IIOSRIOVPhysicalFunction *pPF = NULL;
    SRIOV_CAPABILITY Capability;
    SRIOV_PF_INFO PFInfo;

    printf("\n=== SR-IOV Enable Example ===\n");

    // Create SR-IOV PF instance
    Status = IOSRIOVPFCreate("Intel X710 SR-IOV PF", &pPF);
    if (Status != IO_SUCCESS) {
        printf("Failed to create SR-IOV PF: 0x%08X\n", Status);
        return;
    }

    // Start the PF (this will detect SR-IOV capability)
    Status = IIOService_Start((IIOService*)pPF, (IIOService*)pPCIDevice);
    if (Status != IO_SUCCESS) {
        printf("Failed to start SR-IOV PF: 0x%08X\n", Status);
        goto cleanup;
    }

    // Get PF information
    Status = IIOSRIOVPhysicalFunction_GetPFInfo(pPF, &PFInfo);
    if (Status == IO_SUCCESS) {
        printf("PF Information:\n");
        printf("  Vendor ID:  0x%04X\n", PFInfo.VendorID);
        printf("  Device ID:  0x%04X\n", PFInfo.DeviceID);
        printf("  Location:   %02X:%02X.%X\n", PFInfo.Bus, PFInfo.Device, PFInfo.Function);
        printf("  Total VFs:  %d\n", PFInfo.TotalVFs);
        printf("  Active VFs: %d\n", PFInfo.ActiveVFs);
        printf("  VF Device ID: 0x%04X\n", PFInfo.VFDeviceID);
    }

    // Get SR-IOV capability
    Status = IIOSRIOVPhysicalFunction_GetCapability(pPF, &Capability);
    if (Status == IO_SUCCESS) {
        printf("\nSR-IOV Capability:\n");
        printf("  Total VFs:       %d\n", Capability.TotalVFs);
        printf("  First VF Offset: 0x%04X\n", Capability.FirstVFOffset);
        printf("  VF Stride:       0x%04X\n", Capability.VFStride);
        printf("  VF Migration:    %s\n", Capability.bVFMigrationSupported ? "Yes" : "No");
        printf("  ARI Capable:     %s\n", Capability.bARICapable ? "Yes" : "No");
    }

    // Set number of VFs to create
    printf("\nConfiguring 4 Virtual Functions...\n");
    Status = IIOSRIOVPhysicalFunction_SetNumVFs(pPF, 4);
    if (Status != IO_SUCCESS) {
        printf("Failed to set NumVFs: 0x%08X\n", Status);
        goto cleanup;
    }

    // Enable SR-IOV
    printf("Enabling SR-IOV...\n");
    Status = IIOSRIOVPhysicalFunction_EnableSRIOV(pPF);
    if (Status == IO_SUCCESS) {
        printf("SR-IOV enabled successfully!\n");
        printf("4 Virtual Functions are now available\n");

        // In a real implementation, VFs would now be enumerated on the PCI bus
        // at Bus:Device+FirstVFOffset.Function, Bus:Device+FirstVFOffset+Stride.Function, etc.
    } else {
        printf("Failed to enable SR-IOV: 0x%08X\n", Status);
    }

    // Later, to disable SR-IOV:
    // Status = IIOSRIOVPhysicalFunction_DisableSRIOV(pPF);

cleanup:
    if (pPF) {
        IIOService_Release((IUnknown*)pPF);
    }
}

/**
 * @brief Example: Configure VF parameters
 */
static void
Example_ConfigureVF(
    IIOSRIOVPhysicalFunction *pPF
    )
{
    IO_RETURN Status;
    UINT8 MAC[6] = { 0x52, 0x54, 0x00, 0x12, 0x34, 0x56 }; // Example MAC
    UINT16 VLAN = 100;
    UINT32 BandwidthMbps = 1000; // 1 Gbps

    printf("\n=== VF Configuration Example ===\n");

    // Configure VF 0
    printf("Configuring VF 0:\n");
    printf("  MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
           MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
    printf("  VLAN ID:     %d\n", VLAN);
    printf("  Bandwidth:   %d Mbps\n", BandwidthMbps);

    Status = IIOSRIOVPhysicalFunction_ConfigureVF(pPF, 0, MAC, VLAN, BandwidthMbps);
    if (Status == IO_SUCCESS) {
        printf("VF 0 configured successfully\n");
    } else {
        printf("Failed to configure VF 0: 0x%08X\n", Status);
    }
}

/**
 * @brief Example: Scan for SR-IOV capable devices
 */
static void
Example_ScanForSRIOVDevices(
    VOID
    )
{
    printf("\n=== Scanning for SR-IOV Capable Devices ===\n");
    printf("This example would scan the PCI bus for devices with SR-IOV capability\n");
    printf("Typically devices include:\n");
    printf("  - Intel X710/XXV710/E810 NICs\n");
    printf("  - Mellanox ConnectX-5/6/7 NICs\n");
    printf("  - AMD Radeon Instinct MI200/MI300 GPUs\n");
    printf("  - NVIDIA A100/H100 GPUs\n");

    // In a real implementation:
    // 1. Enumerate PCI bus
    // 2. For each device, check for SR-IOV extended capability (0x10)
    // 3. If found, create SR-IOV PF instance
}

int
main(
    int   argc,
    char *argv[]
    )
{
    IO_RETURN Status;

    printf("=== IOKit Virtualization Family - SR-IOV Example ===\n\n");

    // Initialize virtualization subsystem
    Status = VirtInitialize();
    if (Status != IO_SUCCESS) {
        printf("Failed to initialize virtualization subsystem: 0x%08X\n", Status);
        return 1;
    }

    // Run examples
    Example_ScanForSRIOVDevices();

    // Note: In a real scenario, you would get pPCIDevice from device enumeration
    // Example_EnableSRIOV(pPCIDevice);
    // Example_ConfigureVF(pPF);

    // Shutdown
    VirtShutdown();

    printf("\n=== Example Complete ===\n");
    return 0;
}
