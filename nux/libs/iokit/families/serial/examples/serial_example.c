/**
 * @file serial_example.c
 * @brief Serial Port Family Usage Examples
 *
 * This example demonstrates:
 * - Serial port enumeration and configuration
 * - Data transmission and reception
 * - Modem control line management
 * - UART type detection
 * - Communication with serial devices
 * - Flow control and error handling
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/serial/serial.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief Example 1: Initialize serial subsystem and enumerate ports
 */
static int
Example_Serial_Initialization(void)
{
    IO_RETURN Status;
    IIOSerialPort *pPorts[32];
    UINT32 uPortCount;
    UINT32 i;

    printf("\n");
    printf("========================================\n");
    printf("  Example 1: Serial Port Enumeration\n");
    printf("========================================\n\n");

    // Initialize serial subsystem
    printf("Initializing serial port subsystem...\n");
    Status = SerialInitialize();
    if (Status != IO_SUCCESS) {
        printf("ERROR: Serial initialization failed (status=0x%08X)\n", Status);
        return -1;
    }
    printf("Serial subsystem initialized successfully\n\n");

    // Enumerate all available serial ports
    printf("Enumerating serial ports...\n");
    uPortCount = 32;
    Status = SerialEnumeratePorts(pPorts, &uPortCount);
    if (Status != IO_SUCCESS) {
        printf("ERROR: Port enumeration failed (status=0x%08X)\n", Status);
        return -1;
    }

    printf("Found %u serial port(s)\n\n", uPortCount);

    // Display information about each port
    for (i = 0; i < uPortCount; i++) {
        SERIAL_PORT_INFO PortInfo;

        Status = IIOSerialPort_GetPortInfo(pPorts[i], &PortInfo);
        if (Status == IO_SUCCESS) {
            printf("Port %u:\n", i);
            printf("  Name:        %s\n", PortInfo.PortName);
            printf("  Device:      %s\n", PortInfo.DevicePath);
            printf("  COM Port:    COM%u\n", PortInfo.COMPort);
            printf("  Baud Rate:   %u bps\n", PortInfo.Config.BaudRate);
            printf("  Data Bits:   %u\n", PortInfo.Config.DataBits);
            printf("  Stop Bits:   %u\n", PortInfo.Config.StopBits == STOP_BITS_1 ? 1 : 2);
            printf("  Parity:      ");
            switch (PortInfo.Config.Parity) {
                case PARITY_NONE:  printf("None\n"); break;
                case PARITY_EVEN:  printf("Even\n"); break;
                case PARITY_ODD:   printf("Odd\n"); break;
                case PARITY_MARK:  printf("Mark\n"); break;
                case PARITY_SPACE: printf("Space\n"); break;
            }
            printf("  Status:      %s\n", PortInfo.bOpen ? "Open" : "Closed");
            printf("\n");
        }

        // Release port reference
        IIOSerialPort_Release(pPorts[i]);
    }

    return 0;
}

/**
 * @brief Example 2: Configure and use a serial port
 */
static int
Example_Serial_Configuration(void)
{
    IO_RETURN Status;
    IIOSerialPort *pPort = NULL;
    SERIAL_CONFIG Config;
    UINT8 uModemStatus;

    printf("\n");
    printf("========================================\n");
    printf("  Example 2: Serial Port Configuration\n");
    printf("========================================\n\n");

    // Create a serial port instance for COM1
    printf("Opening COM1...\n");
    Status = SerialPortCreate(1, &pPort);
    if (Status != IO_SUCCESS) {
        printf("Failed to create COM1 (status=0x%08X)\n", Status);
        return 0;  // Not an error, just not available
    }

    printf("COM1 created successfully\n\n");

    // Configure the port: 9600 baud, 8N1, no flow control
    printf("Configuring port (9600 8N1, no flow control)...\n");
    Config.BaudRate = BAUD_9600;
    Config.DataBits = DATA_BITS_8;
    Config.StopBits = STOP_BITS_1;
    Config.Parity = PARITY_NONE;
    Config.FlowControl = FLOW_CONTROL_NONE;
    Config.Protocol = SERIAL_PROTOCOL_RS232;
    Config.bEnableFIFO = TRUE;
    Config.FIFOTriggerLevel = 8;

    Status = IIOSerialPort_Configure(pPort, &Config);
    if (Status != IO_SUCCESS) {
        printf("Configuration failed (status=0x%08X)\n", Status);
        IIOSerialPort_Release(pPort);
        return -1;
    }
    printf("Port configured successfully\n\n");

    // Open the port
    printf("Opening port for communication...\n");
    Status = IIOSerialPort_Open(pPort);
    if (Status != IO_SUCCESS) {
        printf("Failed to open port (status=0x%08X)\n", Status);
        IIOSerialPort_Release(pPort);
        return -1;
    }
    printf("Port opened successfully\n\n");

    // Set modem control lines (DTR and RTS high)
    printf("Setting modem control lines (DTR=1, RTS=1)...\n");
    Status = IIOSerialPort_SetModemControl(pPort, MCR_DTR | MCR_RTS);
    if (Status == IO_SUCCESS) {
        printf("Modem control lines set\n");
    }

    // Get modem status
    Status = IIOSerialPort_GetModemStatus(pPort, &uModemStatus);
    if (Status == IO_SUCCESS) {
        printf("Modem status: 0x%02X\n", uModemStatus);
        if (uModemStatus & MSR_CTS)  printf("  CTS is active\n");
        if (uModemStatus & MSR_DSR)  printf("  DSR is active\n");
        if (uModemStatus & MSR_RI)   printf("  RI is active\n");
        if (uModemStatus & MSR_DCD)  printf("  DCD is active\n");
    }

    printf("\n");

    // Close and release port
    IIOSerialPort_Close(pPort);
    IIOSerialPort_Release(pPort);

    return 0;
}

/**
 * @brief Example 3: Data transmission and reception
 */
static int
Example_Serial_DataTransfer(void)
{
    IO_RETURN Status;
    IIOSerialPort *pPort = NULL;
    SERIAL_CONFIG Config;
    CONST CHAR8 *pszTestMessage = "Hello, Serial Port!\r\n";
    UINT8 ucReadBuffer[256];
    UINTN cbWritten, cbRead;

    printf("\n");
    printf("========================================\n");
    printf("  Example 3: Data Transmission\n");
    printf("========================================\n\n");

    // Create and configure COM1
    Status = SerialPortCreate(1, &pPort);
    if (Status != IO_SUCCESS) {
        printf("COM1 not available for this example\n");
        return 0;
    }

    // Configure for 115200 baud, 8N1
    Config.BaudRate = BAUD_115200;
    Config.DataBits = DATA_BITS_8;
    Config.StopBits = STOP_BITS_1;
    Config.Parity = PARITY_NONE;
    Config.FlowControl = FLOW_CONTROL_NONE;
    Config.Protocol = SERIAL_PROTOCOL_RS232;
    Config.bEnableFIFO = TRUE;
    Config.FIFOTriggerLevel = 14;

    IIOSerialPort_Configure(pPort, &Config);
    IIOSerialPort_Open(pPort);

    printf("Sending test message: \"%s\"\n", pszTestMessage);

    // Write data to the port
    Status = IIOSerialPort_Write(pPort,
                                  (CONST UINT8 *)pszTestMessage,
                                  strlen(pszTestMessage),
                                  &cbWritten);

    if (Status == IO_SUCCESS) {
        printf("Successfully wrote %lu bytes\n", (unsigned long)cbWritten);
    } else {
        printf("Write failed (status=0x%08X)\n", Status);
    }

    printf("\n");

    // Try to read data (non-blocking)
    printf("Attempting to read data (timeout=1000ms)...\n");
    Status = IIOSerialPort_Read(pPort, ucReadBuffer, sizeof(ucReadBuffer) - 1,
                                 &cbRead, 1000);

    if (Status == IO_SUCCESS && cbRead > 0) {
        ucReadBuffer[cbRead] = '\0';
        printf("Received %lu bytes: \"%s\"\n", (unsigned long)cbRead, ucReadBuffer);
    } else if (Status == IO_TIMEOUT) {
        printf("No data received (timeout)\n");
    } else {
        printf("Read failed (status=0x%08X)\n", Status);
    }

    printf("\n");

    // Close and release
    IIOSerialPort_Close(pPort);
    IIOSerialPort_Release(pPort);

    return 0;
}

/**
 * @brief Example 4: UART type detection
 */
static int
Example_UART_Detection(void)
{
    IO_RETURN Status;
    UART_TYPE UARTType;
    UINT16 auStandardPorts[] = {0x3F8, 0x2F8, 0x3E8, 0x2E8};  // COM1-COM4
    CONST CHAR8 *apszUARTNames[] = {
        "Unknown",
        "8250",
        "16450",
        "16550 (broken FIFO)",
        "16550A",
        "16650",
        "16750",
        "16850",
        "16950",
        "Custom"
    };
    UINT32 i;

    printf("\n");
    printf("========================================\n");
    printf("  Example 4: UART Type Detection\n");
    printf("========================================\n\n");

    printf("Detecting UART types at standard ISA ports...\n\n");

    for (i = 0; i < 4; i++) {
        Status = SerialDetectUARTType(auStandardPorts[i], &UARTType);

        printf("COM%u (0x%03X): ", i + 1, auStandardPorts[i]);

        if (Status == IO_SUCCESS) {
            if (UARTType < 10) {
                printf("%s\n", apszUARTNames[UARTType]);
            } else {
                printf("Unknown (type=%u)\n", UARTType);
            }
        } else {
            printf("Not detected\n");
        }
    }

    printf("\n");

    return 0;
}

/**
 * @brief Example 5: Flow control demonstration
 */
static int
Example_Flow_Control(void)
{
    IO_RETURN Status;
    IIOSerialPort *pPort = NULL;
    SERIAL_CONFIG Config;

    printf("\n");
    printf("========================================\n");
    printf("  Example 5: Flow Control\n");
    printf("========================================\n\n");

    Status = SerialPortCreate(1, &pPort);
    if (Status != IO_SUCCESS) {
        printf("COM1 not available for this example\n");
        return 0;
    }

    // Configure with hardware flow control (RTS/CTS)
    printf("Configuring port with hardware flow control (RTS/CTS)...\n");
    Config.BaudRate = BAUD_115200;
    Config.DataBits = DATA_BITS_8;
    Config.StopBits = STOP_BITS_1;
    Config.Parity = PARITY_NONE;
    Config.FlowControl = FLOW_CONTROL_RTSCTS;
    Config.Protocol = SERIAL_PROTOCOL_RS232;
    Config.bEnableFIFO = TRUE;
    Config.FIFOTriggerLevel = 8;

    Status = IIOSerialPort_Configure(pPort, &Config);
    if (Status == IO_SUCCESS) {
        printf("Hardware flow control configured successfully\n");
    } else {
        printf("Failed to configure flow control (status=0x%08X)\n", Status);
    }

    printf("\n");

    // Try software flow control (XON/XOFF)
    printf("Switching to software flow control (XON/XOFF)...\n");
    Status = IIOSerialPort_SetFlowControl(pPort, FLOW_CONTROL_XONXOFF);
    if (Status == IO_SUCCESS) {
        printf("Software flow control configured successfully\n");
    } else {
        printf("Failed to configure software flow control (status=0x%08X)\n", Status);
    }

    printf("\n");

    IIOSerialPort_Release(pPort);

    return 0;
}

/**
 * @brief Example 6: Controller database exploration
 */
static int
Example_Controller_Database(void)
{
    UINT32 uCount;
    UINT32 i;
    SERIAL_CONTROLLER_INFO ControllerInfo;

    printf("\n");
    printf("========================================\n");
    printf("  Example 6: Controller Database\n");
    printf("========================================\n\n");

    uCount = SerialGetControllerCount();
    printf("Serial controller database contains %u entries\n\n", uCount);

    // Display first 10 controllers as examples
    printf("Sample controllers (showing first 10):\n\n");
    for (i = 0; i < (uCount < 10 ? uCount : 10); i++) {
        if (SerialGetControllerByIndex(i, &ControllerInfo) == IO_SUCCESS) {
            printf("%u. %s\n", i + 1, ControllerInfo.ControllerName);
            printf("   Vendor: %s\n", ControllerInfo.Vendor);
            printf("   VID:DID: %04X:%04X\n",
                   ControllerInfo.VendorID, ControllerInfo.DeviceID);
            printf("   Ports: %u\n", ControllerInfo.NumPorts);
            printf("   Max Baud: %u bps\n", ControllerInfo.MaxBaudRate);
            printf("   Capabilities:");
            if (ControllerInfo.Capabilities & SERIAL_CAP_RS232)
                printf(" RS-232");
            if (ControllerInfo.Capabilities & SERIAL_CAP_RS422)
                printf(" RS-422");
            if (ControllerInfo.Capabilities & SERIAL_CAP_RS485)
                printf(" RS-485");
            if (ControllerInfo.Capabilities & SERIAL_CAP_FIFO)
                printf(" FIFO");
            if (ControllerInfo.Capabilities & SERIAL_CAP_AUTO_FLOW)
                printf(" Auto-Flow");
            printf("\n\n");
        }
    }

    return 0;
}

/**
 * @brief Example 7: Device database exploration
 */
static int
Example_Device_Database(void)
{
    UINT32 uCount;
    UINT32 i;
    SERIAL_DEVICE_INFO DeviceInfo;
    CONST CHAR8 *apszDeviceTypes[] = {
        "Unknown",
        "Modem",
        "Mouse",
        "Terminal",
        "Printer",
        "GPS",
        "UPS",
        "PLC",
        "Barcode Scanner",
        "Card Reader",
        "Console",
        "Industrial Device"
    };

    printf("\n");
    printf("========================================\n");
    printf("  Example 7: Device Database\n");
    printf("========================================\n\n");

    uCount = SerialGetDeviceCount();
    printf("Serial device database contains %u entries\n\n", uCount);

    // Display devices by category
    printf("Sample devices (showing first 15):\n\n");
    for (i = 0; i < (uCount < 15 ? uCount : 15); i++) {
        if (SerialGetDeviceByIndex(i, &DeviceInfo) == IO_SUCCESS) {
            printf("%u. %s\n", i + 1, DeviceInfo.DeviceName);
            printf("   Manufacturer: %s\n", DeviceInfo.Manufacturer);
            if (DeviceInfo.DeviceType < 12) {
                printf("   Type: %s\n", apszDeviceTypes[DeviceInfo.DeviceType]);
            }
            printf("\n");
        }
    }

    return 0;
}

/**
 * @brief Example 8: Baud rate divisor calculation
 */
static int
Example_Divisor_Calculation(void)
{
    BAUD_RATE auBaudRates[] = {
        BAUD_9600, BAUD_19200, BAUD_38400, BAUD_57600, BAUD_115200
    };
    UINT16 uDivisor;
    UINT32 i;

    printf("\n");
    printf("========================================\n");
    printf("  Example 8: Divisor Calculation\n");
    printf("========================================\n\n");

    printf("Calculating divisors for standard baud rates:\n");
    printf("(Using standard UART clock: 1.8432 MHz)\n\n");

    for (i = 0; i < sizeof(auBaudRates) / sizeof(auBaudRates[0]); i++) {
        if (SerialCalculateDivisor(auBaudRates[i], 0, &uDivisor) == IO_SUCCESS) {
            printf("  %6u bps: divisor = %5u (0x%04X)\n",
                   auBaudRates[i], uDivisor, uDivisor);
        }
    }

    printf("\n");

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
    printf("  Serial Port Family Examples\n");
    printf("================================================\n");

    // Run all examples
    Example_Serial_Initialization();
    Example_Serial_Configuration();
    Example_Serial_DataTransfer();
    Example_UART_Detection();
    Example_Flow_Control();
    Example_Controller_Database();
    Example_Device_Database();
    Example_Divisor_Calculation();

    printf("\n");
    printf("================================================\n");
    printf("  All examples completed\n");
    printf("================================================\n");
    printf("\n");

    return 0;
}
