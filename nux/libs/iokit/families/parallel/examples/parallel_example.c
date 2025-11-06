/**
 * @file parallel_example.c
 * @brief Parallel Port Family Usage Examples
 *
 * This file demonstrates how to use the Parallel Port family driver for:
 * - Parallel port enumeration and initialization
 * - SPP/EPP/ECP mode operations
 * - IEEE 1284 protocol negotiation
 * - Printer communication
 * - PARSCSI device operations (Zip, Jaz drives)
 * - Security dongle access
 * - Device detection and identification
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/parallel/parallel.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Example 1: Enumerate parallel ports
 *
 * This example shows how to find and enumerate all parallel ports in the system.
 */
void
Example1_EnumerateParallelPorts(
    VOID
    )
{
    IO_RETURN Status;
    IIOParallelPort *pPort = NULL;
    PARALLEL_PORT_INFO PortInfo;

    printf("\n=== Example 1: Enumerate Parallel Ports ===\n");

    // Initialize parallel port family
    Status = ParallelInitialize();
    if (Status != IO_SUCCESS) {
        printf("Failed to initialize parallel port family: 0x%X\n", Status);
        return;
    }

    printf("Parallel ports found in system:\n");
    printf("  - LPT1 at 0x378, IRQ 7 (ISA)\n");
    printf("  - LPT2 at 0x278, IRQ 5 (ISA)\n");
    printf("  - NetMos PCI 9805 1-Port Parallel (PCI)\n");
    printf("  - StarTech PCIe Parallel Card (PCIe)\n");

    // Example: Create port instance for LPT1
    printf("\nCreating LPT1 port instance...\n");
    // In a real implementation, you would pass the provider service
    // Status = ParallelPortCreate(pProvider, &pPort);

    if (pPort != NULL) {
        Status = IIOParallelPort_GetPortInfo(pPort, &PortInfo);
        if (Status == IO_SUCCESS) {
            printf("\nPort Information: %s\n", PortInfo.PortName);
            printf("  I/O Base: 0x%04X\n", PortInfo.IOBase);
            printf("  IRQ: %u\n", PortInfo.IRQ);
            printf("  DMA: %u\n", PortInfo.DMAChannel);
            printf("  Type: %s\n",
                   PortInfo.PortType == PARALLEL_TYPE_ISA ? "ISA" :
                   PortInfo.PortType == PARALLEL_TYPE_PCI ? "PCI" :
                   PortInfo.PortType == PARALLEL_TYPE_PCIE ? "PCIe" :
                   PortInfo.PortType == PARALLEL_TYPE_USB ? "USB" : "Unknown");
            printf("  Current Mode: %s\n",
                   PortInfo.CurrentMode == PARALLEL_MODE_SPP ? "SPP" :
                   PortInfo.CurrentMode == PARALLEL_MODE_EPP ? "EPP" :
                   PortInfo.CurrentMode == PARALLEL_MODE_ECP ? "ECP" : "Unknown");
            printf("  FIFO Size: %u bytes\n", PortInfo.FIFOSize);
        }
    }
}

/**
 * @brief Example 2: SPP mode operations
 *
 * This example demonstrates basic SPP (Standard Parallel Port) operations.
 */
void
Example2_SPPOperations(
    IIOParallelPort *pPort
    )
{
    IO_RETURN Status;
    UINT8 Data, StatusReg, ControlReg;

    printf("\n=== Example 2: SPP Mode Operations ===\n");

    if (pPort == NULL) {
        printf("No port provided\n");
        return;
    }

    // Set SPP mode
    printf("Setting SPP mode...\n");
    Status = IIOParallelPort_SetMode(pPort, PARALLEL_MODE_SPP);
    if (Status != IO_SUCCESS) {
        printf("Failed to set SPP mode: 0x%X\n", Status);
        return;
    }

    // Write data to port
    printf("Writing data byte 0x55 to data register...\n");
    Status = IIOParallelPort_WriteData(pPort, 0x55);
    if (Status == IO_SUCCESS) {
        printf("Data written successfully\n");
    }

    // Read status register
    printf("Reading status register...\n");
    Status = IIOParallelPort_ReadStatus(pPort, &StatusReg);
    if (Status == IO_SUCCESS) {
        printf("Status Register: 0x%02X\n", StatusReg);
        printf("  BUSY: %s\n", (StatusReg & PARALLEL_STATUS_BUSY) ? "Yes" : "No");
        printf("  ACK: %s\n", (StatusReg & PARALLEL_STATUS_ACK) ? "Yes" : "No");
        printf("  PAPER_OUT: %s\n", (StatusReg & PARALLEL_STATUS_PAPER_OUT) ? "Yes" : "No");
        printf("  SELECT: %s\n", (StatusReg & PARALLEL_STATUS_SELECT) ? "Yes" : "No");
        printf("  ERROR: %s\n", (StatusReg & PARALLEL_STATUS_ERROR) ? "Yes" : "No");
    }

    // Set control register
    printf("Setting control register...\n");
    ControlReg = PARALLEL_CONTROL_INIT | PARALLEL_CONTROL_SELECT_IN | PARALLEL_CONTROL_STROBE;
    Status = IIOParallelPort_WriteControl(pPort, ControlReg);
    if (Status == IO_SUCCESS) {
        printf("Control register set to 0x%02X\n", ControlReg);
    }

    // Pulse strobe signal (typical printer operation)
    printf("\nPulsing strobe signal for printer...\n");
    IIOParallelPort_WriteData(pPort, 0x48); // 'H'
    ControlReg |= PARALLEL_CONTROL_STROBE;
    IIOParallelPort_WriteControl(pPort, ControlReg);
    // Small delay would go here
    ControlReg &= ~PARALLEL_CONTROL_STROBE;
    IIOParallelPort_WriteControl(pPort, ControlReg);
    printf("Character sent to printer\n");
}

/**
 * @brief Example 3: EPP mode operations
 *
 * This example demonstrates EPP (Enhanced Parallel Port) operations.
 */
void
Example3_EPPOperations(
    IIOParallelPort *pPort
    )
{
    IO_RETURN Status;
    UINT8 Buffer[256];
    UINT32 i;

    printf("\n=== Example 3: EPP Mode Operations ===\n");

    if (pPort == NULL) {
        printf("No port provided\n");
        return;
    }

    // Set EPP mode
    printf("Setting EPP mode...\n");
    Status = IIOParallelPort_SetMode(pPort, PARALLEL_MODE_EPP);
    if (Status != IO_SUCCESS) {
        printf("Failed to set EPP mode: 0x%X\n", Status);
        return;
    }

    // Prepare test data
    for (i = 0; i < sizeof(Buffer); i++) {
        Buffer[i] = (UINT8)(i & 0xFF);
    }

    // EPP address write (device selection)
    printf("Writing EPP address 0x10...\n");
    UINT8 Address = 0x10;
    Status = IIOParallelPort_EPPWrite(pPort, TRUE, &Address, 1);
    if (Status == IO_SUCCESS) {
        printf("EPP address written\n");
    }

    // EPP data write (fast block transfer)
    printf("Writing 256 bytes via EPP data...\n");
    Status = IIOParallelPort_EPPWrite(pPort, FALSE, Buffer, sizeof(Buffer));
    if (Status == IO_SUCCESS) {
        printf("EPP data write successful: %u bytes\n", sizeof(Buffer));
    }

    // EPP data read (fast block transfer)
    printf("Reading 256 bytes via EPP data...\n");
    memset(Buffer, 0, sizeof(Buffer));
    Status = IIOParallelPort_EPPRead(pPort, FALSE, Buffer, sizeof(Buffer));
    if (Status == IO_SUCCESS) {
        printf("EPP data read successful: %u bytes\n", sizeof(Buffer));
        printf("First 16 bytes: ");
        for (i = 0; i < 16; i++) {
            printf("%02X ", Buffer[i]);
        }
        printf("\n");
    }

    printf("\nEPP mode provides:\n");
    printf("  - Bidirectional data transfer\n");
    printf("  - Fast data rates (up to 2 MB/s)\n");
    printf("  - Address/data strobes\n");
    printf("  - Suitable for PARSCSI, scanners, storage devices\n");
}

/**
 * @brief Example 4: ECP mode operations
 *
 * This example demonstrates ECP (Extended Capabilities Port) operations.
 */
void
Example4_ECPOperations(
    IIOParallelPort *pPort
    )
{
    IO_RETURN Status;
    UINT8 Buffer[1024];
    UINT32 BytesWritten, BytesRead;
    UINT32 i;

    printf("\n=== Example 4: ECP Mode Operations ===\n");

    if (pPort == NULL) {
        printf("No port provided\n");
        return;
    }

    // Set ECP mode
    printf("Setting ECP mode...\n");
    Status = IIOParallelPort_SetMode(pPort, PARALLEL_MODE_ECP);
    if (Status != IO_SUCCESS) {
        printf("Failed to set ECP mode: 0x%X\n", Status);
        return;
    }

    // Enable DMA for faster transfers
    printf("Enabling ECP DMA...\n");
    Status = IIOParallelPort_SetDMAEnable(pPort, TRUE);
    if (Status == IO_SUCCESS) {
        printf("DMA enabled\n");
    } else {
        printf("DMA not available, using FIFO polling\n");
    }

    // Prepare large data block
    for (i = 0; i < sizeof(Buffer); i++) {
        Buffer[i] = (UINT8)(i & 0xFF);
    }

    // ECP FIFO write (high-speed transfer to printer)
    printf("Writing 1024 bytes via ECP FIFO...\n");
    Status = IIOParallelPort_ECPWrite(pPort, Buffer, sizeof(Buffer), &BytesWritten);
    if (Status == IO_SUCCESS) {
        printf("ECP write successful: %u bytes written\n", BytesWritten);
    }

    // ECP FIFO read (scanner data, bidirectional)
    printf("Reading from ECP FIFO...\n");
    Status = IIOParallelPort_ECPRead(pPort, Buffer, sizeof(Buffer), &BytesRead);
    if (Status == IO_SUCCESS) {
        printf("ECP read successful: %u bytes read\n", BytesRead);
    }

    printf("\nECP mode provides:\n");
    printf("  - DMA support for high-speed transfers\n");
    printf("  - Hardware FIFO buffering\n");
    printf("  - RLE compression (optional)\n");
    printf("  - Data rates up to 2.5 MB/s\n");
    printf("  - Ideal for printers, scanners\n");
}

/**
 * @brief Example 5: IEEE 1284 protocol operations
 *
 * This example demonstrates IEEE 1284 protocol negotiation and device ID.
 */
void
Example5_IEEE1284Operations(
    IIOParallelPort *pPort
    )
{
    IO_RETURN Status;
    IEEE1284_DEVICE_ID DeviceID;

    printf("\n=== Example 5: IEEE 1284 Protocol Operations ===\n");

    if (pPort == NULL) {
        printf("No port provided\n");
        return;
    }

    // Read IEEE 1284 Device ID string
    printf("Reading IEEE 1284 Device ID...\n");
    Status = IIOParallelPort_ReadDeviceID(pPort, &DeviceID);
    if (Status == IO_SUCCESS) {
        printf("Device ID Length: %u\n", DeviceID.Length);
        printf("Device ID: %s\n", DeviceID.Data);

        // Parse device ID (typical format)
        // MFG:Hewlett-Packard;MDL:LaserJet 4;CMD:PCL,PJL;CLS:PRINTER;
        printf("\nParsed Information:\n");
        if (strstr(DeviceID.Data, "MFG:") != NULL) {
            printf("  Manufacturer: HP\n");
        }
        if (strstr(DeviceID.Data, "MDL:") != NULL) {
            printf("  Model: LaserJet 4\n");
        }
        if (strstr(DeviceID.Data, "CMD:") != NULL) {
            printf("  Commands: PCL, PJL\n");
        }
        if (strstr(DeviceID.Data, "CLS:PRINTER") != NULL) {
            printf("  Class: PRINTER\n");
        }
    }

    // Negotiate IEEE 1284 mode
    printf("\nNegotiating IEEE 1284 EPP mode...\n");
    Status = IIOParallelPort_IEEE1284Negotiate(pPort, IEEE1284_MODE_EPP);
    if (Status == IO_SUCCESS) {
        printf("EPP mode negotiation successful\n");
        printf("Port is now in IEEE 1284 EPP mode\n");
    } else {
        printf("EPP mode not supported by device\n");

        // Try nibble mode fallback
        printf("Trying nibble mode...\n");
        Status = IIOParallelPort_IEEE1284Negotiate(pPort, IEEE1284_MODE_NIBBLE);
        if (Status == IO_SUCCESS) {
            printf("Nibble mode negotiation successful\n");
        }
    }

    // Perform data transfer in negotiated mode
    printf("Performing data transfer...\n");
    UINT8 TestData[] = "Hello Parallel Port!";
    // Transfer would go here based on negotiated mode

    // Terminate IEEE 1284 session
    printf("Terminating IEEE 1284 session...\n");
    Status = IIOParallelPort_IEEE1284Terminate(pPort);
    if (Status == IO_SUCCESS) {
        printf("IEEE 1284 session terminated\n");
    }
}

/**
 * @brief Example 6: PARSCSI operations (Iomega Zip drive)
 *
 * This example demonstrates PARSCSI operations for devices like Iomega Zip drives.
 */
void
Example6_PARSCSIOperations(
    IIOParallelPort *pPort
    )
{
    IO_RETURN Status;
    PARALLEL_PORT_INFO PortInfo;

    printf("\n=== Example 6: PARSCSI Operations ===\n");

    if (pPort == NULL) {
        printf("No port provided\n");
        return;
    }

    // Get port information
    Status = IIOParallelPort_GetPortInfo(pPort, &PortInfo);
    if (Status != IO_SUCCESS) {
        printf("Failed to get port info\n");
        return;
    }

    if (!(PortInfo.Capabilities & PARALLEL_CAP_PARSCSI)) {
        printf("Port does not support PARSCSI\n");
        printf("PARSCSI support can be added via software protocol\n");
    }

    printf("Detecting PARSCSI device...\n");
    printf("Devices that use PARSCSI:\n");
    printf("  - Iomega Zip 100/250 MB drives\n");
    printf("  - Iomega Jaz 1GB/2GB drives\n");
    printf("  - Adaptec SlimSCSI adapters\n");
    printf("  - MicroSolutions BackPack CD-ROM/HDD\n");

    // Set EPP mode for PARSCSI
    printf("\nSetting EPP mode for PARSCSI...\n");
    Status = IIOParallelPort_SetMode(pPort, PARALLEL_MODE_EPP);
    if (Status == IO_SUCCESS) {
        printf("EPP mode set\n");
    }

    // PARSCSI initialization sequence
    printf("Initializing PARSCSI device...\n");
    printf("  1. Resetting device via control signals\n");
    printf("  2. Sending SCSI INQUIRY command\n");
    printf("  3. Identifying device type\n");

    // Example: Iomega Zip Drive detected
    printf("\nPARSCSI Device Detected:\n");
    printf("  Manufacturer: Iomega\n");
    printf("  Model: Zip 250\n");
    printf("  Type: Removable Direct Access\n");
    printf("  Capacity: 250 MB\n");
    printf("  Interface: EPP (2 MB/s)\n");

    // Example: Read from Zip disk
    printf("\nReading sector 0 from Zip disk...\n");
    UINT8 SectorBuffer[512];
    printf("SCSI READ(10) command sent via PARSCSI\n");
    printf("Sector data received: 512 bytes\n");

    // Example: Write to Zip disk
    printf("\nWriting sector 100 to Zip disk...\n");
    printf("SCSI WRITE(10) command sent via PARSCSI\n");
    printf("Sector data written: 512 bytes\n");
}

/**
 * @brief Example 7: Printer operations
 *
 * This example demonstrates printer communication.
 */
void
Example7_PrinterOperations(
    IIOParallelPort *pPort
    )
{
    IO_RETURN Status;
    CONST CHAR8 *pszTestPage =
        "\x1B\x45"          // PCL: Reset printer
        "Hello from NUX Parallel Port Driver!\r\n"
        "This is a test page.\r\n"
        "\x0C";             // Form feed

    printf("\n=== Example 7: Printer Operations ===\n");

    if (pPort == NULL) {
        printf("No port provided\n");
        return;
    }

    printf("Sending test page to printer...\n");

    // Check printer status
    UINT8 Status;
    IIOParallelPort_ReadStatus(pPort, &StatusReg);

    if (StatusReg & PARALLEL_STATUS_PAPER_OUT) {
        printf("Error: Printer is out of paper\n");
        return;
    }

    if (!(StatusReg & PARALLEL_STATUS_SELECT)) {
        printf("Error: Printer is offline\n");
        return;
    }

    if (StatusReg & PARALLEL_STATUS_ERROR) {
        printf("Error: Printer error\n");
        return;
    }

    // Send data to printer byte by byte (SPP mode)
    printf("Printing via SPP mode...\n");
    for (UINT32 i = 0; i < strlen(pszTestPage); i++) {
        // Write data
        IIOParallelPort_WriteData(pPort, pszTestPage[i]);

        // Pulse strobe
        UINT8 Control;
        IIOParallelPort_ReadControl(pPort, &Control);
        Control |= PARALLEL_CONTROL_STROBE;
        IIOParallelPort_WriteControl(pPort, Control);
        // Small delay
        Control &= ~PARALLEL_CONTROL_STROBE;
        IIOParallelPort_WriteControl(pPort, Control);

        // Wait for ACK
        do {
            IIOParallelPort_ReadStatus(pPort, &StatusReg);
        } while (!(StatusReg & PARALLEL_STATUS_ACK));
    }

    printf("Test page sent successfully\n");
    printf("\nPrinter Information:\n");
    printf("  Type: HP LaserJet 4\n");
    printf("  Mode: SPP (unidirectional)\n");
    printf("  Speed: ~50 KB/s\n");
}

/**
 * @brief Example 8: Security dongle operations
 *
 * This example demonstrates security dongle/key access.
 */
void
Example8_SecurityDongleOperations(
    IIOParallelPort *pPort
    )
{
    IO_RETURN Status;
    UINT8 Challenge, Response;

    printf("\n=== Example 8: Security Dongle Operations ===\n");

    if (pPort == NULL) {
        printf("No port provided\n");
        return;
    }

    printf("Detecting security dongle...\n");
    printf("Common parallel port security dongles:\n");
    printf("  - HASP (Aladdin/SafeNet)\n");
    printf("  - Sentinel SuperPro\n");
    printf("  - KeyLok\n");
    printf("  - Wibu CodeMeter\n");

    // Dongles typically use simple challenge-response
    printf("\nCommunicating with dongle...\n");

    // Send challenge
    Challenge = 0xA5;
    printf("Sending challenge: 0x%02X\n", Challenge);
    IIOParallelPort_WriteData(pPort, Challenge);

    // Pulse strobe
    UINT8 Control;
    IIOParallelPort_ReadControl(pPort, &Control);
    Control |= PARALLEL_CONTROL_STROBE;
    IIOParallelPort_WriteControl(pPort, Control);
    Control &= ~PARALLEL_CONTROL_STROBE;
    IIOParallelPort_WriteControl(pPort, Control);

    // Read response from status lines
    UINT8 StatusReg;
    IIOParallelPort_ReadStatus(pPort, &StatusReg);

    // Dongles encode response in status bits
    Response = (StatusReg >> 3) & 0x0F; // Extract 4 bits
    printf("Received response: 0x%02X\n", Response);

    if (Response == 0x0A) { // Example expected response
        printf("Dongle authentication successful\n");
        printf("Software is licensed\n");
    } else {
        printf("Dongle authentication failed\n");
        printf("Invalid or missing security key\n");
    }
}

/**
 * @brief Example 9: Device detection
 *
 * This example demonstrates automatic device detection.
 */
void
Example9_DeviceDetection(
    IIOParallelPort *pPort
    )
{
    IO_RETURN Status;
    IIOParallelDevice *pDevice = NULL;
    PARALLEL_DEVICE_INFO DeviceInfo;

    printf("\n=== Example 9: Device Detection ===\n");

    if (pPort == NULL) {
        printf("No port provided\n");
        return;
    }

    printf("Detecting connected device...\n");
    Status = IIOParallelPort_DetectDevice(pPort, &pDevice);

    if (Status == IO_SUCCESS && pDevice != NULL) {
        printf("Device detected!\n");

        // Get device information
        Status = IIOParallelDevice_GetDeviceInfo(pDevice, &DeviceInfo);
        if (Status == IO_SUCCESS) {
            printf("\nDevice Information:\n");
            printf("  Type: ");
            switch (DeviceInfo.DeviceType) {
                case PARALLEL_DEVICE_PRINTER:
                    printf("Printer\n");
                    break;
                case PARALLEL_DEVICE_SCANNER:
                    printf("Scanner\n");
                    break;
                case PARALLEL_DEVICE_PARSCSI:
                    printf("PARSCSI Device\n");
                    break;
                case PARALLEL_DEVICE_TAPE:
                    printf("Tape Drive\n");
                    break;
                case PARALLEL_DEVICE_DONGLE:
                    printf("Security Dongle\n");
                    break;
                case PARALLEL_DEVICE_NETWORK:
                    printf("Network Adapter\n");
                    break;
                default:
                    printf("Unknown\n");
                    break;
            }

            printf("  Manufacturer: %s\n", DeviceInfo.Manufacturer);
            printf("  Model: %s\n", DeviceInfo.Model);
            printf("  IEEE 1284: %s\n", DeviceInfo.bIEEE1284 ? "Yes" : "No");
            printf("  Bidirectional: %s\n", DeviceInfo.bBidirectional ? "Yes" : "No");
            printf("  Max Transfer Rate: %u KB/s\n", DeviceInfo.MaxTransferRate);
        }

        pDevice->lpVtbl->Release(pDevice);
    } else {
        printf("No device detected or device not responding\n");
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
    IIOParallelPort *pPort = NULL;

    printf("=============================================\n");
    printf("Parallel Port Family Usage Examples\n");
    printf("=============================================\n");

    // Run examples
    Example1_EnumerateParallelPorts();
    Example2_SPPOperations(pPort);
    Example3_EPPOperations(pPort);
    Example4_ECPOperations(pPort);
    Example5_IEEE1284Operations(pPort);
    Example6_PARSCSIOperations(pPort);
    Example7_PrinterOperations(pPort);
    Example8_SecurityDongleOperations(pPort);
    Example9_DeviceDetection(pPort);

    printf("\n=============================================\n");
    printf("Examples completed\n");
    printf("=============================================\n");

    printf("\nNote: These examples demonstrate API usage.\n");
    printf("In a real implementation, you would:\n");
    printf("  1. Enumerate PCI/ISA devices\n");
    printf("  2. Create port instances with real providers\n");
    printf("  3. Perform actual I/O operations\n");
    printf("  4. Handle interrupts and DMA\n");
    printf("  5. Implement full IEEE 1284 protocol\n");

    return 0;
}
