/**
 * @file platform_example.c
 * @brief Platform Device Matcher Usage Examples
 *
 * This file demonstrates how to use the platform device matchers to
 * discover and enumerate devices across different firmware interfaces.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/platform/platform.h>
#include <stdio.h>

/**
 * @brief Example 1: Match ACPI devices by HID
 */
void Example_ACPIMatchByHID(void)
{
    IIOACPIMatcher      *pMatcher = NULL;
    IIOPlatformDevice   *pDevice = NULL;
    IO_RETURN           ret;

    printf("Example 1: ACPI Device Matching by HID\n");
    printf("========================================\n\n");

    // Get ACPI matcher instance
    ret = IOPlatformGetACPIMatcher(&pMatcher);
    if (IO_FAILED(ret)) {
        printf("Failed to get ACPI matcher: 0x%08X\n", ret);
        return;
    }

    // Match 16550A-compatible COM port
    ret = pMatcher->MatchByHID(pMatcher, "PNP0501", &pDevice);
    if (IO_SUCCEEDED(ret) && pDevice) {
        ACPI_DEVICE_INFO info;

        // Get device information
        ret = pMatcher->GetDeviceInfo(pMatcher, pDevice, &info);
        if (IO_SUCCEEDED(ret)) {
            printf("Found COM port:\n");
            printf("  HID: %s\n", info.HID.String);
            printf("  Path: %s\n", info.szPath);
            printf("  Status: 0x%08X\n", info.uStatus);
        }

        pDevice->Base.Base.Release((IUnknown *)pDevice);
    } else {
        printf("No COM port found\n");
    }

    // Match PCI Express root bridge
    ret = pMatcher->MatchByHID(pMatcher, "PNP0A08", &pDevice);
    if (IO_SUCCEEDED(ret) && pDevice) {
        printf("\nFound PCI Express root bridge\n");
        pDevice->Base.Base.Release((IUnknown *)pDevice);
    }

    pMatcher->Base.Release((IUnknown *)pMatcher);
    printf("\n");
}

/**
 * @brief Example 2: Enumerate all ACPI devices
 */
void Example_ACPIEnumerate(void)
{
    IIOACPIMatcher      *pMatcher = NULL;
    IIOPlatformDevice   **ppDevices = NULL;
    UINT32              uCount = 0;
    IO_RETURN           ret;

    printf("Example 2: Enumerate All ACPI Devices\n");
    printf("======================================\n\n");

    ret = IOPlatformGetACPIMatcher(&pMatcher);
    if (IO_FAILED(ret)) {
        return;
    }

    ret = pMatcher->EnumerateDevices(pMatcher, &ppDevices, &uCount);
    if (IO_SUCCEEDED(ret)) {
        printf("Found %u ACPI devices\n", uCount);

        for (UINT32 i = 0; i < uCount; i++) {
            ACPI_DEVICE_INFO info;

            if (IO_SUCCEEDED(pMatcher->GetDeviceInfo(pMatcher, ppDevices[i], &info))) {
                printf("[%3u] %s - %s\n", i + 1, info.HID.String, info.szPath);
            }

            ppDevices[i]->Base.Base.Release((IUnknown *)ppDevices[i]);
        }

        free(ppDevices);
    }

    pMatcher->Base.Release((IUnknown *)pMatcher);
    printf("\n");
}

/**
 * @brief Example 3: Match ISA PnP devices
 */
void Example_ISAPnPMatch(void)
{
    IIOISAPnPMatcher    *pMatcher = NULL;
    IIOPlatformDevice   *pDevice = NULL;
    IO_RETURN           ret;

    printf("Example 3: ISA Plug and Play Device Matching\n");
    printf("=============================================\n\n");

    ret = IOPlatformGetISAPnPMatcher(&pMatcher);
    if (IO_FAILED(ret)) {
        return;
    }

    // Match serial port
    ret = pMatcher->MatchByID(pMatcher, "PNP0501", &pDevice);
    if (IO_SUCCEEDED(ret) && pDevice) {
        ISAPNP_DEVICE_INFO info;

        ret = pMatcher->GetDeviceInfo(pMatcher, pDevice, &info);
        if (IO_SUCCEEDED(ret)) {
            printf("Found ISA PnP serial port:\n");
            printf("  Device ID: %s\n", info.DeviceID.szFull);
            printf("  CSN: %u, LDN: %u\n", info.uCSN, info.uLDN);
            printf("  I/O Base: 0x%04X\n", info.IOBase[0]);
            printf("  IRQ: %u\n", info.IRQ[0]);
            printf("  Activated: %s\n", info.bActivated ? "Yes" : "No");

            // Activate device if not already activated
            if (!info.bActivated) {
                ret = pMatcher->ActivateDevice(pMatcher, pDevice);
                if (IO_SUCCEEDED(ret)) {
                    printf("  Device activated successfully\n");
                }
            }
        }

        pDevice->Base.Base.Release((IUnknown *)pDevice);
    }

    pMatcher->Base.Release((IUnknown *)pMatcher);
    printf("\n");
}

/**
 * @brief Example 4: Match Device Tree nodes
 */
void Example_DeviceTreeMatch(void)
{
    IIODeviceTreeMatcher    *pMatcher = NULL;
    IIOPlatformDevice       **ppDevices = NULL;
    UINT32                  uCount = 0;
    IO_RETURN               ret;

    printf("Example 4: Device Tree Node Matching\n");
    printf("=====================================\n\n");

    ret = IOPlatformGetDeviceTreeMatcher(&pMatcher);
    if (IO_FAILED(ret)) {
        return;
    }

    // Match ARM PL011 UART devices
    ret = pMatcher->MatchByCompatible(pMatcher, "arm,pl011", &ppDevices, &uCount);
    if (IO_SUCCEEDED(ret) && uCount > 0) {
        printf("Found %u ARM PL011 UART(s):\n", uCount);

        for (UINT32 i = 0; i < uCount; i++) {
            DT_NODE_INFO info;

            if (IO_SUCCEEDED(pMatcher->GetNodeInfo(pMatcher, ppDevices[i], &info))) {
                printf("  [%u] %s at %s\n", i + 1, info.szName, info.szFullPath);
                printf("      Base address: 0x%016llX\n", (unsigned long long)info.uReg[0]);
            }

            ppDevices[i]->Base.Base.Release((IUnknown *)ppDevices[i]);
        }

        free(ppDevices);
    } else {
        printf("No PL011 UARTs found\n");
    }

    // Match by device type
    ret = pMatcher->MatchByDeviceType(pMatcher, "cpu", &ppDevices, &uCount);
    if (IO_SUCCEEDED(ret) && uCount > 0) {
        printf("\nFound %u CPU(s) in device tree\n", uCount);

        for (UINT32 i = 0; i < uCount; i++) {
            ppDevices[i]->Base.Base.Release((IUnknown *)ppDevices[i]);
        }

        free(ppDevices);
    }

    pMatcher->Base.Release((IUnknown *)pMatcher);
    printf("\n");
}

/**
 * @brief Example 5: Match OpenFirmware devices
 */
void Example_OpenFirmwareMatch(void)
{
    IIOOpenFirmwareMatcher  *pMatcher = NULL;
    IIOPlatformDevice       **ppDevices = NULL;
    UINT32                  uCount = 0;
    IO_RETURN               ret;

    printf("Example 5: OpenFirmware Device Matching\n");
    printf("========================================\n\n");

    ret = IOPlatformGetOpenFirmwareMatcher(&pMatcher);
    if (IO_FAILED(ret)) {
        return;
    }

    // Match display devices
    ret = pMatcher->MatchByDeviceType(pMatcher, "display", &ppDevices, &uCount);
    if (IO_SUCCEEDED(ret) && uCount > 0) {
        printf("Found %u display device(s):\n", uCount);

        for (UINT32 i = 0; i < uCount; i++) {
            OF_NODE_INFO info;

            if (IO_SUCCEEDED(pMatcher->GetNodeInfo(pMatcher, ppDevices[i], &info))) {
                printf("  [%u] %s\n", i + 1, info.szName);
                printf("      Path: %s\n", info.szFullPath);
                printf("      Model: %s\n", info.szModel);
                printf("      Compatible: %s\n", info.szCompatible);
            }

            ppDevices[i]->Base.Base.Release((IUnknown *)ppDevices[i]);
        }

        free(ppDevices);
    }

    pMatcher->Base.Release((IUnknown *)pMatcher);
    printf("\n");
}

/**
 * @brief Example 6: Match ARC components
 */
void Example_ARCMatch(void)
{
    IIOARCMatcher       *pMatcher = NULL;
    IIOPlatformDevice   **ppDevices = NULL;
    UINT32              uCount = 0;
    IO_RETURN           ret;

    printf("Example 6: ARC/ARCS Component Matching\n");
    printf("=======================================\n\n");

    ret = IOPlatformGetARCMatcher(&pMatcher);
    if (IO_FAILED(ret)) {
        return;
    }

    // Match SCSI controllers
    ret = pMatcher->MatchByClassType(pMatcher, ARC_CLASS_ADAPTER,
                                     ARC_TYPE_SCSI_ADAPTER, &ppDevices, &uCount);
    if (IO_SUCCEEDED(ret) && uCount > 0) {
        printf("Found %u SCSI adapter(s):\n", uCount);

        for (UINT32 i = 0; i < uCount; i++) {
            ARC_COMPONENT_INFO info;

            if (IO_SUCCEEDED(pMatcher->GetComponentInfo(pMatcher, ppDevices[i], &info))) {
                printf("  [%u] %s\n", i + 1, info.szIdentifier);
                printf("      Path: %s\n", info.szPath);
                printf("      Version: %u.%u\n", info.uVersion, info.uRevision);
            }

            ppDevices[i]->Base.Base.Release((IUnknown *)ppDevices[i]);
        }

        free(ppDevices);
    }

    // Match disk controllers
    ret = pMatcher->MatchByClassType(pMatcher, ARC_CLASS_CONTROLLER,
                                     ARC_TYPE_DISK_CONTROLLER, &ppDevices, &uCount);
    if (IO_SUCCEEDED(ret) && uCount > 0) {
        printf("\nFound %u disk controller(s)\n", uCount);

        for (UINT32 i = 0; i < uCount; i++) {
            ppDevices[i]->Base.Base.Release((IUnknown *)ppDevices[i]);
        }

        free(ppDevices);
    }

    pMatcher->Base.Release((IUnknown *)pMatcher);
    printf("\n");
}

/**
 * @brief Example 7: Detect platform firmware type
 */
void Example_DetectFirmware(void)
{
    PLATFORM_FIRMWARE_TYPE type;
    IO_RETURN ret;

    printf("Example 7: Platform Firmware Detection\n");
    printf("=======================================\n\n");

    ret = IOPlatformDetectFirmware(&type);
    if (IO_SUCCEEDED(ret)) {
        const char *pszType;

        switch (type) {
            case PLATFORM_FIRMWARE_BIOS:        pszType = "Legacy BIOS"; break;
            case PLATFORM_FIRMWARE_UEFI:        pszType = "UEFI"; break;
            case PLATFORM_FIRMWARE_ACPI:        pszType = "ACPI"; break;
            case PLATFORM_FIRMWARE_DEVICETREE:  pszType = "Device Tree"; break;
            case PLATFORM_FIRMWARE_OPENFIRMWARE:pszType = "OpenFirmware"; break;
            case PLATFORM_FIRMWARE_ARC:         pszType = "ARC"; break;
            case PLATFORM_FIRMWARE_ARCS:        pszType = "ARCS"; break;
            case PLATFORM_FIRMWARE_EFIBOOT:     pszType = "EFI Boot Services"; break;
            case PLATFORM_FIRMWARE_COREBOOT:    pszType = "coreboot"; break;
            default:                            pszType = "Unknown"; break;
        }

        printf("Detected platform firmware: %s\n", pszType);
    }

    printf("\n");
}

/**
 * @brief Main example driver
 */
int main(void)
{
    IO_RETURN ret;

    printf("==============================================\n");
    printf("Platform Device Matcher Examples\n");
    printf("==============================================\n\n");

    // Initialize platform subsystem
    ret = IOPlatformInitialize();
    if (IO_FAILED(ret)) {
        printf("Failed to initialize platform subsystem: 0x%08X\n", ret);
        return 1;
    }

    // Run examples
    Example_DetectFirmware();
    Example_ACPIMatchByHID();
    Example_ACPIEnumerate();
    Example_ISAPnPMatch();
    Example_DeviceTreeMatch();
    Example_OpenFirmwareMatch();
    Example_ARCMatch();

    printf("==============================================\n");
    printf("All examples completed\n");
    printf("==============================================\n");

    return 0;
}
