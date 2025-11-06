/**
 * @file aoe_example.c
 * @brief ATA over Ethernet (AoE) Usage Examples
 *
 * This file demonstrates how to use the AoE protocol interface to discover,
 * connect to, and perform I/O operations on AoE storage targets over Ethernet.
 *
 * AoE is a lightweight Layer 2 protocol that provides block storage access
 * over Ethernet without the overhead of TCP/IP, UDP, or iSCSI. It's ideal
 * for storage area networks (SANs) and provides low-latency disk access.
 *
 * Key AoE Features:
 * - Simple Layer 2 Ethernet protocol (EtherType 0x88A2)
 * - Major/Minor device addressing (16.7M addressable devices)
 * - Direct ATA command encapsulation over Ethernet
 * - Jumbo frame support for high performance
 * - Broadcast discovery mechanism
 * - Reserve/Release for multi-host access control
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/families/ata/ata.h>
#include <iokit/families/network/network.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Example 1: Create AoE Controller and Discover Targets
 *
 * This example demonstrates how to create an AoE initiator controller
 * and discover all available AoE targets on the network.
 */
void
Example1_DiscoverAoETargets(
    IIOService *pNetworkDevice
    )
{
    IIOAoEController *pAoEController = NULL;
    IO_RETURN Status;
    UINT32 uTargetCount = 0;
    UINT32 i;
    AOE_DEVICE_INFO TargetInfo;
    CHAR8 szAddr[16];

    printf("=== Example 1: Discover AoE Targets ===\n\n");

    if (pNetworkDevice == NULL) {
        printf("No network device provided.\n");
        printf("In a real implementation:\n");
        printf("  1. Query IORegistry for network devices\n");
        printf("  2. Filter for Ethernet devices with raw frame support\n");
        printf("  3. Create IIOAoEController for each suitable device\n\n");
        return;
    }

    // Create AoE controller instance
    Status = AoEControllerCreate(pNetworkDevice, &pAoEController);
    if (Status != IO_SUCCESS) {
        printf("Failed to create AoE controller: 0x%X\n\n", Status);
        return;
    }

    printf("AoE controller created successfully.\n");

    // Discover targets with 5 second timeout
    printf("Discovering AoE targets on network...\n");
    Status = IIOAoEController_DiscoverTargets(pAoEController, 5000);
    if (Status != IO_SUCCESS) {
        printf("Target discovery failed: 0x%X\n\n", Status);
        pAoEController->lpVtbl->Release(pAoEController);
        return;
    }

    // Get target count
    Status = IIOAoEController_GetTargetCount(pAoEController, &uTargetCount);
    if (Status != IO_SUCCESS || uTargetCount == 0) {
        printf("No AoE targets found on network.\n\n");
        pAoEController->lpVtbl->Release(pAoEController);
        return;
    }

    printf("Found %u AoE target(s):\n\n", uTargetCount);

    // Enumerate and display each target
    for (i = 0; i < uTargetCount; i++) {
        Status = IIOAoEController_GetTargetInfo(pAoEController, i, &TargetInfo);
        if (Status == IO_SUCCESS) {
            AoEAddressToString(TargetInfo.Major, TargetInfo.Minor, szAddr, sizeof(szAddr));

            printf("Target %u: %s\n", i, szAddr);
            printf("  Major/Minor:   %u.%u\n", TargetInfo.Major, TargetInfo.Minor);
            printf("  MAC Address:   %02X:%02X:%02X:%02X:%02X:%02X\n",
                   TargetInfo.MacAddr[0], TargetInfo.MacAddr[1],
                   TargetInfo.MacAddr[2], TargetInfo.MacAddr[3],
                   TargetInfo.MacAddr[4], TargetInfo.MacAddr[5]);
            printf("  MTU:           %u bytes\n", TargetInfo.MTU);
            printf("  Jumbo Frames:  %s\n", TargetInfo.bJumboFrames ? "Yes" : "No");
            printf("  Max Sectors:   %u sectors/request\n", TargetInfo.MaxSectors);
            printf("  Buffers:       %u\n", TargetInfo.BufferCount);
            printf("  Firmware:      v%u.%u\n",
                   TargetInfo.FirmwareVersion >> 8,
                   TargetInfo.FirmwareVersion & 0xFF);
            printf("  Config:        %s\n\n", TargetInfo.ConfigString);
        }
    }

    // Release controller
    pAoEController->lpVtbl->Release(pAoEController);
}

/**
 * @brief Example 2: Connect to AoE Target
 *
 * This example shows how to connect to a specific AoE target by
 * major/minor address and obtain an ATA device interface for I/O.
 */
void
Example2_ConnectToTarget(
    IIOAoEController *pAoEController
    )
{
    IIOATADevice *pATADevice = NULL;
    ATA_DEVICE_INFO DeviceInfo;
    IO_RETURN Status;
    UINT16 uMajor = 0;
    UINT8 uMinor = 0;

    printf("=== Example 2: Connect to AoE Target ===\n\n");

    if (pAoEController == NULL) {
        printf("No AoE controller provided for example.\n\n");
        return;
    }

    // Connect to target e0.0 (major=0, minor=0)
    printf("Connecting to AoE target e%u.%u...\n", uMajor, uMinor);
    Status = IIOAoEController_ConnectTarget(pAoEController, uMajor, uMinor, &pATADevice);
    if (Status != IO_SUCCESS) {
        printf("Connection failed: 0x%X\n", Status);
        if (Status == IO_NO_DEVICE) {
            printf("  Target not found. Run discovery first.\n");
        } else if (Status == IO_BUSY) {
            printf("  Target is reserved by another host.\n");
        }
        printf("\n");
        return;
    }

    printf("Successfully connected to target e%u.%u\n\n", uMajor, uMinor);

    // Get device information
    if (pATADevice != NULL) {
        Status = IIOATADevice_GetDeviceInfo(pATADevice, &DeviceInfo);
        if (Status == IO_SUCCESS) {
            printf("Device Information:\n");
            printf("  Model:         %s\n", DeviceInfo.Model);
            printf("  Serial:        %s\n", DeviceInfo.SerialNumber);
            printf("  Firmware:      %s\n", DeviceInfo.FirmwareRevision);
            printf("  Capacity:      %llu MB (%llu sectors)\n",
                   DeviceInfo.TotalCapacity / (1024 * 1024),
                   DeviceInfo.TotalSectors);
            printf("  Sector Size:   %u bytes\n", DeviceInfo.SectorSize);
            printf("  Protocol:      ATA over Ethernet (AoE)\n");
            printf("  LBA48:         %s\n", DeviceInfo.bLBA48 ? "Yes" : "No");
            printf("  S.M.A.R.T.:    %s\n", DeviceInfo.bSMART ? "Yes" : "No");
        }

        // Release device interface
        pATADevice->lpVtbl->Release(pATADevice);
    }

    printf("\n");
}

/**
 * @brief Example 3: AoE Read/Write Operations
 *
 * Demonstrates reading and writing sectors to an AoE target.
 */
void
Example3_ReadWriteOperations(
    IIOATADevice *pAoEDevice
    )
{
    UINT8 WriteBuffer[512 * 4];  // 4 sectors
    UINT8 ReadBuffer[512 * 4];   // 4 sectors
    IO_RETURN Status;
    UINT32 i;

    printf("=== Example 3: AoE Read/Write Operations ===\n\n");

    if (pAoEDevice == NULL) {
        printf("No AoE device provided for example.\n\n");
        return;
    }

    // Prepare write data
    printf("Preparing test data (4 sectors)...\n");
    for (i = 0; i < sizeof(WriteBuffer); i++) {
        WriteBuffer[i] = (UINT8)(i & 0xFF);
    }

    // Write to LBA 1000
    printf("Writing 4 sectors to LBA 1000...\n");
    Status = IIOATADevice_WriteSectors(pAoEDevice, 1000, 4, WriteBuffer, sizeof(WriteBuffer));
    if (Status != IO_SUCCESS) {
        printf("Write failed: 0x%X\n\n", Status);
        return;
    }

    printf("Write successful!\n\n");

    // Read back from LBA 1000
    printf("Reading 4 sectors from LBA 1000...\n");
    memset(ReadBuffer, 0, sizeof(ReadBuffer));
    Status = IIOATADevice_ReadSectors(pAoEDevice, 1000, 4, ReadBuffer, sizeof(ReadBuffer));
    if (Status != IO_SUCCESS) {
        printf("Read failed: 0x%X\n\n", Status);
        return;
    }

    printf("Read successful!\n");

    // Verify data
    printf("Verifying data...\n");
    if (memcmp(WriteBuffer, ReadBuffer, sizeof(WriteBuffer)) == 0) {
        printf("Data verification: PASS\n");
        printf("First 64 bytes:\n");
        for (i = 0; i < 64; i++) {
            printf("%02X ", ReadBuffer[i]);
            if ((i + 1) % 16 == 0) {
                printf("\n");
            }
        }
    } else {
        printf("Data verification: FAIL\n");
    }

    printf("\n");
}

/**
 * @brief Example 4: AoE Configuration and Jumbo Frames
 *
 * Shows how to query AoE target configuration and enable jumbo frames
 * for improved performance.
 */
void
Example4_ConfigurationAndJumboFrames(
    IIOAoEController *pAoEController
    )
{
    AOE_DEVICE_INFO Config;
    IO_RETURN Status;
    UINT16 uMajor = 0;
    UINT8 uMinor = 0;

    printf("=== Example 4: AoE Configuration and Jumbo Frames ===\n\n");

    if (pAoEController == NULL) {
        printf("No AoE controller provided for example.\n\n");
        return;
    }

    // Query target configuration
    printf("Querying configuration for target e%u.%u...\n", uMajor, uMinor);
    Status = IIOAoEController_QueryConfig(pAoEController, uMajor, uMinor, &Config);
    if (Status != IO_SUCCESS) {
        printf("Configuration query failed: 0x%X\n\n", Status);
        return;
    }

    printf("Target Configuration:\n");
    printf("  Major/Minor:   %u.%u\n", Config.Major, Config.Minor);
    printf("  MTU:           %u bytes\n", Config.MTU);
    printf("  Max Sectors:   %u sectors/request\n", Config.MaxSectors);
    printf("  Buffers:       %u\n", Config.BufferCount);
    printf("  Firmware:      v%u.%u\n",
           Config.FirmwareVersion >> 8,
           Config.FirmwareVersion & 0xFF);
    printf("  Config String: %s\n\n", Config.ConfigString);

    // Enable jumbo frames if supported
    if (Config.bJumboFrames) {
        printf("Target supports jumbo frames. Enabling...\n");
        Status = IIOAoEController_SetJumboFrames(pAoEController, TRUE, AOE_MTU_JUMBO);
        if (Status == IO_SUCCESS) {
            printf("Jumbo frames enabled (MTU %u bytes)\n", AOE_MTU_JUMBO);
            printf("Performance benefit:\n");
            printf("  Standard MTU: %u sectors/request (max %u KB/request)\n",
                   AOE_MAX_SECTORS_STANDARD,
                   AOE_MAX_SECTORS_STANDARD / 2);
            printf("  Jumbo frames: %u sectors/request (max %u KB/request)\n",
                   AOE_MAX_SECTORS_JUMBO,
                   AOE_MAX_SECTORS_JUMBO / 2);
            printf("  Improvement:  %ux throughput increase\n",
                   AOE_MAX_SECTORS_JUMBO / AOE_MAX_SECTORS_STANDARD);
        } else {
            printf("Failed to enable jumbo frames: 0x%X\n", Status);
        }
    } else {
        printf("Target does not support jumbo frames (standard MTU %u bytes)\n",
               AOE_MTU_STANDARD);
    }

    printf("\n");
}

/**
 * @brief Example 5: AoE Reserve/Release
 *
 * Demonstrates multi-host access control using reserve/release commands.
 */
void
Example5_ReserveRelease(
    IIOAoEController *pAoEController
    )
{
    IO_RETURN Status;
    UINT16 uMajor = 1;
    UINT8 uMinor = 5;

    printf("=== Example 5: AoE Reserve/Release ===\n\n");

    if (pAoEController == NULL) {
        printf("No AoE controller provided for example.\n\n");
        return;
    }

    // Reserve target for exclusive access
    printf("Reserving target e%u.%u for exclusive access...\n", uMajor, uMinor);
    Status = IIOAoEController_ReserveTarget(pAoEController, uMajor, uMinor, FALSE);
    if (Status == IO_SUCCESS) {
        printf("Target reserved successfully.\n");
        printf("Other hosts will receive IO_BUSY when attempting to access.\n\n");

        // Perform exclusive operations here
        printf("Performing exclusive operations on reserved target...\n");
        printf("(In real usage, perform critical I/O operations here)\n\n");

        // Release target
        printf("Releasing target e%u.%u...\n", uMajor, uMinor);
        Status = IIOAoEController_ReleaseTarget(pAoEController, uMajor, uMinor);
        if (Status == IO_SUCCESS) {
            printf("Target released. Other hosts can now access.\n");
        } else {
            printf("Release failed: 0x%X\n", Status);
        }
    } else if (Status == IO_BUSY) {
        printf("Target is already reserved by another host.\n");
        printf("Use bForce=TRUE to force reservation (use with caution!).\n");
    } else if (Status == IO_NO_DEVICE) {
        printf("Target not found.\n");
    } else {
        printf("Reservation failed: 0x%X\n", Status);
    }

    printf("\n");
}

/**
 * @brief Example 6: AoE Performance Statistics
 *
 * Shows how to retrieve network performance statistics for AoE operations.
 */
void
Example6_PerformanceStatistics(
    IIOAoEController *pAoEController
    )
{
    UINT64 uPacketsSent = 0;
    UINT64 uPacketsReceived = 0;
    UINT64 uBytesTransferred = 0;
    UINT32 uErrors = 0;
    IO_RETURN Status;

    printf("=== Example 6: AoE Performance Statistics ===\n\n");

    if (pAoEController == NULL) {
        printf("No AoE controller provided for example.\n\n");
        return;
    }

    // Get statistics
    Status = IIOAoEController_GetStatistics(pAoEController,
                                            &uPacketsSent,
                                            &uPacketsReceived,
                                            &uBytesTransferred,
                                            &uErrors);
    if (Status != IO_SUCCESS) {
        printf("Failed to retrieve statistics: 0x%X\n\n", Status);
        return;
    }

    printf("AoE Network Statistics:\n");
    printf("  Packets Sent:      %llu\n", uPacketsSent);
    printf("  Packets Received:  %llu\n", uPacketsReceived);
    printf("  Total Transfers:   %llu (%llu MB)\n",
           uPacketsSent + uPacketsReceived,
           uBytesTransferred / (1024 * 1024));
    printf("  Bytes Transferred: %llu bytes\n", uBytesTransferred);
    printf("  Errors:            %u\n", uErrors);

    if (uPacketsSent > 0) {
        printf("  Success Rate:      %.2f%%\n",
               ((double)(uPacketsSent - uErrors) / uPacketsSent) * 100.0);
    }

    printf("\n");
}

/**
 * @brief Example 7: AoE Protocol Details
 *
 * Demonstrates low-level AoE protocol operations and packet construction.
 */
void
Example7_ProtocolDetails(
    VOID
    )
{
    AOE_HEADER Header;
    AOE_ATA_COMMAND AtaCmd;
    CHAR8 szAddr[16];

    printf("=== Example 7: AoE Protocol Details ===\n\n");

    // Display protocol constants
    printf("AoE Protocol Information:\n");
    printf("  EtherType:     0x%04X\n", AOE_ETHERTYPE);
    printf("  Version:       %u\n", AOE_VERSION_1);
    printf("  Header Size:   %zu bytes\n", sizeof(AOE_HEADER));
    printf("  ATA Cmd Size:  %zu bytes\n", sizeof(AOE_ATA_COMMAND));
    printf("\n");

    printf("Address Space:\n");
    printf("  Major Range:   %u - %u (0x%04X - 0x%04X)\n",
           AOE_MAJOR_MIN, AOE_MAJOR_MAX,
           AOE_MAJOR_MIN, AOE_MAJOR_MAX);
    printf("  Minor Range:   %u - %u (0x%02X - 0x%02X)\n",
           AOE_MINOR_MIN, AOE_MINOR_MAX,
           AOE_MINOR_MIN, AOE_MINOR_MAX);
    printf("  Broadcast:     0xFFFF.0xFF\n");
    printf("  Total Devices: %u (65536 × 256)\n", 65536 * 256);
    printf("\n");

    printf("Performance Characteristics:\n");
    printf("  Standard MTU:  %u bytes (%u sectors max)\n",
           AOE_MTU_STANDARD, AOE_MAX_SECTORS_STANDARD);
    printf("  Jumbo MTU:     %u bytes (%u sectors max)\n",
           AOE_MTU_JUMBO, AOE_MAX_SECTORS_JUMBO);
    printf("  Overhead:      ~%zu bytes (Ethernet + AoE headers)\n",
           14 + sizeof(AOE_HEADER) + sizeof(AOE_ATA_COMMAND));
    printf("\n");

    // Build sample AoE header
    printf("Sample AoE Header Construction:\n");
    AoEBuildHeader(&Header, AOE_CMD_ISSUE_ATA_COMMAND, 0, 0, 12345, AOE_FLAG_WRITE);
    printf("  Version|Flags: 0x%02X\n", Header.Ver_Flags);
    printf("  Error:         0x%02X\n", Header.Error);
    printf("  Major:         %u (0x%04X)\n", Header.Major, Header.Major);
    printf("  Minor:         %u (0x%02X)\n", Header.Minor, Header.Minor);
    printf("  Command:       0x%02X (%s)\n", Header.Command,
           Header.Command == AOE_CMD_ISSUE_ATA_COMMAND ? "ATA Command" : "Unknown");
    printf("  Tag:           %u (0x%08X)\n", Header.Tag, Header.Tag);
    printf("\n");

    // Show addressing examples
    printf("AoE Address Format Examples:\n");
    AoEAddressToString(0, 0, szAddr, sizeof(szAddr));
    printf("  %s  -> Major 0, Minor 0 (shelf 0, slot 0)\n", szAddr);
    AoEAddressToString(1, 5, szAddr, sizeof(szAddr));
    printf("  %s  -> Major 1, Minor 5 (shelf 1, slot 5)\n", szAddr);
    AoEAddressToString(255, 15, szAddr, sizeof(szAddr));
    printf("  %s -> Major 255, Minor 15 (shelf 255, slot 15)\n", szAddr);

    printf("\n");
}

/**
 * @brief Example 8: AoE Hot-plug and Device Discovery
 *
 * Shows how to handle hot-plug events and dynamic device discovery.
 */
void
Example8_HotPlugDiscovery(
    IIOAoEController *pAoEController
    )
{
    IO_RETURN Status;
    UINT32 uTargetCount1, uTargetCount2;

    printf("=== Example 8: AoE Hot-plug and Dynamic Discovery ===\n\n");

    if (pAoEController == NULL) {
        printf("No AoE controller provided for example.\n\n");
        return;
    }

    // Initial discovery
    printf("Performing initial discovery...\n");
    Status = IIOAoEController_DiscoverTargets(pAoEController, 3000);
    if (Status != IO_SUCCESS) {
        printf("Initial discovery failed: 0x%X\n\n", Status);
        return;
    }

    IIOAoEController_GetTargetCount(pAoEController, &uTargetCount1);
    printf("Found %u targets initially.\n\n", uTargetCount1);

    // Simulate waiting for hot-plug event
    printf("Waiting for hot-plug events...\n");
    printf("(In real implementation, network interface would notify of new frames)\n\n");

    // Rediscover to detect new/removed targets
    printf("Performing rediscovery...\n");
    Status = IIOAoEController_DiscoverTargets(pAoEController, 3000);
    if (Status == IO_SUCCESS) {
        IIOAoEController_GetTargetCount(pAoEController, &uTargetCount2);
        printf("Found %u targets after rediscovery.\n", uTargetCount2);

        if (uTargetCount2 > uTargetCount1) {
            printf("Detected %u new target(s) - hot-plug add event\n",
                   uTargetCount2 - uTargetCount1);
        } else if (uTargetCount2 < uTargetCount1) {
            printf("Detected %u target(s) removed - hot-plug remove event\n",
                   uTargetCount1 - uTargetCount2);
        } else {
            printf("No changes detected.\n");
        }
    }

    printf("\n");
}

/**
 * @brief Main function - Run all AoE examples
 */
int
main(
    int argc,
    char *argv[]
    )
{
    printf("========================================\n");
    printf("ATA over Ethernet (AoE) Usage Examples\n");
    printf("========================================\n\n");

    printf("AoE Protocol Overview:\n");
    printf("  EtherType:     0x%04X (Layer 2 Ethernet)\n", AOE_ETHERTYPE);
    printf("  Version:       %u\n", AOE_VERSION_1);
    printf("  Addressing:    16-bit Major + 8-bit Minor\n");
    printf("  Max Devices:   16,777,216 (65536 × 256)\n");
    printf("  Performance:   Low latency, no TCP/IP overhead\n");
    printf("  Use Cases:     SANs, storage clusters, high-performance I/O\n");
    printf("\n");

    // Run examples (with NULL devices for demonstration)
    Example1_DiscoverAoETargets(NULL);
    Example2_ConnectToTarget(NULL);
    Example3_ReadWriteOperations(NULL);
    Example4_ConfigurationAndJumboFrames(NULL);
    Example5_ReserveRelease(NULL);
    Example6_PerformanceStatistics(NULL);
    Example7_ProtocolDetails();
    Example8_HotPlugDiscovery(NULL);

    printf("========================================\n");
    printf("AoE Examples Complete\n");
    printf("\n");
    printf("Key Takeaways:\n");
    printf("  1. AoE provides simple Layer 2 block storage access\n");
    printf("  2. Discovery uses broadcast to major=0xFFFF, minor=0xFF\n");
    printf("  3. Jumbo frames significantly improve performance\n");
    printf("  4. Reserve/Release enables multi-host coordination\n");
    printf("  5. No TCP/IP overhead = lower latency than iSCSI\n");
    printf("  6. Ideal for dedicated storage networks\n");
    printf("========================================\n");

    return 0;
}
