/**
 * @file ata_example.c
 * @brief ATA/IDE Family Usage Examples
 *
 * This file demonstrates how to use the ATA/IDE family driver interface
 * to interact with ATA/ATAPI devices, including hard drives, CD/DVD drives,
 * and other ATAPI devices.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/families/ata/ata.h>
#include <iokit/families/pcie/pcie.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Example 1: Enumerate ATA controllers
 *
 * This example shows how to enumerate all ATA/IDE controllers in the system.
 */
void
Example1_EnumerateControllers(
    VOID
    )
{
    printf("=== Example 1: Enumerate ATA Controllers ===\n");
    printf("This example would enumerate all ATA/IDE controllers.\n");
    printf("In a real implementation:\n");
    printf("  1. Query IORegistry for PCI devices\n");
    printf("  2. Filter for Class 01h, Subclass 01h (IDE/ATA)\n");
    printf("  3. Create IIOATAController for each device\n");
    printf("  4. Retrieve controller information\n\n");
}

/**
 * @brief Example 2: Get controller information
 *
 * Shows how to retrieve detailed information about an ATA controller.
 */
void
Example2_GetControllerInfo(
    IIOATAController *pController
    )
{
    ATA_CONTROLLER_INFO ControllerInfo;
    IO_RETURN Status;

    printf("=== Example 2: Get Controller Information ===\n");

    if (pController == NULL) {
        printf("No controller provided for example.\n\n");
        return;
    }

    // Get controller information
    Status = IIOATAController_GetControllerInfo(pController, &ControllerInfo);
    if (Status != IO_SUCCESS) {
        printf("Failed to get controller info: 0x%X\n", Status);
        return;
    }

    // Display controller information
    printf("Controller Information:\n");
    printf("  Vendor:      %s\n", ControllerInfo.VendorName);
    printf("  Model:       %s\n", ControllerInfo.ControllerName);
    printf("  Vendor ID:   0x%04X\n", ControllerInfo.VendorID);
    printf("  Device ID:   0x%04X\n", ControllerInfo.DeviceID);
    printf("  Protocol:    ");

    switch (ControllerInfo.Protocol) {
        case ATA_PROTOCOL_ST506:
            printf("ST506/ST412 (MFM/RLL)\n");
            break;
        case ATA_PROTOCOL_ESDI:
            printf("ESDI\n");
            break;
        case ATA_PROTOCOL_IDE:
            printf("IDE/ATA-1\n");
            break;
        case ATA_PROTOCOL_EIDE:
            printf("EIDE/ATA-2\n");
            break;
        case ATA_PROTOCOL_ATAPI:
            printf("ATAPI/ATA-3\n");
            break;
        case ATA_PROTOCOL_UDMA33:
            printf("Ultra DMA/33 (ATA-4)\n");
            break;
        case ATA_PROTOCOL_UDMA66:
            printf("Ultra DMA/66 (ATA-5)\n");
            break;
        case ATA_PROTOCOL_UDMA100:
            printf("Ultra DMA/100 (ATA-6)\n");
            break;
        case ATA_PROTOCOL_UDMA133:
            printf("Ultra DMA/133 (ATA-7)\n");
            break;
        default:
            printf("Unknown\n");
            break;
    }

    printf("  Channels:    %u\n", ControllerInfo.NumChannels);
    printf("  Dev/Channel: %u\n", ControllerInfo.NumDevicesPerChannel);
    printf("  DMA Support: %s\n", ControllerInfo.bDMASupported ? "Yes" : "No");
    printf("  UDMA Mode:   %u (up to %u MB/s)\n",
           ControllerInfo.MaxUdmaMode,
           (16 << ControllerInfo.MaxUdmaMode) + (ControllerInfo.MaxUdmaMode > 2 ?
            ((ControllerInfo.MaxUdmaMode - 2) * 17) : 0));
    printf("\n");
}

/**
 * @brief Example 3: Scan for devices
 *
 * Demonstrates how to scan the ATA bus for devices and enumerate them.
 */
void
Example3_ScanDevices(
    IIOATAController *pController
    )
{
    IO_RETURN Status;
    UINT32 uDeviceCount;
    UINT32 uChannel, uDevice;
    IIOATADevice *pATADevice;
    ATA_DEVICE_INFO DeviceInfo;

    printf("=== Example 3: Scan for ATA Devices ===\n");

    if (pController == NULL) {
        printf("No controller provided for example.\n\n");
        return;
    }

    // Scan the bus
    Status = IIOATAController_ScanBus(pController);
    if (Status != IO_SUCCESS) {
        printf("Bus scan failed: 0x%X\n", Status);
        return;
    }

    // Get device count
    Status = IIOATAController_GetDeviceCount(pController, &uDeviceCount);
    if (Status != IO_SUCCESS) {
        printf("Failed to get device count: 0x%X\n", Status);
        return;
    }

    printf("Scanning for devices...\n");

    // Enumerate all possible device locations
    for (uChannel = 0; uChannel < 2; uChannel++) {
        for (uDevice = 0; uDevice < 2; uDevice++) {
            // Try to get device
            Status = IIOATAController_GetDevice(pController, uChannel, uDevice, &pATADevice);
            if (Status == IO_SUCCESS && pATADevice != NULL) {
                // Get device information
                Status = IIOATADevice_GetDeviceInfo(pATADevice, &DeviceInfo);
                if (Status == IO_SUCCESS) {
                    printf("\nDevice at Channel %u, Device %u (%s):\n",
                           uChannel, uDevice, DeviceInfo.bMaster ? "Master" : "Slave");
                    printf("  Model:         %s\n", DeviceInfo.Model);
                    printf("  Serial:        %s\n", DeviceInfo.SerialNumber);
                    printf("  Firmware:      %s\n", DeviceInfo.FirmwareRevision);
                    printf("  Type:          ");

                    switch (DeviceInfo.DeviceType) {
                        case ATA_DEVICE_HDD:
                            printf("Hard Disk Drive\n");
                            break;
                        case ATA_DEVICE_CDROM:
                            printf("CD-ROM\n");
                            break;
                        case ATA_DEVICE_DVDROM:
                            printf("DVD-ROM\n");
                            break;
                        case ATA_DEVICE_CDRW:
                            printf("CD-RW\n");
                            break;
                        case ATA_DEVICE_DVDRW:
                            printf("DVD±RW\n");
                            break;
                        case ATA_DEVICE_ZIP:
                            printf("Zip Drive\n");
                            break;
                        default:
                            printf("Unknown\n");
                            break;
                    }

                    printf("  ATAPI:         %s\n", DeviceInfo.bAtapi ? "Yes" : "No");
                    printf("  Capacity:      %llu MB (%llu sectors)\n",
                           DeviceInfo.TotalCapacity / (1024 * 1024),
                           DeviceInfo.TotalSectors);
                    printf("  LBA Support:   %s\n", DeviceInfo.bLBA ? "Yes" : "No");
                    printf("  LBA48 Support: %s\n", DeviceInfo.bLBA48 ? "Yes" : "No");
                    printf("  DMA Support:   %s\n", DeviceInfo.bDMA ? "Yes" : "No");
                    printf("  UDMA Support:  %s\n", DeviceInfo.bUDMA ? "Yes" : "No");
                    if (DeviceInfo.bUDMA) {
                        printf("  UDMA Mode:     %u\n", DeviceInfo.MaxUdmaMode);
                    }
                    printf("  S.M.A.R.T.:    %s\n", DeviceInfo.bSMART ? "Yes" : "No");
                }

                // Release device interface
                pATADevice->lpVtbl->Release(pATADevice);
            }
        }
    }

    printf("\n");
}

/**
 * @brief Example 4: Read sectors from ATA drive
 *
 * Shows how to read sectors from an ATA hard drive.
 */
void
Example4_ReadSectors(
    IIOATADevice *pDevice
    )
{
    UINT8 Buffer[512 * 8];  // 8 sectors
    IO_RETURN Status;
    UINT32 i;

    printf("=== Example 4: Read Sectors ===\n");

    if (pDevice == NULL) {
        printf("No device provided for example.\n\n");
        return;
    }

    // Read 8 sectors starting at LBA 0
    printf("Reading 8 sectors from LBA 0...\n");
    Status = IIOATADevice_ReadSectors(pDevice, 0, 8, Buffer, sizeof(Buffer));
    if (Status != IO_SUCCESS) {
        printf("Read failed: 0x%X\n", Status);
        return;
    }

    printf("Read successful. First 128 bytes:\n");
    for (i = 0; i < 128; i++) {
        printf("%02X ", Buffer[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

/**
 * @brief Example 5: Execute ATA IDENTIFY command
 *
 * Demonstrates executing the ATA IDENTIFY DEVICE command.
 */
void
Example5_IdentifyDevice(
    IIOATADevice *pDevice
    )
{
    ATA_IDENTIFY_DATA IdentifyData;
    IO_RETURN Status;
    CHAR8 ModelStr[41];
    CHAR8 SerialStr[21];
    CHAR8 FirmwareStr[9];

    printf("=== Example 5: Execute IDENTIFY DEVICE ===\n");

    if (pDevice == NULL) {
        printf("No device provided for example.\n\n");
        return;
    }

    // Execute IDENTIFY DEVICE command
    printf("Executing IDENTIFY DEVICE command...\n");
    Status = IIOATADevice_IdentifyDevice(pDevice, &IdentifyData);
    if (Status != IO_SUCCESS) {
        printf("IDENTIFY failed: 0x%X\n", Status);
        return;
    }

    // Parse model number (byte-swapped)
    memcpy(ModelStr, IdentifyData.ModelNumber, 40);
    ModelStr[40] = '\0';

    // Parse serial number (byte-swapped)
    memcpy(SerialStr, IdentifyData.SerialNumber, 20);
    SerialStr[20] = '\0';

    // Parse firmware revision (byte-swapped)
    memcpy(FirmwareStr, IdentifyData.FirmwareRevision, 8);
    FirmwareStr[8] = '\0';

    printf("IDENTIFY Data:\n");
    printf("  Model Number:     %s\n", ModelStr);
    printf("  Serial Number:    %s\n", SerialStr);
    printf("  Firmware Rev:     %s\n", FirmwareStr);
    printf("  Capabilities:     0x%04X\n", IdentifyData.Capabilities1);
    printf("  LBA Sectors (28): %u\n", IdentifyData.TotalAddressableSectors);
    printf("  LBA Sectors (48): %llu\n", IdentifyData.TotalAddressableSectors48);
    printf("  UDMA Modes:       0x%04X\n", IdentifyData.UltraDmaMode);
    printf("  Major Version:    0x%04X\n", IdentifyData.MajorVersion);
    printf("\n");
}

/**
 * @brief Example 6: Set transfer mode
 *
 * Shows how to configure the transfer mode for a device.
 */
void
Example6_SetTransferMode(
    IIOATADevice *pDevice
    )
{
    IO_RETURN Status;

    printf("=== Example 6: Set Transfer Mode ===\n");

    if (pDevice == NULL) {
        printf("No device provided for example.\n\n");
        return;
    }

    // Set transfer mode to UDMA Mode 5 (100 MB/s)
    printf("Setting transfer mode to UDMA Mode 5...\n");
    Status = IIOATADevice_SetTransferMode(pDevice, ATA_TRANSFER_UDMA, ATA_UDMA_MODE_5);
    if (Status == IO_SUCCESS) {
        printf("Transfer mode set successfully.\n");
    } else if (Status == IO_UNSUPPORTED) {
        printf("UDMA Mode 5 not supported by device.\n");
    } else {
        printf("Failed to set transfer mode: 0x%X\n", Status);
    }

    // Try UDMA Mode 2 (33 MB/s) as fallback
    printf("Setting transfer mode to UDMA Mode 2...\n");
    Status = IIOATADevice_SetTransferMode(pDevice, ATA_TRANSFER_UDMA, ATA_UDMA_MODE_2);
    if (Status == IO_SUCCESS) {
        printf("Transfer mode set successfully.\n");
    } else {
        printf("Failed to set transfer mode: 0x%X\n", Status);
    }

    printf("\n");
}

/**
 * @brief Example 7: ATAPI packet command (CD-ROM)
 *
 * Demonstrates executing an ATAPI packet command for a CD/DVD drive.
 */
void
Example7_ATAPIPacket(
    IIOATADevice *pDevice
    )
{
    UINT8 Packet[12];
    UINT8 Buffer[2048];  // One CD sector
    IO_RETURN Status;

    printf("=== Example 7: Execute ATAPI Packet Command ===\n");

    if (pDevice == NULL) {
        printf("No device provided for example.\n\n");
        return;
    }

    // Build ATAPI READ(10) command to read sector 16 (first data sector)
    memset(Packet, 0, sizeof(Packet));
    Packet[0] = 0x28;  // READ(10) opcode
    Packet[2] = 0x00;  // LBA MSB
    Packet[3] = 0x00;
    Packet[4] = 0x00;
    Packet[5] = 0x10;  // LBA LSB (sector 16)
    Packet[7] = 0x00;  // Transfer length MSB
    Packet[8] = 0x01;  // Transfer length LSB (1 sector)

    printf("Executing ATAPI READ command for sector 16...\n");
    Status = IIOATADevice_ExecutePacket(pDevice, Packet, 12, Buffer, sizeof(Buffer), TRUE);
    if (Status == IO_SUCCESS) {
        printf("ATAPI command successful. First 64 bytes:\n");
        for (UINT32 i = 0; i < 64; i++) {
            printf("%02X ", Buffer[i]);
            if ((i + 1) % 16 == 0) {
                printf("\n");
            }
        }
    } else if (Status == IO_UNSUPPORTED) {
        printf("Device is not an ATAPI device.\n");
    } else {
        printf("ATAPI command failed: 0x%X\n", Status);
    }

    printf("\n");
}

/**
 * @brief Example 8: Flush cache
 *
 * Shows how to flush the device write cache.
 */
void
Example8_FlushCache(
    IIOATADevice *pDevice
    )
{
    IO_RETURN Status;

    printf("=== Example 8: Flush Cache ===\n");

    if (pDevice == NULL) {
        printf("No device provided for example.\n\n");
        return;
    }

    printf("Flushing device cache...\n");
    Status = IIOATADevice_FlushCache(pDevice);
    if (Status == IO_SUCCESS) {
        printf("Cache flushed successfully.\n");
    } else {
        printf("Cache flush failed: 0x%X\n", Status);
    }

    printf("\n");
}

/**
 * @brief Example 9: Execute raw ATA command
 *
 * Demonstrates executing a raw ATA command structure.
 */
void
Example9_RawATACommand(
    IIOATAController *pController
    )
{
    ATA_COMMAND Command;
    UINT8 Buffer[512];
    IO_RETURN Status;

    printf("=== Example 9: Execute Raw ATA Command ===\n");

    if (pController == NULL) {
        printf("No controller provided for example.\n\n");
        return;
    }

    // Build IDENTIFY DEVICE command
    memset(&Command, 0, sizeof(Command));
    Command.Command = ATA_CMD_IDENTIFY;
    Command.DeviceHead = 0xA0;  // Master device, LBA mode
    Command.pDataBuffer = Buffer;
    Command.DataLength = 512;
    Command.bDataIn = TRUE;
    Command.TimeoutMs = 5000;

    printf("Executing raw IDENTIFY DEVICE command...\n");
    Status = IIOATAController_SubmitCommand(pController, 0, 0, &Command);
    if (Status == IO_SUCCESS) {
        printf("Command executed successfully.\n");
        printf("Status: 0x%02X, Error: 0x%02X\n", Command.Status, Command.Error);
    } else {
        printf("Command failed: 0x%X\n", Status);
    }

    printf("\n");
}

/**
 * @brief Main function - Run all examples
 */
int
main(
    int argc,
    char *argv[]
    )
{
    printf("========================================\n");
    printf("ATA/IDE Family Driver Usage Examples\n");
    printf("========================================\n\n");

    // Run examples (with NULL devices for demonstration)
    Example1_EnumerateControllers();
    Example2_GetControllerInfo(NULL);
    Example3_ScanDevices(NULL);
    Example4_ReadSectors(NULL);
    Example5_IdentifyDevice(NULL);
    Example6_SetTransferMode(NULL);
    Example7_ATAPIPacket(NULL);
    Example8_FlushCache(NULL);
    Example9_RawATACommand(NULL);

    printf("========================================\n");
    printf("Examples Complete\n");
    printf("========================================\n");

    return 0;
}
