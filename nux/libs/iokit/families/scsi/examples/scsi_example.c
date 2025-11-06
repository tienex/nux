/**
 * @file scsi_example.c
 * @brief SCSI/SAS/FC Family Usage Examples
 *
 * This file demonstrates how to use the SCSI family driver for:
 * - SCSI controller enumeration and initialization
 * - Device discovery and management
 * - SCSI command execution
 * - SAS and Fibre Channel operations
 * - SCSI device I/O (disk, tape, CD/DVD)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/scsi/scsi.h>
#include <iokit/families/pcie/pcie.h>
#include <iokit/families/storage/storage.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Example 1: Enumerate all SCSI/SAS/FC controllers
 *
 * This example shows how to find and enumerate all SCSI controllers in the system.
 */
void
Example1_EnumerateSCSIControllers(
    VOID
    )
{
    IO_RETURN Status;
    IIOSCSIController *pController = NULL;
    IIOService *pService = NULL;
    SCSI_CONTROLLER_INFO ControllerInfo;
    UINT32 i;

    printf("\n=== Example 1: Enumerate SCSI Controllers ===\n");

    // Initialize SCSI family
    Status = SCSIInitialize();
    if (Status != IO_SUCCESS) {
        printf("Failed to initialize SCSI family: 0x%X\n", Status);
        return;
    }

    // In a real implementation, you would iterate through PCIe devices
    // and create SCSI controller instances for matching devices
    // For this example, we demonstrate the API usage

    printf("SCSI/SAS/FC controllers found in system:\n");
    printf("  - LSI/Broadcom SAS3008 12Gb/s SAS-3 Controller\n");
    printf("  - QLogic ISP2532 8Gb Fibre Channel HBA\n");
    printf("  - Adaptec ASC-29320 Ultra320 SCSI Controller\n");

    // Example: Get controller information
    if (pController != NULL) {
        Status = IIOSCSIController_GetControllerInfo(pController, &ControllerInfo);
        if (Status == IO_SUCCESS) {
            printf("\nController Information:\n");
            printf("  Vendor:Device ID: %04X:%04X\n",
                   ControllerInfo.VendorID, ControllerInfo.DeviceID);
            printf("  Protocol: SCSI-%d\n", ControllerInfo.Protocol);
            printf("  Max Targets: %u\n", ControllerInfo.MaxTargets);
            printf("  Max LUNs: %u\n", ControllerInfo.MaxLUNs);
            printf("  Max Transfer: %u bytes\n", ControllerInfo.MaxTransferSize);
            printf("  Queue Depth: %u\n", ControllerInfo.MaxQueueDepth);
            printf("  SAS Support: %s\n", ControllerInfo.bSASSupport ? "Yes" : "No");
            printf("  FC Support: %s\n", ControllerInfo.bFCSupport ? "Yes" : "No");
            printf("  FCoE Support: %s\n", ControllerInfo.bFCoESupport ? "Yes" : "No");
            printf("  Wide SCSI: %s\n", ControllerInfo.bWideSupport ? "Yes" : "No");
            printf("  Tagged Queuing: %s\n", ControllerInfo.bTaggedQueuing ? "Yes" : "No");
            printf("  Hot-plug: %s\n", ControllerInfo.bHotplug ? "Yes" : "No");
        }
    }
}

/**
 * @brief Example 2: Scan SCSI bus for devices
 *
 * This example demonstrates how to scan the SCSI bus and enumerate all devices.
 */
void
Example2_ScanSCSIBus(
    IIOSCSIController *pController
    )
{
    IO_RETURN Status;
    UINT32 DeviceCount;
    UINT32 i;
    IIOSCSIDevice *pDevice;
    SCSI_DEVICE_INFO DeviceInfo;

    printf("\n=== Example 2: Scan SCSI Bus ===\n");

    if (pController == NULL) {
        printf("No controller provided\n");
        return;
    }

    // Trigger bus scan
    printf("Scanning SCSI bus...\n");
    Status = IIOSCSIController_ScanBus(pController);
    if (Status != IO_SUCCESS) {
        printf("Bus scan failed: 0x%X\n", Status);
        return;
    }

    // Get device count
    Status = IIOSCSIController_GetDeviceCount(pController, &DeviceCount);
    if (Status != IO_SUCCESS) {
        printf("Failed to get device count: 0x%X\n", Status);
        return;
    }

    printf("Found %u SCSI device(s)\n\n", DeviceCount);

    // Example device output
    printf("Target 0, LUN 0: Seagate Cheetah 15K.7 600GB SAS\n");
    printf("  Device Type: Direct Access (Disk)\n");
    printf("  Capacity: 600 GB\n");
    printf("  Block Size: 512 bytes\n");
    printf("  Removable: No\n");
    printf("  SCSI Version: SPC-4\n");

    printf("\nTarget 1, LUN 0: HP Ultrium LTO-6 SAS Tape Drive\n");
    printf("  Device Type: Sequential Access (Tape)\n");
    printf("  Capacity: 2.5 TB native, 6.25 TB compressed\n");
    printf("  Removable: Yes\n");
    printf("  SCSI Version: SSC-4\n");

    printf("\nTarget 2, LUN 0: Plextor PX-712A DVD±RW\n");
    printf("  Device Type: CD/DVD\n");
    printf("  Removable: Yes\n");
    printf("  Write Protected: No\n");
}

/**
 * @brief Example 3: Execute SCSI INQUIRY command
 *
 * This example shows how to execute a SCSI INQUIRY command to get device information.
 */
void
Example3_ExecuteInquiryCommand(
    IIOSCSIController *pController
    )
{
    IO_RETURN Status;
    SCSI_COMMAND Command;
    SCSI_INQUIRY_DATA InquiryData;
    UINT32 TargetID = 0;
    UINT32 LUN = 0;

    printf("\n=== Example 3: Execute SCSI INQUIRY Command ===\n");

    if (pController == NULL) {
        printf("No controller provided\n");
        return;
    }

    // Build INQUIRY command (6-byte CDB)
    memset(&Command, 0, sizeof(SCSI_COMMAND));
    Command.CDB[0] = SCSI_CMD_INQUIRY;      // Opcode
    Command.CDB[1] = 0;                      // LUN (upper bits) + flags
    Command.CDB[2] = 0;                      // Page code
    Command.CDB[3] = 0;                      // Reserved
    Command.CDB[4] = sizeof(SCSI_INQUIRY_DATA); // Allocation length
    Command.CDB[5] = 0;                      // Control
    Command.CDBLength = 6;

    // Set data buffer
    Command.pDataBuffer = &InquiryData;
    Command.DataLength = sizeof(SCSI_INQUIRY_DATA);
    Command.bDataIn = TRUE;                 // Data transfer from device
    Command.TimeoutMs = 5000;               // 5 second timeout

    // Execute command
    printf("Sending INQUIRY to Target %u, LUN %u...\n", TargetID, LUN);
    Status = IIOSCSIController_SubmitCommand(pController, TargetID, LUN, &Command);

    if (Status == IO_SUCCESS && Command.Status == SCSI_STATUS_GOOD) {
        char VendorStr[9];
        char ProductStr[17];
        char RevisionStr[5];

        // Extract and null-terminate strings
        memcpy(VendorStr, InquiryData.VendorID, 8);
        VendorStr[8] = '\0';
        memcpy(ProductStr, InquiryData.ProductID, 16);
        ProductStr[16] = '\0';
        memcpy(RevisionStr, InquiryData.ProductRevision, 4);
        RevisionStr[4] = '\0';

        printf("INQUIRY succeeded:\n");
        printf("  Vendor:   %s\n", VendorStr);
        printf("  Product:  %s\n", ProductStr);
        printf("  Revision: %s\n", RevisionStr);
        printf("  Device Type: 0x%02X\n", InquiryData.DeviceType);
        printf("  Removable: %s\n", (InquiryData.RMB & 0x80) ? "Yes" : "No");
        printf("  SCSI Version: %u\n", InquiryData.Version);
    } else {
        printf("INQUIRY failed: Status=0x%X, SCSI Status=0x%02X\n",
               Status, Command.Status);

        if (Command.Status == SCSI_STATUS_CHECK_CONDITION) {
            printf("  Sense Key: 0x%02X\n", Command.SenseData.SenseKey);
            printf("  ASC/ASCQ: 0x%02X/0x%02X\n",
                   Command.SenseData.ASC, Command.SenseData.ASCQ);
        }
    }
}

/**
 * @brief Example 4: Read blocks from SCSI disk
 *
 * This example demonstrates how to read data from a SCSI direct-access device.
 */
void
Example4_ReadDiskBlocks(
    IIOSCSIDevice *pDevice
    )
{
    IO_RETURN Status;
    UINT8 Buffer[4096];  // 8 blocks of 512 bytes
    UINT64 LBA = 0;      // Starting logical block address
    UINT32 BlockCount = 8;
    SCSI_DEVICE_INFO DeviceInfo;

    printf("\n=== Example 4: Read Disk Blocks ===\n");

    if (pDevice == NULL) {
        printf("No device provided\n");
        return;
    }

    // Get device information
    Status = IIOSCSIDevice_GetDeviceInfo(pDevice, &DeviceInfo);
    if (Status != IO_SUCCESS) {
        printf("Failed to get device info: 0x%X\n", Status);
        return;
    }

    printf("Reading %u blocks from LBA %llu...\n", BlockCount, LBA);
    printf("Device: %s %s\n", DeviceInfo.VendorID, DeviceInfo.ProductID);
    printf("Block Size: %u bytes\n", DeviceInfo.BlockSize);
    printf("Total Blocks: %llu\n", DeviceInfo.TotalBlocks);

    // Read blocks
    Status = IIOSCSIDevice_ReadBlocks(pDevice, LBA, BlockCount,
                                       Buffer, sizeof(Buffer));

    if (Status == IO_SUCCESS) {
        printf("Read successful! Data (first 64 bytes):\n");
        for (UINT32 i = 0; i < 64; i++) {
            printf("%02X ", Buffer[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        printf("\n");
    } else {
        printf("Read failed: 0x%X\n", Status);
    }
}

/**
 * @brief Example 5: Write blocks to SCSI disk
 *
 * This example demonstrates how to write data to a SCSI direct-access device.
 */
void
Example5_WriteDiskBlocks(
    IIOSCSIDevice *pDevice
    )
{
    IO_RETURN Status;
    UINT8 Buffer[4096];  // 8 blocks of 512 bytes
    UINT64 LBA = 100;    // Starting logical block address
    UINT32 BlockCount = 8;
    SCSI_DEVICE_INFO DeviceInfo;

    printf("\n=== Example 5: Write Disk Blocks ===\n");

    if (pDevice == NULL) {
        printf("No device provided\n");
        return;
    }

    // Get device information
    Status = IIOSCSIDevice_GetDeviceInfo(pDevice, &DeviceInfo);
    if (Status != IO_SUCCESS) {
        printf("Failed to get device info: 0x%X\n", Status);
        return;
    }

    // Check if device is write-protected
    if (DeviceInfo.bWriteProtected) {
        printf("Device is write-protected!\n");
        return;
    }

    // Fill buffer with pattern
    for (UINT32 i = 0; i < sizeof(Buffer); i++) {
        Buffer[i] = (UINT8)(i & 0xFF);
    }

    printf("Writing %u blocks to LBA %llu...\n", BlockCount, LBA);

    // Write blocks
    Status = IIOSCSIDevice_WriteBlocks(pDevice, LBA, BlockCount,
                                        Buffer, sizeof(Buffer));

    if (Status == IO_SUCCESS) {
        printf("Write successful!\n");
    } else {
        printf("Write failed: 0x%X\n", Status);
    }
}

/**
 * @brief Example 6: SAS expander operations
 *
 * This example demonstrates SAS expander enumeration and management.
 */
void
Example6_SASExpanderOperations(
    IIOSCSIController *pController
    )
{
    SCSI_CONTROLLER_INFO ControllerInfo;
    IO_RETURN Status;

    printf("\n=== Example 6: SAS Expander Operations ===\n");

    if (pController == NULL) {
        printf("No controller provided\n");
        return;
    }

    // Get controller info
    Status = IIOSCSIController_GetControllerInfo(pController, &ControllerInfo);
    if (Status != IO_SUCCESS) {
        printf("Failed to get controller info\n");
        return;
    }

    if (!ControllerInfo.bSASSupport) {
        printf("Controller does not support SAS\n");
        return;
    }

    if (!ControllerInfo.bExpander) {
        printf("Controller does not support SAS expanders\n");
        return;
    }

    printf("SAS Expander topology:\n");
    printf("  HBA: LSI SAS3008 (12 Gbps SAS-3)\n");
    printf("    └─ Expander 0: 36-port SAS Expander\n");
    printf("        ├─ PHY 0-7: Direct-attached devices\n");
    printf("        │   ├─ Device 0: Seagate 600GB 15K SAS\n");
    printf("        │   ├─ Device 1: HGST 1.2TB 10K SAS\n");
    printf("        │   └─ Device 2: HP LTO-6 Tape Drive\n");
    printf("        └─ PHY 8-35: Fan-out to additional enclosures\n");
    printf("            └─ Enclosure 1: 24-bay JBOD\n");
    printf("                └─ 24x 4TB SATA drives\n");
}

/**
 * @brief Example 7: Fibre Channel operations
 *
 * This example demonstrates Fibre Channel HBA operations.
 */
void
Example7_FibreChannelOperations(
    IIOSCSIController *pController
    )
{
    SCSI_CONTROLLER_INFO ControllerInfo;
    IO_RETURN Status;

    printf("\n=== Example 7: Fibre Channel Operations ===\n");

    if (pController == NULL) {
        printf("No controller provided\n");
        return;
    }

    // Get controller info
    Status = IIOSCSIController_GetControllerInfo(pController, &ControllerInfo);
    if (Status != IO_SUCCESS) {
        printf("Failed to get controller info\n");
        return;
    }

    if (!ControllerInfo.bFCSupport) {
        printf("Controller does not support Fibre Channel\n");
        return;
    }

    printf("Fibre Channel HBA Information:\n");
    printf("  Speed: ");
    switch (ControllerInfo.FCMaxSpeed) {
        case FC_SPEED_1_GBPS:  printf("1 Gbps\n"); break;
        case FC_SPEED_2_GBPS:  printf("2 Gbps\n"); break;
        case FC_SPEED_4_GBPS:  printf("4 Gbps\n"); break;
        case FC_SPEED_8_GBPS:  printf("8 Gbps\n"); break;
        case FC_SPEED_16_GBPS: printf("16 Gbps\n"); break;
        case FC_SPEED_32_GBPS: printf("32 Gbps\n"); break;
        default: printf("Unknown\n"); break;
    }

    printf("  Topology: ");
    switch (ControllerInfo.FCTopology) {
        case FC_TOPOLOGY_P2P:    printf("Point-to-Point\n"); break;
        case FC_TOPOLOGY_FABRIC: printf("Fabric (Switched)\n"); break;
        case FC_TOPOLOGY_LOOP:   printf("Arbitrated Loop\n"); break;
        default: printf("Unknown\n"); break;
    }

    printf("  WWN:  %016llX\n", ControllerInfo.WWN);
    printf("  WWPN: %016llX\n", ControllerInfo.WWPN);
    printf("  FCoE: %s\n", ControllerInfo.bFCoESupport ? "Supported" : "Not supported");

    printf("\nFibre Channel Fabric Devices:\n");
    printf("  - Storage Array: EMC Clariion CX4-960\n");
    printf("  - Storage Array: NetApp FAS6280\n");
    printf("  - Tape Library: IBM TS3500 with 8x LTO-6 drives\n");
}

/**
 * @brief Example 8: Tape drive operations
 *
 * This example demonstrates tape drive operations (sequential access).
 */
void
Example8_TapeDriveOperations(
    IIOSCSIDevice *pTapeDrive
    )
{
    IO_RETURN Status;
    SCSI_DEVICE_INFO DeviceInfo;
    SCSI_COMMAND Command;

    printf("\n=== Example 8: Tape Drive Operations ===\n");

    if (pTapeDrive == NULL) {
        printf("No tape drive provided\n");
        return;
    }

    // Get device information
    Status = IIOSCSIDevice_GetDeviceInfo(pTapeDrive, &DeviceInfo);
    if (Status != IO_SUCCESS) {
        printf("Failed to get device info\n");
        return;
    }

    if (DeviceInfo.DeviceType != SCSI_DEVICE_SEQUENTIAL) {
        printf("Device is not a tape drive\n");
        return;
    }

    printf("Tape Drive: %s %s\n", DeviceInfo.VendorID, DeviceInfo.ProductID);
    printf("Revision: %s\n", DeviceInfo.Revision);

    // Example: Rewind tape
    printf("\nRewinding tape...\n");
    memset(&Command, 0, sizeof(SCSI_COMMAND));
    Command.CDB[0] = 0x01;  // REWIND command
    Command.CDBLength = 6;
    Command.TimeoutMs = 60000;  // 60 second timeout for rewind

    Status = IIOSCSIDevice_ExecuteCommand(pTapeDrive, &Command);
    if (Status == IO_SUCCESS) {
        printf("Tape rewound successfully\n");
    }

    // Example: Load/eject tape
    printf("\nEjecting tape...\n");
    memset(&Command, 0, sizeof(SCSI_COMMAND));
    Command.CDB[0] = SCSI_CMD_START_STOP_UNIT;
    Command.CDB[4] = 0x02;  // Eject
    Command.CDBLength = 6;
    Command.TimeoutMs = 30000;

    Status = IIOSCSIDevice_ExecuteCommand(pTapeDrive, &Command);
    if (Status == IO_SUCCESS) {
        printf("Tape ejected successfully\n");
    }
}

/**
 * @brief Main function - runs all examples
 */
int
main(
    int argc,
    char *argv[]
    )
{
    printf("===================================\n");
    printf("SCSI/SAS/FC Family Usage Examples\n");
    printf("===================================\n");

    // Run examples
    Example1_EnumerateSCSIControllers();
    Example2_ScanSCSIBus(NULL);
    Example3_ExecuteInquiryCommand(NULL);
    Example4_ReadDiskBlocks(NULL);
    Example5_WriteDiskBlocks(NULL);
    Example6_SASExpanderOperations(NULL);
    Example7_FibreChannelOperations(NULL);
    Example8_TapeDriveOperations(NULL);

    printf("\n===================================\n");
    printf("Examples completed\n");
    printf("===================================\n");

    return 0;
}
