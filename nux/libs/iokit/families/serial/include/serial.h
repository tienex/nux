/**
 * @file serial.h
 * @brief Serial Port Family Interface - Universal Serial Communication
 *
 * This header defines the Serial Port family interface providing a unified
 * abstraction layer for all serial communication devices including RS-232,
 * RS-422, RS-485, and various UART implementations.
 *
 * The Serial family sits ABOVE bus-specific drivers and provides:
 * - Unified serial port enumeration and configuration
 * - Protocol-agnostic data transmission and reception
 * - Support for legacy COM ports and modern USB-to-serial adapters
 * - UART register-level access for low-level control
 * - Hardware and software flow control
 * - Modem control and status line management
 *
 * This family supports:
 * - RS-232 (standard serial, EIA-232)
 * - RS-422 (differential, balanced)
 * - RS-485 (multi-drop, differential)
 * - UART controllers (16550, 16650, 16750, 16850, 16950)
 * - USB-to-serial adapters (FTDI, Prolific, Silicon Labs)
 * - Multi-port serial cards (ISA, PCI, PCIe)
 * - Industrial serial devices
 * - Legacy devices (modems, terminals, serial mice)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_SERIAL_H
#define IOKIT_SERIAL_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOSerialPort interface GUID
 * {A1B2C3D4-5E6F-4A7B-8C9D-0E1F2A3B4C5D}
 */
DEFINE_GUID(IID_IIOSerialPort,
    0xA1B2C3D4, 0x5E6F, 0x4A7B, 0x8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D);

/**
 * @brief IIOSerialDevice interface GUID
 * {B2C3D4E5-6F7A-4B8C-9D0E-1F2A3B4C5D6E}
 */
DEFINE_GUID(IID_IIOSerialDevice,
    0xB2C3D4E5, 0x6F7A, 0x4B8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E);

/**
 * @brief Serial Protocol Types
 */
typedef enum _SERIAL_PROTOCOL {
    SERIAL_PROTOCOL_RS232       = 0x01,     /**< RS-232 (single-ended) */
    SERIAL_PROTOCOL_RS422       = 0x02,     /**< RS-422 (differential, full duplex) */
    SERIAL_PROTOCOL_RS485       = 0x03,     /**< RS-485 (differential, multi-drop) */
    SERIAL_PROTOCOL_TTL         = 0x04,     /**< TTL-level serial */
    SERIAL_PROTOCOL_CURRENT_LOOP = 0x05,    /**< 20mA current loop */
} SERIAL_PROTOCOL;

/**
 * @brief UART Controller Types
 */
typedef enum _UART_TYPE {
    UART_TYPE_UNKNOWN           = 0x00,     /**< Unknown UART type */
    UART_TYPE_8250              = 0x01,     /**< 8250 (original, no FIFO) */
    UART_TYPE_16450             = 0x02,     /**< 16450 (no FIFO) */
    UART_TYPE_16550             = 0x03,     /**< 16550 (broken FIFO) */
    UART_TYPE_16550A            = 0x04,     /**< 16550A (16-byte FIFO) */
    UART_TYPE_16650             = 0x05,     /**< 16650 (32-byte FIFO) */
    UART_TYPE_16750             = 0x06,     /**< 16750 (64-byte FIFO) */
    UART_TYPE_16850             = 0x07,     /**< 16850 (128-byte FIFO) */
    UART_TYPE_16950             = 0x08,     /**< 16950 (128-byte FIFO, enhanced) */
    UART_TYPE_CUSTOM            = 0xFF,     /**< Custom/proprietary UART */
} UART_TYPE;

/**
 * @brief Standard Baud Rates
 */
typedef enum _BAUD_RATE {
    BAUD_110                    = 110,      /**< 110 bps */
    BAUD_300                    = 300,      /**< 300 bps */
    BAUD_600                    = 600,      /**< 600 bps */
    BAUD_1200                   = 1200,     /**< 1200 bps */
    BAUD_2400                   = 2400,     /**< 2400 bps */
    BAUD_4800                   = 4800,     /**< 4800 bps */
    BAUD_9600                   = 9600,     /**< 9600 bps */
    BAUD_14400                  = 14400,    /**< 14400 bps */
    BAUD_19200                  = 19200,    /**< 19200 bps */
    BAUD_38400                  = 38400,    /**< 38400 bps */
    BAUD_57600                  = 57600,    /**< 57600 bps */
    BAUD_115200                 = 115200,   /**< 115200 bps */
    BAUD_230400                 = 230400,   /**< 230400 bps */
    BAUD_460800                 = 460800,   /**< 460800 bps */
    BAUD_921600                 = 921600,   /**< 921600 bps */
} BAUD_RATE;

/**
 * @brief Data Bits
 */
typedef enum _DATA_BITS {
    DATA_BITS_5                 = 5,        /**< 5 data bits */
    DATA_BITS_6                 = 6,        /**< 6 data bits */
    DATA_BITS_7                 = 7,        /**< 7 data bits */
    DATA_BITS_8                 = 8,        /**< 8 data bits */
} DATA_BITS;

/**
 * @brief Stop Bits
 */
typedef enum _STOP_BITS {
    STOP_BITS_1                 = 0,        /**< 1 stop bit */
    STOP_BITS_1_5               = 1,        /**< 1.5 stop bits */
    STOP_BITS_2                 = 2,        /**< 2 stop bits */
} STOP_BITS;

/**
 * @brief Parity Types
 */
typedef enum _PARITY {
    PARITY_NONE                 = 0,        /**< No parity */
    PARITY_ODD                  = 1,        /**< Odd parity */
    PARITY_EVEN                 = 2,        /**< Even parity */
    PARITY_MARK                 = 3,        /**< Mark parity (always 1) */
    PARITY_SPACE                = 4,        /**< Space parity (always 0) */
} PARITY;

/**
 * @brief Flow Control Types
 */
typedef enum _FLOW_CONTROL {
    FLOW_CONTROL_NONE           = 0x00,     /**< No flow control */
    FLOW_CONTROL_XONXOFF        = 0x01,     /**< Software flow control (XON/XOFF) */
    FLOW_CONTROL_RTSCTS         = 0x02,     /**< Hardware flow control (RTS/CTS) */
    FLOW_CONTROL_DTRDSR         = 0x04,     /**< Hardware flow control (DTR/DSR) */
} FLOW_CONTROL;

/**
 * @brief Modem Control Lines (Bitmask)
 */
#define MCR_DTR                 (1 << 0)    /**< Data Terminal Ready */
#define MCR_RTS                 (1 << 1)    /**< Request To Send */
#define MCR_OUT1                (1 << 2)    /**< OUT1 (auxiliary) */
#define MCR_OUT2                (1 << 3)    /**< OUT2 (interrupt enable) */
#define MCR_LOOPBACK            (1 << 4)    /**< Loopback mode */

/**
 * @brief Modem Status Lines (Bitmask)
 */
#define MSR_DCTS                (1 << 0)    /**< Delta Clear To Send */
#define MSR_DDSR                (1 << 1)    /**< Delta Data Set Ready */
#define MSR_TERI                (1 << 2)    /**< Trailing Edge Ring Indicator */
#define MSR_DDCD                (1 << 3)    /**< Delta Data Carrier Detect */
#define MSR_CTS                 (1 << 4)    /**< Clear To Send */
#define MSR_DSR                 (1 << 5)    /**< Data Set Ready */
#define MSR_RI                  (1 << 6)    /**< Ring Indicator */
#define MSR_DCD                 (1 << 7)    /**< Data Carrier Detect */

/**
 * @brief Line Status Register Flags (Bitmask)
 */
#define LSR_DATA_READY          (1 << 0)    /**< Data available */
#define LSR_OVERRUN_ERROR       (1 << 1)    /**< Overrun error */
#define LSR_PARITY_ERROR        (1 << 2)    /**< Parity error */
#define LSR_FRAMING_ERROR       (1 << 3)    /**< Framing error */
#define LSR_BREAK_INTERRUPT     (1 << 4)    /**< Break interrupt */
#define LSR_THR_EMPTY           (1 << 5)    /**< Transmitter holding register empty */
#define LSR_THR_EMPTY_IDLE      (1 << 6)    /**< Transmitter empty and idle */
#define LSR_FIFO_ERROR          (1 << 7)    /**< Error in FIFO */

/**
 * @brief FIFO Control Register Flags
 */
#define FCR_ENABLE_FIFO         (1 << 0)    /**< Enable FIFO */
#define FCR_CLEAR_RX_FIFO       (1 << 1)    /**< Clear receive FIFO */
#define FCR_CLEAR_TX_FIFO       (1 << 2)    /**< Clear transmit FIFO */
#define FCR_DMA_MODE            (1 << 3)    /**< DMA mode select */
#define FCR_TRIGGER_1           (0 << 6)    /**< RX FIFO trigger: 1 byte */
#define FCR_TRIGGER_4           (1 << 6)    /**< RX FIFO trigger: 4 bytes */
#define FCR_TRIGGER_8           (2 << 6)    /**< RX FIFO trigger: 8 bytes */
#define FCR_TRIGGER_14          (3 << 6)    /**< RX FIFO trigger: 14 bytes */

/**
 * @brief Serial Port Capabilities (Bitmask)
 */
#define SERIAL_CAP_RS232        (1 << 0)    /**< RS-232 support */
#define SERIAL_CAP_RS422        (1 << 1)    /**< RS-422 support */
#define SERIAL_CAP_RS485        (1 << 2)    /**< RS-485 support */
#define SERIAL_CAP_FIFO         (1 << 3)    /**< FIFO support */
#define SERIAL_CAP_AUTO_FLOW    (1 << 4)    /**< Automatic flow control */
#define SERIAL_CAP_MODEM_CTRL   (1 << 5)    /**< Modem control lines */
#define SERIAL_CAP_BREAK        (1 << 6)    /**< Break signal support */
#define SERIAL_CAP_9BIT         (1 << 7)    /**< 9-bit data support */
#define SERIAL_CAP_MULTI_DROP   (1 << 8)    /**< Multi-drop support (RS-485) */
#define SERIAL_CAP_IRDA         (1 << 9)    /**< IrDA support */

/**
 * @brief Maximum COM Port Number
 */
#define MAX_COM_PORT            255

/**
 * @brief Serial Port Configuration
 */
typedef struct _SERIAL_CONFIG {
    BAUD_RATE           BaudRate;           /**< Baud rate */
    DATA_BITS           DataBits;           /**< Number of data bits */
    STOP_BITS           StopBits;           /**< Number of stop bits */
    PARITY              Parity;             /**< Parity type */
    FLOW_CONTROL        FlowControl;        /**< Flow control type */
    SERIAL_PROTOCOL     Protocol;           /**< Serial protocol type */
    BOOLEAN             bEnableFIFO;        /**< Enable FIFO if available */
    UINT8               FIFOTriggerLevel;   /**< FIFO trigger level */
} SERIAL_CONFIG;

/**
 * @brief Serial Controller Information
 */
typedef struct _SERIAL_CONTROLLER_INFO {
    // Hardware Information
    CHAR8               ControllerName[64]; /**< Controller name */
    CHAR8               Vendor[40];         /**< Vendor/manufacturer */
    CHAR8               Model[40];          /**< Model/chipset */
    UART_TYPE           UARTType;           /**< UART controller type */
    UINT16              VendorID;           /**< PCI/USB Vendor ID */
    UINT16              DeviceID;           /**< PCI/USB Device ID */

    // Capabilities
    UINT32              Capabilities;       /**< Capability flags */
    UINT32              MaxBaudRate;        /**< Maximum baud rate */
    UINT32              MinBaudRate;        /**< Minimum baud rate */
    UINT16              FIFOSize;           /**< FIFO buffer size (0 = no FIFO) */
    UINT8               NumPorts;           /**< Number of serial ports */

    // Port Mapping
    UINT16              BaseIOPort;         /**< Base I/O port (ISA) */
    UINT8               IRQ;                /**< IRQ line (ISA) */
    UINT64              BaseAddress;        /**< Base memory address (PCI) */

    // Driver Information
    CHAR8               DriverName[32];     /**< Driver name */
    CHAR8               DriverVersion[16];  /**< Driver version */
} SERIAL_CONTROLLER_INFO;

/**
 * @brief Serial Port Information
 */
typedef struct _SERIAL_PORT_INFO {
    // Port Identity
    UINT16              COMPort;            /**< COM port number (1-255) */
    CHAR8               PortName[16];       /**< Port name (e.g., "COM1") */
    CHAR8               DevicePath[64];     /**< Device path (e.g., "/dev/ttyS0") */

    // Current Configuration
    SERIAL_CONFIG       Config;             /**< Current configuration */

    // Status
    BOOLEAN             bOpen;              /**< Port is open */
    BOOLEAN             bCarrierDetect;     /**< Carrier detected (DCD) */
    BOOLEAN             bRingIndicator;     /**< Ring indicator active */
    UINT8               ModemStatus;        /**< Modem status register */
    UINT8               LineStatus;         /**< Line status register */

    // Statistics
    UINT64              BytesTransmitted;   /**< Total bytes transmitted */
    UINT64              BytesReceived;      /**< Total bytes received */
    UINT32              FramingErrors;      /**< Framing error count */
    UINT32              ParityErrors;       /**< Parity error count */
    UINT32              OverrunErrors;      /**< Overrun error count */
    UINT32              BufferOverruns;     /**< Buffer overrun count */
} SERIAL_PORT_INFO;

/**
 * @brief Serial Device Types
 */
typedef enum _SERIAL_DEVICE_TYPE {
    SERIAL_DEVICE_UNKNOWN       = 0x00,     /**< Unknown device */
    SERIAL_DEVICE_MODEM         = 0x01,     /**< Modem */
    SERIAL_DEVICE_MOUSE         = 0x02,     /**< Serial mouse */
    SERIAL_DEVICE_TERMINAL      = 0x03,     /**< Serial terminal */
    SERIAL_DEVICE_PRINTER       = 0x04,     /**< Serial printer */
    SERIAL_DEVICE_GPS           = 0x05,     /**< GPS receiver */
    SERIAL_DEVICE_UPS           = 0x06,     /**< UPS monitoring */
    SERIAL_DEVICE_PLC           = 0x07,     /**< Programmable Logic Controller */
    SERIAL_DEVICE_BARCODE       = 0x08,     /**< Barcode scanner */
    SERIAL_DEVICE_CARD_READER   = 0x09,     /**< Card reader */
    SERIAL_DEVICE_CONSOLE       = 0x0A,     /**< System console */
    SERIAL_DEVICE_INDUSTRIAL    = 0x0B,     /**< Generic industrial device */
} SERIAL_DEVICE_TYPE;

/**
 * @brief Serial Device Information
 */
typedef struct _SERIAL_DEVICE_INFO {
    CHAR8               DeviceName[64];     /**< Device name */
    SERIAL_DEVICE_TYPE  DeviceType;         /**< Device type */
    CHAR8               Manufacturer[40];   /**< Manufacturer */
    CHAR8               Model[40];          /**< Model name */
    CHAR8               SerialNumber[32];   /**< Serial number (if available) */
    UINT16              AttachedCOMPort;    /**< COM port device is attached to */
} SERIAL_DEVICE_INFO;

/**
 * @brief Receive Callback Function
 *
 * Called when data is received on the serial port.
 *
 * @param pContext      User context pointer
 * @param pData         Received data buffer
 * @param cbLength      Number of bytes received
 *
 * @retval IO_SUCCESS   Data processed successfully
 */
typedef IO_RETURN (*PFN_SERIAL_RECEIVE_CALLBACK)(
    VOID *pContext,
    CONST UINT8 *pData,
    UINTN cbLength
    );

//
// Forward declarations
//
DECLARE_INTERFACE_(IIOSerialPort, IIOService);
DECLARE_INTERFACE_(IIOSerialDevice, IIOService);

/**
 * @brief IIOSerialPort - Serial Port Interface
 *
 * This interface represents a serial port (COM port) and provides methods
 * for configuration, data transmission/reception, and control line management.
 */
#undef INTERFACE
#define INTERFACE IIOSerialPort

DECLARE_INTERFACE_(IIOSerialPort, IIOService)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)(THIS_
        REFIID riid,
        void **ppvObject
        ) PURE;

    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IIOService methods (inherited)
    STDMETHOD_(IO_RETURN, Probe)(THIS_
        IIOService *pProvider,
        UINT32 *puProbeScore
        ) PURE;

    STDMETHOD_(IO_RETURN, Start)(THIS_
        IIOService *pProvider
        ) PURE;

    STDMETHOD_(IO_RETURN, Stop)(THIS_
        IIOService *pProvider
        ) PURE;

    STDMETHOD_(IO_RETURN, Terminate)(THIS_
        UINT32 uOptions
        ) PURE;

    STDMETHOD_(IO_RETURN, GetProperty)(THIS_
        CONST CHAR8 *pszKey,
        VOID *pValue,
        UINTN *pcbSize,
        UINT32 *puType
        ) PURE;

    STDMETHOD_(IO_RETURN, SetProperty)(THIS_
        CONST CHAR8 *pszKey,
        CONST VOID *pValue,
        UINTN cbSize,
        UINT32 uType
        ) PURE;

    STDMETHOD_(IO_RETURN, GetParentService)(THIS_
        IIOService **ppParent
        ) PURE;

    STDMETHOD_(IO_RETURN, GetChildService)(THIS_
        UINT32 uIndex,
        IIOService **ppChild
        ) PURE;

    STDMETHOD_(IO_RETURN, GetServiceState)(THIS_
        UINT32 *puState
        ) PURE;

    STDMETHOD_(IO_RETURN, GetServiceName)(THIS_
        CHAR8 *pszName,
        UINTN cbSize
        ) PURE;

    STDMETHOD_(IO_RETURN, RegisterService)(THIS_
        UINT32 uOptions
        ) PURE;

    // IIOSerialPort methods

    /**
     * @brief Get port information
     *
     * Retrieves comprehensive information about the serial port.
     *
     * @param pPortInfo     Receives port information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetPortInfo)(THIS_
        SERIAL_PORT_INFO *pPortInfo
        ) PURE;

    /**
     * @brief Configure port
     *
     * Configures the serial port parameters.
     *
     * @param pConfig       Configuration parameters
     *
     * @retval IO_SUCCESS       Port configured successfully
     * @retval IO_BAD_ARGUMENT  Invalid configuration
     * @retval IO_NOT_READY     Port not open
     */
    STDMETHOD_(IO_RETURN, Configure)(THIS_
        CONST SERIAL_CONFIG *pConfig
        ) PURE;

    /**
     * @brief Get configuration
     *
     * Retrieves the current port configuration.
     *
     * @param pConfig       Receives current configuration
     *
     * @retval IO_SUCCESS       Configuration retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetConfiguration)(THIS_
        SERIAL_CONFIG *pConfig
        ) PURE;

    /**
     * @brief Open port
     *
     * Opens the serial port for communication.
     *
     * @retval IO_SUCCESS       Port opened successfully
     * @retval IO_BUSY          Port already open
     * @retval IO_ERROR         Failed to open port
     */
    STDMETHOD_(IO_RETURN, Open)(THIS) PURE;

    /**
     * @brief Close port
     *
     * Closes the serial port.
     *
     * @retval IO_SUCCESS       Port closed successfully
     */
    STDMETHOD_(IO_RETURN, Close)(THIS) PURE;

    /**
     * @brief Write data
     *
     * Transmits data through the serial port.
     *
     * @param pData         Data to transmit
     * @param cbLength      Number of bytes to write
     * @param pcbWritten    Receives number of bytes actually written
     *
     * @retval IO_SUCCESS       Data written successfully
     * @retval IO_NOT_READY     Port not open
     * @retval IO_TIMEOUT       Write timeout
     */
    STDMETHOD_(IO_RETURN, Write)(THIS_
        CONST UINT8 *pData,
        UINTN cbLength,
        UINTN *pcbWritten
        ) PURE;

    /**
     * @brief Read data
     *
     * Receives data from the serial port.
     *
     * @param pBuffer       Buffer to receive data
     * @param cbLength      Maximum bytes to read
     * @param pcbRead       Receives number of bytes actually read
     * @param uTimeout      Timeout in milliseconds (0 = non-blocking)
     *
     * @retval IO_SUCCESS       Data read successfully
     * @retval IO_NOT_READY     Port not open
     * @retval IO_TIMEOUT       No data available (timeout)
     */
    STDMETHOD_(IO_RETURN, Read)(THIS_
        UINT8 *pBuffer,
        UINTN cbLength,
        UINTN *pcbRead,
        UINT32 uTimeout
        ) PURE;

    /**
     * @brief Set baud rate
     *
     * Sets the communication baud rate.
     *
     * @param BaudRate      Desired baud rate
     *
     * @retval IO_SUCCESS       Baud rate set successfully
     * @retval IO_UNSUPPORTED   Baud rate not supported
     */
    STDMETHOD_(IO_RETURN, SetBaudRate)(THIS_
        BAUD_RATE BaudRate
        ) PURE;

    /**
     * @brief Set data format
     *
     * Sets the data bits, stop bits, and parity.
     *
     * @param DataBits      Number of data bits
     * @param StopBits      Number of stop bits
     * @param Parity        Parity type
     *
     * @retval IO_SUCCESS       Format set successfully
     * @retval IO_UNSUPPORTED   Format not supported
     */
    STDMETHOD_(IO_RETURN, SetDataFormat)(THIS_
        DATA_BITS DataBits,
        STOP_BITS StopBits,
        PARITY Parity
        ) PURE;

    /**
     * @brief Set flow control
     *
     * Configures flow control mode.
     *
     * @param FlowControl   Flow control type
     *
     * @retval IO_SUCCESS       Flow control set successfully
     * @retval IO_UNSUPPORTED   Flow control mode not supported
     */
    STDMETHOD_(IO_RETURN, SetFlowControl)(THIS_
        FLOW_CONTROL FlowControl
        ) PURE;

    /**
     * @brief Set modem control
     *
     * Sets modem control lines (DTR, RTS, etc.).
     *
     * @param uControlLines Modem control line flags (MCR_*)
     *
     * @retval IO_SUCCESS       Control lines set successfully
     */
    STDMETHOD_(IO_RETURN, SetModemControl)(THIS_
        UINT8 uControlLines
        ) PURE;

    /**
     * @brief Get modem status
     *
     * Retrieves modem status lines (CTS, DSR, DCD, RI).
     *
     * @param puStatusLines Receives modem status flags (MSR_*)
     *
     * @retval IO_SUCCESS       Status retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetModemStatus)(THIS_
        UINT8 *puStatusLines
        ) PURE;

    /**
     * @brief Get line status
     *
     * Retrieves line status register.
     *
     * @param puLineStatus  Receives line status flags (LSR_*)
     *
     * @retval IO_SUCCESS       Status retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetLineStatus)(THIS_
        UINT8 *puLineStatus
        ) PURE;

    /**
     * @brief Send break signal
     *
     * Sends a break signal for the specified duration.
     *
     * @param uDurationMs   Break duration in milliseconds
     *
     * @retval IO_SUCCESS       Break signal sent
     * @retval IO_UNSUPPORTED   Break signal not supported
     */
    STDMETHOD_(IO_RETURN, SendBreak)(THIS_
        UINT32 uDurationMs
        ) PURE;

    /**
     * @brief Flush buffers
     *
     * Flushes transmit and/or receive buffers.
     *
     * @param bFlushTX      Flush transmit buffer
     * @param bFlushRX      Flush receive buffer
     *
     * @retval IO_SUCCESS       Buffers flushed successfully
     */
    STDMETHOD_(IO_RETURN, FlushBuffers)(THIS_
        BOOLEAN bFlushTX,
        BOOLEAN bFlushRX
        ) PURE;

    /**
     * @brief Get buffer status
     *
     * Retrieves the number of bytes in TX/RX buffers.
     *
     * @param puTXBytes     Receives bytes in TX buffer
     * @param puRXBytes     Receives bytes in RX buffer
     *
     * @retval IO_SUCCESS       Status retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetBufferStatus)(THIS_
        UINT32 *puTXBytes,
        UINT32 *puRXBytes
        ) PURE;

    /**
     * @brief Register receive callback
     *
     * Registers a callback function to be called when data is received.
     *
     * @param pfnCallback   Callback function
     * @param pContext      User context pointer
     *
     * @retval IO_SUCCESS       Callback registered successfully
     */
    STDMETHOD_(IO_RETURN, RegisterReceiveCallback)(THIS_
        PFN_SERIAL_RECEIVE_CALLBACK pfnCallback,
        VOID *pContext
        ) PURE;

    /**
     * @brief Enable FIFO
     *
     * Enables FIFO buffers if supported.
     *
     * @param uTriggerLevel FIFO trigger level
     *
     * @retval IO_SUCCESS       FIFO enabled successfully
     * @retval IO_UNSUPPORTED   FIFO not supported
     */
    STDMETHOD_(IO_RETURN, EnableFIFO)(THIS_
        UINT8 uTriggerLevel
        ) PURE;

    /**
     * @brief Disable FIFO
     *
     * Disables FIFO buffers.
     *
     * @retval IO_SUCCESS       FIFO disabled successfully
     */
    STDMETHOD_(IO_RETURN, DisableFIFO)(THIS) PURE;

    /**
     * @brief Set timeout
     *
     * Sets read/write timeout values.
     *
     * @param uReadTimeout  Read timeout in milliseconds
     * @param uWriteTimeout Write timeout in milliseconds
     *
     * @retval IO_SUCCESS       Timeouts set successfully
     */
    STDMETHOD_(IO_RETURN, SetTimeout)(THIS_
        UINT32 uReadTimeout,
        UINT32 uWriteTimeout
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOSerialDevice - Serial Device Interface
 *
 * This interface represents a device connected to a serial port
 * (e.g., modem, GPS, UPS) and provides device-specific operations.
 */
#undef INTERFACE
#define INTERFACE IIOSerialDevice

DECLARE_INTERFACE_(IIOSerialDevice, IIOService)
{
    // IUnknown and IIOService methods inherited...

    /**
     * @brief Get device information
     *
     * Retrieves information about the serial device.
     *
     * @param pDeviceInfo   Receives device information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        SERIAL_DEVICE_INFO *pDeviceInfo
        ) PURE;

    /**
     * @brief Initialize device
     *
     * Initializes the serial device with appropriate settings.
     *
     * @retval IO_SUCCESS       Device initialized successfully
     * @retval IO_ERROR         Initialization failed
     */
    STDMETHOD_(IO_RETURN, Initialize)(THIS) PURE;

    /**
     * @brief Reset device
     *
     * Resets the serial device to default state.
     *
     * @retval IO_SUCCESS       Device reset successfully
     */
    STDMETHOD_(IO_RETURN, Reset)(THIS) PURE;

    /**
     * @brief Send command
     *
     * Sends a command string to the device.
     *
     * @param pszCommand    Command string
     *
     * @retval IO_SUCCESS       Command sent successfully
     * @retval IO_ERROR         Command failed
     */
    STDMETHOD_(IO_RETURN, SendCommand)(THIS_
        CONST CHAR8 *pszCommand
        ) PURE;

    /**
     * @brief Receive response
     *
     * Receives a response from the device.
     *
     * @param pszResponse   Buffer to receive response
     * @param cbSize        Size of buffer
     * @param uTimeout      Timeout in milliseconds
     *
     * @retval IO_SUCCESS       Response received successfully
     * @retval IO_TIMEOUT       Timeout waiting for response
     */
    STDMETHOD_(IO_RETURN, ReceiveResponse)(THIS_
        CHAR8 *pszResponse,
        UINTN cbSize,
        UINT32 uTimeout
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOSerialPort methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOSerialPort_GetPortInfo(p,a)              (p)->lpVtbl->GetPortInfo(p,a)
#define IIOSerialPort_Configure(p,a)                (p)->lpVtbl->Configure(p,a)
#define IIOSerialPort_GetConfiguration(p,a)         (p)->lpVtbl->GetConfiguration(p,a)
#define IIOSerialPort_Open(p)                       (p)->lpVtbl->Open(p)
#define IIOSerialPort_Close(p)                      (p)->lpVtbl->Close(p)
#define IIOSerialPort_Write(p,a,b,c)                (p)->lpVtbl->Write(p,a,b,c)
#define IIOSerialPort_Read(p,a,b,c,d)               (p)->lpVtbl->Read(p,a,b,c,d)
#define IIOSerialPort_SetBaudRate(p,a)              (p)->lpVtbl->SetBaudRate(p,a)
#define IIOSerialPort_SetDataFormat(p,a,b,c)        (p)->lpVtbl->SetDataFormat(p,a,b,c)
#define IIOSerialPort_SetFlowControl(p,a)           (p)->lpVtbl->SetFlowControl(p,a)
#define IIOSerialPort_SetModemControl(p,a)          (p)->lpVtbl->SetModemControl(p,a)
#define IIOSerialPort_GetModemStatus(p,a)           (p)->lpVtbl->GetModemStatus(p,a)
#define IIOSerialPort_GetLineStatus(p,a)            (p)->lpVtbl->GetLineStatus(p,a)
#define IIOSerialPort_SendBreak(p,a)                (p)->lpVtbl->SendBreak(p,a)
#define IIOSerialPort_FlushBuffers(p,a,b)           (p)->lpVtbl->FlushBuffers(p,a,b)
#define IIOSerialPort_GetBufferStatus(p,a,b)        (p)->lpVtbl->GetBufferStatus(p,a,b)
#define IIOSerialPort_RegisterReceiveCallback(p,a,b) (p)->lpVtbl->RegisterReceiveCallback(p,a,b)
#define IIOSerialPort_EnableFIFO(p,a)               (p)->lpVtbl->EnableFIFO(p,a)
#define IIOSerialPort_DisableFIFO(p)                (p)->lpVtbl->DisableFIFO(p)
#define IIOSerialPort_SetTimeout(p,a,b)             (p)->lpVtbl->SetTimeout(p,a,b)

#define IIOSerialDevice_GetDeviceInfo(p,a)          (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOSerialDevice_Initialize(p)               (p)->lpVtbl->Initialize(p)
#define IIOSerialDevice_Reset(p)                    (p)->lpVtbl->Reset(p)
#define IIOSerialDevice_SendCommand(p,a)            (p)->lpVtbl->SendCommand(p,a)
#define IIOSerialDevice_ReceiveResponse(p,a,b,c)    (p)->lpVtbl->ReceiveResponse(p,a,b,c)

#endif

/**
 * @brief Initialize Serial Port family subsystem
 *
 * Initializes the serial port abstraction layer and registers it with IOKit.
 *
 * @retval IO_SUCCESS   Initialization successful
 * @retval IO_ERROR     Initialization failed
 */
IO_RETURN
SerialInitialize(
    VOID
    );

/**
 * @brief Shutdown Serial Port family subsystem
 *
 * Shuts down the serial port abstraction layer and releases resources.
 *
 * @retval IO_SUCCESS   Shutdown successful
 */
IO_RETURN
SerialShutdown(
    VOID
    );

/**
 * @brief Create a serial port instance
 *
 * Creates a serial port interface for the specified COM port.
 *
 * @param uCOMPort      COM port number (1-255)
 * @param ppSerialPort  Receives serial port interface
 *
 * @retval IO_SUCCESS           Port created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid port number
 * @retval IO_NO_DEVICE         Port does not exist
 */
IO_RETURN
SerialPortCreate(
    UINT16 uCOMPort,
    IIOSerialPort **ppSerialPort
    );

/**
 * @brief Enumerate serial ports
 *
 * Enumerates all available serial ports in the system.
 *
 * @param ppPorts       Array to receive port interfaces
 * @param puCount       On input: max ports; On output: actual count
 *
 * @retval IO_SUCCESS       Enumeration successful
 * @retval IO_BAD_ARGUMENT  Invalid argument
 */
IO_RETURN
SerialEnumeratePorts(
    IIOSerialPort **ppPorts,
    UINT32 *puCount
    );

/**
 * @brief Detect controller type
 *
 * Detects the UART controller type by probing registers.
 *
 * @param uBasePort     Base I/O port address
 * @param pUARTType     Receives UART type
 *
 * @retval IO_SUCCESS       Type detected successfully
 * @retval IO_NO_DEVICE     No UART at specified address
 */
IO_RETURN
SerialDetectUARTType(
    UINT16 uBasePort,
    UART_TYPE *pUARTType
    );

/**
 * @brief Get controller name
 *
 * Retrieves the name of a serial controller by vendor/device ID.
 *
 * @param uVendorID     Vendor ID
 * @param uDeviceID     Device ID
 * @param pszName       Buffer to receive name
 * @param cbSize        Size of buffer
 *
 * @retval IO_SUCCESS       Name retrieved successfully
 * @retval IO_NO_MATCH      Controller not in database
 */
IO_RETURN
SerialGetControllerName(
    UINT16 uVendorID,
    UINT16 uDeviceID,
    CHAR8 *pszName,
    UINTN cbSize
    );

/**
 * @brief Calculate baud rate divisor
 *
 * Calculates the divisor value for a given baud rate.
 *
 * @param uBaudRate     Desired baud rate
 * @param uClockRate    UART clock rate (typically 115200 * 16)
 * @param puDivisor     Receives divisor value
 *
 * @retval IO_SUCCESS       Divisor calculated successfully
 */
IO_RETURN
SerialCalculateDivisor(
    UINT32 uBaudRate,
    UINT32 uClockRate,
    UINT16 *puDivisor
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_SERIAL_H */
