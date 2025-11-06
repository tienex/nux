/**
 * @file parallel.h
 * @brief Parallel Port Family Interface - IEEE 1284 Compliant
 *
 * This header defines the Parallel Port family interface for managing
 * parallel port controllers and devices, supporting SPP, EPP, ECP, and
 * IEEE 1284 protocols including PARSCSI (parallel port SCSI adapters).
 *
 * Supported Modes:
 * - SPP (Standard Parallel Port): Unidirectional, 8-bit parallel
 * - EPP (Enhanced Parallel Port): Bidirectional, high-speed
 * - ECP (Extended Capabilities Port): DMA, high-speed, compression
 * - IEEE 1284: Negotiation, ECP/EPP mode switching
 * - PARSCSI: SCSI protocol over parallel port (Iomega Zip, Jaz)
 *
 * Hardware Support:
 * - ISA/LPT ports (LPT1-LPT3: 0x378, 0x278, 0x3BC)
 * - PCI parallel port cards (NetMos, SIIG, StarTech, Lava)
 * - PCIe parallel port cards
 * - USB-to-parallel adapters
 * - Parallel port devices (printers, scanners, PARSCSI, dongles)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_PARALLEL_H
#define IOKIT_PARALLEL_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOParallelPort interface GUID
 * {A7B8C9D0-1E2F-3A4B-5C6D-7E8F9A0B1C2D}
 */
DEFINE_GUID(IID_IIOParallelPort,
    0xA7B8C9D0, 0x1E2F, 0x3A4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B, 0x1C, 0x2D);

/**
 * @brief IIOParallelDevice interface GUID
 * {B8C9D0E1-2F3A-4B5C-6D7E-8F9A0B1C2D3E}
 */
DEFINE_GUID(IID_IIOParallelDevice,
    0xB8C9D0E1, 0x2F3A, 0x4B5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B, 0x1C, 0x2D, 0x3E);

/**
 * @brief Parallel port modes
 */
typedef enum _PARALLEL_PORT_MODE {
    PARALLEL_MODE_SPP           = 0,    /**< Standard Parallel Port (unidirectional) */
    PARALLEL_MODE_PS2           = 1,    /**< PS/2 bidirectional mode */
    PARALLEL_MODE_EPP           = 2,    /**< Enhanced Parallel Port */
    PARALLEL_MODE_ECP           = 3,    /**< Extended Capabilities Port */
    PARALLEL_MODE_EPP_ECP       = 4,    /**< EPP+ECP combined mode */
    PARALLEL_MODE_IEEE1284      = 5,    /**< IEEE 1284 negotiated mode */
} PARALLEL_PORT_MODE;

/**
 * @brief Parallel port types
 */
typedef enum _PARALLEL_PORT_TYPE {
    PARALLEL_TYPE_ISA           = 0,    /**< ISA/Legacy LPT port */
    PARALLEL_TYPE_PCI           = 1,    /**< PCI parallel port card */
    PARALLEL_TYPE_PCIE          = 2,    /**< PCIe parallel port card */
    PARALLEL_TYPE_USB           = 3,    /**< USB-to-parallel adapter */
    PARALLEL_TYPE_ONBOARD       = 4,    /**< Onboard/integrated port */
} PARALLEL_PORT_TYPE;

/**
 * @brief Parallel device types
 */
typedef enum _PARALLEL_DEVICE_TYPE {
    PARALLEL_DEVICE_UNKNOWN     = 0,    /**< Unknown device */
    PARALLEL_DEVICE_PRINTER     = 1,    /**< Parallel printer */
    PARALLEL_DEVICE_SCANNER     = 2,    /**< Parallel port scanner */
    PARALLEL_DEVICE_PARSCSI     = 3,    /**< PARSCSI adapter (Zip, Jaz) */
    PARALLEL_DEVICE_TAPE        = 4,    /**< Tape drive */
    PARALLEL_DEVICE_DONGLE      = 5,    /**< Security dongle/key */
    PARALLEL_DEVICE_NETWORK     = 6,    /**< Network adapter (DirectParallel) */
    PARALLEL_DEVICE_PLOTTER     = 7,    /**< Plotter */
    PARALLEL_DEVICE_CAMERA      = 8,    /**< Camera/scanner */
    PARALLEL_DEVICE_EXTERNAL_HDD = 9,   /**< External hard drive */
} PARALLEL_DEVICE_TYPE;

/**
 * @brief IEEE 1284 protocol phases
 */
typedef enum _IEEE1284_PHASE {
    IEEE1284_PHASE_NEGOTIATION  = 0,    /**< Mode negotiation */
    IEEE1284_PHASE_DATA         = 1,    /**< Data transfer */
    IEEE1284_PHASE_TERMINATION  = 2,    /**< Terminate transfer */
    IEEE1284_PHASE_INTERRUPT    = 3,    /**< Interrupt phase */
} IEEE1284_PHASE;

/**
 * @brief IEEE 1284 modes
 */
typedef enum _IEEE1284_MODE {
    IEEE1284_MODE_NIBBLE        = 0,    /**< Nibble mode (4-bit reverse) */
    IEEE1284_MODE_BYTE          = 1,    /**< Byte mode (8-bit reverse) */
    IEEE1284_MODE_EPP           = 2,    /**< EPP mode */
    IEEE1284_MODE_ECP           = 3,    /**< ECP mode */
    IEEE1284_MODE_ECP_RLE       = 4,    /**< ECP with RLE compression */
    IEEE1284_MODE_BECP          = 5,    /**< Bounded ECP */
} IEEE1284_MODE;

/**
 * @brief Parallel port capabilities
 */
#define PARALLEL_CAP_SPP            0x00000001  /**< SPP support */
#define PARALLEL_CAP_PS2            0x00000002  /**< PS/2 bidirectional */
#define PARALLEL_CAP_EPP            0x00000004  /**< EPP support */
#define PARALLEL_CAP_ECP            0x00000008  /**< ECP support */
#define PARALLEL_CAP_IEEE1284       0x00000010  /**< IEEE 1284 protocol */
#define PARALLEL_CAP_DMA            0x00000020  /**< DMA support */
#define PARALLEL_CAP_FIFO           0x00000040  /**< FIFO buffer */
#define PARALLEL_CAP_INTERRUPT      0x00000080  /**< Interrupt support */
#define PARALLEL_CAP_TRISTATE       0x00000100  /**< Tri-state support */
#define PARALLEL_CAP_PARSCSI        0x00000200  /**< PARSCSI support */
#define PARALLEL_CAP_RLE            0x00000400  /**< RLE compression */
#define PARALLEL_CAP_HOTPLUG        0x00000800  /**< Hot-plug detection */

/**
 * @brief Parallel port register offsets (standard LPT)
 */
#define PARALLEL_REG_DATA           0x00    /**< Data register (base+0) */
#define PARALLEL_REG_STATUS         0x01    /**< Status register (base+1) */
#define PARALLEL_REG_CONTROL        0x02    /**< Control register (base+2) */
#define PARALLEL_REG_EPP_ADDR       0x03    /**< EPP address register (base+3) */
#define PARALLEL_REG_EPP_DATA       0x04    /**< EPP data register (base+4) */
#define PARALLEL_REG_ECP_DATA       0x400   /**< ECP data FIFO */
#define PARALLEL_REG_ECP_CONTROL    0x402   /**< ECP control register */

/**
 * @brief Parallel port status register bits
 */
#define PARALLEL_STATUS_ERROR       0x08    /**< Error (pin 15) */
#define PARALLEL_STATUS_SELECT      0x10    /**< Select (pin 13) */
#define PARALLEL_STATUS_PAPER_OUT   0x20    /**< Paper out (pin 12) */
#define PARALLEL_STATUS_ACK         0x40    /**< Acknowledge (pin 10) */
#define PARALLEL_STATUS_BUSY        0x80    /**< Busy (pin 11, inverted) */

/**
 * @brief Parallel port control register bits
 */
#define PARALLEL_CONTROL_STROBE     0x01    /**< Strobe (pin 1) */
#define PARALLEL_CONTROL_AUTOFEED   0x02    /**< Auto feed (pin 14) */
#define PARALLEL_CONTROL_INIT       0x04    /**< Initialize (pin 16) */
#define PARALLEL_CONTROL_SELECT_IN  0x08    /**< Select in (pin 17) */
#define PARALLEL_CONTROL_IRQ_ENABLE 0x10    /**< Enable interrupt */
#define PARALLEL_CONTROL_DIRECTION  0x20    /**< Direction (0=out, 1=in) */

/**
 * @brief Parallel port standard I/O addresses
 */
#define PARALLEL_IOADDR_LPT1        0x378   /**< LPT1 I/O address */
#define PARALLEL_IOADDR_LPT2        0x278   /**< LPT2 I/O address */
#define PARALLEL_IOADDR_LPT3        0x3BC   /**< LPT3 I/O address */

/**
 * @brief Parallel port IRQs
 */
#define PARALLEL_IRQ_LPT1           7       /**< LPT1 IRQ */
#define PARALLEL_IRQ_LPT2           5       /**< LPT2 IRQ */

/**
 * @brief Parallel port DMA channels
 */
#define PARALLEL_DMA_NONE           0xFF    /**< No DMA */
#define PARALLEL_DMA_CHANNEL_3      3       /**< DMA channel 3 */

/**
 * @brief Parallel port controller information
 */
typedef struct _PARALLEL_PORT_INFO {
    UINT16              VendorID;           /**< PCI Vendor ID (0 for ISA) */
    UINT16              DeviceID;           /**< PCI Device ID (0 for ISA) */
    PARALLEL_PORT_TYPE  PortType;           /**< Port type */
    PARALLEL_PORT_MODE  CurrentMode;        /**< Current operating mode */
    UINT32              Capabilities;       /**< Capability flags */
    UINT16              IOBase;             /**< I/O base address */
    UINT16              IOSize;             /**< I/O address range size */
    UINT8               IRQ;                /**< IRQ number */
    UINT8               DMAChannel;         /**< DMA channel */
    UINT32              FIFOSize;           /**< FIFO size (bytes) */
    BOOLEAN             bECPDMAEnabled;     /**< ECP DMA enabled */
    BOOLEAN             bIRQEnabled;        /**< IRQ enabled */
    CHAR8               PortName[16];       /**< Port name (e.g., "LPT1") */
} PARALLEL_PORT_INFO;

/**
 * @brief Parallel device information
 */
typedef struct _PARALLEL_DEVICE_INFO {
    PARALLEL_DEVICE_TYPE    DeviceType;     /**< Device type */
    CHAR8                   Manufacturer[64]; /**< Manufacturer */
    CHAR8                   Model[64];      /**< Model name */
    CHAR8                   SerialNumber[64]; /**< Serial number */
    UINT32                  Capabilities;   /**< Device capabilities */
    IEEE1284_MODE           PreferredMode;  /**< Preferred transfer mode */
    BOOLEAN                 bIEEE1284;      /**< IEEE 1284 compliant */
    BOOLEAN                 bBidirectional; /**< Bidirectional support */
    UINT32                  MaxTransferRate; /**< Max transfer rate (KB/s) */
} PARALLEL_DEVICE_INFO;

/**
 * @brief Parallel I/O request
 */
typedef struct _PARALLEL_IO_REQUEST {
    VOID               *pBuffer;            /**< Data buffer */
    UINT32              Length;             /**< Transfer length (bytes) */
    BOOLEAN             bRead;              /**< TRUE=read, FALSE=write */
    UINT32              TimeoutMs;          /**< Timeout (milliseconds) */
    UINT32              Flags;              /**< Transfer flags */
    UINT32              BytesTransferred;   /**< Actual bytes transferred */
    UINT32              Status;             /**< Transfer status */
} PARALLEL_IO_REQUEST;

/**
 * @brief IEEE 1284 device ID string (max 64KB per spec)
 */
typedef struct _IEEE1284_DEVICE_ID {
    UINT16              Length;             /**< String length */
    CHAR8               Data[1024];         /**< ID string data */
} IEEE1284_DEVICE_ID;

//
// Forward declarations
//
DECLARE_INTERFACE_(IIOParallelPort, IIOService);
DECLARE_INTERFACE_(IIOParallelDevice, IIOService);

/**
 * @brief Parallel Port Interface
 *
 * Represents a parallel port controller (LPT1/LPT2/LPT3, PCI card, USB adapter).
 */
#undef INTERFACE
#define INTERFACE IIOParallelPort

DECLARE_INTERFACE_(IIOParallelPort, IIOService)
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

    // IIOParallelPort methods

    /**
     * @brief Get port information
     *
     * Retrieves comprehensive port information including mode, capabilities,
     * and I/O configuration.
     *
     * @param pPortInfo         Receives port information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetPortInfo)(THIS_
        PARALLEL_PORT_INFO *pPortInfo
        ) PURE;

    /**
     * @brief Set port mode
     *
     * Sets the parallel port operating mode (SPP/EPP/ECP).
     *
     * @param Mode              Desired port mode
     *
     * @retval IO_SUCCESS       Mode set successfully
     * @retval IO_UNSUPPORTED   Mode not supported
     */
    STDMETHOD_(IO_RETURN, SetMode)(THIS_
        PARALLEL_PORT_MODE Mode
        ) PURE;

    /**
     * @brief Read data register
     *
     * Reads 8 bits from the parallel port data register.
     *
     * @param pData             Receives data byte
     *
     * @retval IO_SUCCESS       Data read successfully
     */
    STDMETHOD_(IO_RETURN, ReadData)(THIS_
        UINT8 *pData
        ) PURE;

    /**
     * @brief Write data register
     *
     * Writes 8 bits to the parallel port data register.
     *
     * @param Data              Data byte to write
     *
     * @retval IO_SUCCESS       Data written successfully
     */
    STDMETHOD_(IO_RETURN, WriteData)(THIS_
        UINT8 Data
        ) PURE;

    /**
     * @brief Read status register
     *
     * Reads the parallel port status register.
     *
     * @param pStatus           Receives status byte
     *
     * @retval IO_SUCCESS       Status read successfully
     */
    STDMETHOD_(IO_RETURN, ReadStatus)(THIS_
        UINT8 *pStatus
        ) PURE;

    /**
     * @brief Write control register
     *
     * Writes the parallel port control register.
     *
     * @param Control           Control byte to write
     *
     * @retval IO_SUCCESS       Control written successfully
     */
    STDMETHOD_(IO_RETURN, WriteControl)(THIS_
        UINT8 Control
        ) PURE;

    /**
     * @brief Read control register
     *
     * Reads the parallel port control register.
     *
     * @param pControl          Receives control byte
     *
     * @retval IO_SUCCESS       Control read successfully
     */
    STDMETHOD_(IO_RETURN, ReadControl)(THIS_
        UINT8 *pControl
        ) PURE;

    /**
     * @brief EPP read operation
     *
     * Performs an EPP read operation (address or data).
     *
     * @param bAddress          TRUE for address, FALSE for data
     * @param pBuffer           Buffer to receive data
     * @param Length            Number of bytes to read
     *
     * @retval IO_SUCCESS       Read successful
     * @retval IO_UNSUPPORTED   EPP not supported
     */
    STDMETHOD_(IO_RETURN, EPPRead)(THIS_
        BOOLEAN bAddress,
        VOID *pBuffer,
        UINT32 Length
        ) PURE;

    /**
     * @brief EPP write operation
     *
     * Performs an EPP write operation (address or data).
     *
     * @param bAddress          TRUE for address, FALSE for data
     * @param pBuffer           Data to write
     * @param Length            Number of bytes to write
     *
     * @retval IO_SUCCESS       Write successful
     * @retval IO_UNSUPPORTED   EPP not supported
     */
    STDMETHOD_(IO_RETURN, EPPWrite)(THIS_
        BOOLEAN bAddress,
        CONST VOID *pBuffer,
        UINT32 Length
        ) PURE;

    /**
     * @brief ECP read FIFO
     *
     * Reads data from ECP FIFO.
     *
     * @param pBuffer           Buffer to receive data
     * @param Length            Number of bytes to read
     * @param pBytesRead        Receives actual bytes read
     *
     * @retval IO_SUCCESS       Read successful
     * @retval IO_UNSUPPORTED   ECP not supported
     */
    STDMETHOD_(IO_RETURN, ECPRead)(THIS_
        VOID *pBuffer,
        UINT32 Length,
        UINT32 *pBytesRead
        ) PURE;

    /**
     * @brief ECP write FIFO
     *
     * Writes data to ECP FIFO.
     *
     * @param pBuffer           Data to write
     * @param Length            Number of bytes to write
     * @param pBytesWritten     Receives actual bytes written
     *
     * @retval IO_SUCCESS       Write successful
     * @retval IO_UNSUPPORTED   ECP not supported
     */
    STDMETHOD_(IO_RETURN, ECPWrite)(THIS_
        CONST VOID *pBuffer,
        UINT32 Length,
        UINT32 *pBytesWritten
        ) PURE;

    /**
     * @brief IEEE 1284 mode negotiation
     *
     * Negotiates IEEE 1284 protocol mode with device.
     *
     * @param Mode              Desired IEEE 1284 mode
     *
     * @retval IO_SUCCESS       Negotiation successful
     * @retval IO_UNSUPPORTED   Mode not supported by device
     * @retval IO_ERROR         Negotiation failed
     */
    STDMETHOD_(IO_RETURN, IEEE1284Negotiate)(THIS_
        IEEE1284_MODE Mode
        ) PURE;

    /**
     * @brief IEEE 1284 termination
     *
     * Terminates IEEE 1284 protocol session.
     *
     * @retval IO_SUCCESS       Termination successful
     */
    STDMETHOD_(IO_RETURN, IEEE1284Terminate)(THIS) PURE;

    /**
     * @brief Read IEEE 1284 device ID
     *
     * Reads the IEEE 1284 device identification string.
     *
     * @param pDeviceID         Receives device ID
     *
     * @retval IO_SUCCESS       Device ID read successfully
     * @retval IO_ERROR         Read failed
     */
    STDMETHOD_(IO_RETURN, ReadDeviceID)(THIS_
        IEEE1284_DEVICE_ID *pDeviceID
        ) PURE;

    /**
     * @brief Detect connected device
     *
     * Detects and identifies a device connected to the port.
     *
     * @param ppDevice          Receives device interface
     *
     * @retval IO_SUCCESS       Device detected
     * @retval IO_NO_DEVICE     No device connected
     */
    STDMETHOD_(IO_RETURN, DetectDevice)(THIS_
        IIOParallelDevice **ppDevice
        ) PURE;

    /**
     * @brief Enable/disable interrupts
     *
     * Enables or disables parallel port interrupts.
     *
     * @param bEnable           TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS       IRQ state changed
     */
    STDMETHOD_(IO_RETURN, SetInterruptEnable)(THIS_
        BOOLEAN bEnable
        ) PURE;

    /**
     * @brief Enable/disable DMA
     *
     * Enables or disables ECP DMA mode.
     *
     * @param bEnable           TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS       DMA state changed
     * @retval IO_UNSUPPORTED   DMA not supported
     */
    STDMETHOD_(IO_RETURN, SetDMAEnable)(THIS_
        BOOLEAN bEnable
        ) PURE;

    /**
     * @brief Reset port
     *
     * Resets the parallel port to default state.
     *
     * @retval IO_SUCCESS       Port reset successfully
     */
    STDMETHOD_(IO_RETURN, ResetPort)(THIS) PURE;
};

/**
 * @brief Parallel Device Interface
 *
 * Represents a device connected to a parallel port.
 */
#undef INTERFACE
#define INTERFACE IIOParallelDevice

DECLARE_INTERFACE_(IIOParallelDevice, IIOService)
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

    // IIOParallelDevice methods

    /**
     * @brief Get device information
     *
     * Retrieves comprehensive device information.
     *
     * @param pDeviceInfo       Receives device information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        PARALLEL_DEVICE_INFO *pDeviceInfo
        ) PURE;

    /**
     * @brief Read from device
     *
     * Reads data from the parallel device.
     *
     * @param pBuffer           Buffer to receive data
     * @param Length            Number of bytes to read
     * @param pBytesRead        Receives actual bytes read
     *
     * @retval IO_SUCCESS       Read successful
     * @retval IO_ERROR         Read failed
     * @retval IO_TIMEOUT       Read timeout
     */
    STDMETHOD_(IO_RETURN, Read)(THIS_
        VOID *pBuffer,
        UINT32 Length,
        UINT32 *pBytesRead
        ) PURE;

    /**
     * @brief Write to device
     *
     * Writes data to the parallel device.
     *
     * @param pBuffer           Data to write
     * @param Length            Number of bytes to write
     * @param pBytesWritten     Receives actual bytes written
     *
     * @retval IO_SUCCESS       Write successful
     * @retval IO_ERROR         Write failed
     * @retval IO_TIMEOUT       Write timeout
     */
    STDMETHOD_(IO_RETURN, Write)(THIS_
        CONST VOID *pBuffer,
        UINT32 Length,
        UINT32 *pBytesWritten
        ) PURE;

    /**
     * @brief Perform I/O request
     *
     * Performs a complete I/O request with device.
     *
     * @param pRequest          I/O request structure
     *
     * @retval IO_SUCCESS       I/O successful
     * @retval IO_ERROR         I/O failed
     * @retval IO_TIMEOUT       I/O timeout
     */
    STDMETHOD_(IO_RETURN, IORequest)(THIS_
        PARALLEL_IO_REQUEST *pRequest
        ) PURE;

    /**
     * @brief Reset device
     *
     * Resets the parallel device.
     *
     * @retval IO_SUCCESS       Device reset successfully
     */
    STDMETHOD_(IO_RETURN, ResetDevice)(THIS) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOParallelPort methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOParallelPort_GetPortInfo(p,a)            (p)->lpVtbl->GetPortInfo(p,a)
#define IIOParallelPort_SetMode(p,a)                (p)->lpVtbl->SetMode(p,a)
#define IIOParallelPort_ReadData(p,a)               (p)->lpVtbl->ReadData(p,a)
#define IIOParallelPort_WriteData(p,a)              (p)->lpVtbl->WriteData(p,a)
#define IIOParallelPort_ReadStatus(p,a)             (p)->lpVtbl->ReadStatus(p,a)
#define IIOParallelPort_WriteControl(p,a)           (p)->lpVtbl->WriteControl(p,a)
#define IIOParallelPort_ReadControl(p,a)            (p)->lpVtbl->ReadControl(p,a)
#define IIOParallelPort_EPPRead(p,a,b,c)            (p)->lpVtbl->EPPRead(p,a,b,c)
#define IIOParallelPort_EPPWrite(p,a,b,c)           (p)->lpVtbl->EPPWrite(p,a,b,c)
#define IIOParallelPort_ECPRead(p,a,b,c)            (p)->lpVtbl->ECPRead(p,a,b,c)
#define IIOParallelPort_ECPWrite(p,a,b,c)           (p)->lpVtbl->ECPWrite(p,a,b,c)
#define IIOParallelPort_IEEE1284Negotiate(p,a)      (p)->lpVtbl->IEEE1284Negotiate(p,a)
#define IIOParallelPort_IEEE1284Terminate(p)        (p)->lpVtbl->IEEE1284Terminate(p)
#define IIOParallelPort_ReadDeviceID(p,a)           (p)->lpVtbl->ReadDeviceID(p,a)
#define IIOParallelPort_DetectDevice(p,a)           (p)->lpVtbl->DetectDevice(p,a)
#define IIOParallelPort_SetInterruptEnable(p,a)     (p)->lpVtbl->SetInterruptEnable(p,a)
#define IIOParallelPort_SetDMAEnable(p,a)           (p)->lpVtbl->SetDMAEnable(p,a)
#define IIOParallelPort_ResetPort(p)                (p)->lpVtbl->ResetPort(p)

#define IIOParallelDevice_GetDeviceInfo(p,a)        (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOParallelDevice_Read(p,a,b,c)             (p)->lpVtbl->Read(p,a,b,c)
#define IIOParallelDevice_Write(p,a,b,c)            (p)->lpVtbl->Write(p,a,b,c)
#define IIOParallelDevice_IORequest(p,a)            (p)->lpVtbl->IORequest(p,a)
#define IIOParallelDevice_ResetDevice(p)            (p)->lpVtbl->ResetDevice(p)

#endif

/**
 * @brief Initialize parallel port family driver
 *
 * Initializes the parallel port family driver and registers controller classes.
 *
 * @retval IO_SUCCESS   Initialization successful
 * @retval IO_ERROR     Initialization failed
 */
IO_RETURN
ParallelInitialize(
    VOID
    );

/**
 * @brief Shutdown parallel port family driver
 *
 * Shuts down the parallel port family driver and releases resources.
 *
 * @retval IO_SUCCESS   Shutdown successful
 */
IO_RETURN
ParallelShutdown(
    VOID
    );

/**
 * @brief Create parallel port controller instance
 *
 * Creates a parallel port interface.
 *
 * @param pProvider         Provider service (PCI device or ISA)
 * @param ppPort            Receives port interface
 *
 * @retval IO_SUCCESS       Port created successfully
 * @retval IO_NO_MEMORY     Insufficient memory
 * @retval IO_NO_DEVICE     Not a parallel port
 */
IO_RETURN
ParallelPortCreate(
    IIOService *pProvider,
    IIOParallelPort **ppPort
    );

/**
 * @brief Create parallel device instance
 *
 * Creates a parallel device interface.
 *
 * @param pPort             Parent port interface
 * @param ppDevice          Receives device interface
 *
 * @retval IO_SUCCESS       Device created successfully
 * @retval IO_NO_MEMORY     Insufficient memory
 */
IO_RETURN
ParallelDeviceCreate(
    IIOParallelPort *pPort,
    IIOParallelDevice **ppDevice
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_PARALLEL_H */
