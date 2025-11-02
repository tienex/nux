/**
 * @file spi.h
 * @brief SPI Family Interface - Serial Peripheral Interface Bus Driver
 *
 * This header defines the SPI family interface for SPI bus controllers
 * and devices, supporting standard SPI, Dual SPI, and Quad SPI (QSPI) modes
 * with master/slave operation and various transfer modes.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_SPI_H
#define IOKIT_SPI_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOSPIController interface GUID
 * {D3E4F5A6-8B9C-6E7F-BA9D-4C5D6E7F8A9B}
 */
DEFINE_GUID(IID_IIOSPIController,
    0xD3E4F5A6, 0x8B9C, 0x6E7F, 0xBA, 0x9D, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B);

/**
 * @brief IIOSPIDevice interface GUID
 * {E4F5G6H7-9CAD-7F8G-CB9E-5D6E7F8A9BAC}
 */
DEFINE_GUID(IID_IIOSPIDevice,
    0xE4F5G6H7, 0x9CAD, 0x7F8G, 0xCB, 0x9E, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0xAC);

/**
 * @brief SPI Modes (Clock Polarity and Phase)
 */
typedef enum _SPI_MODE {
    SPI_MODE_0          = 0,        /**< CPOL=0, CPHA=0 */
    SPI_MODE_1          = 1,        /**< CPOL=0, CPHA=1 */
    SPI_MODE_2          = 2,        /**< CPOL=1, CPHA=0 */
    SPI_MODE_3          = 3,        /**< CPOL=1, CPHA=1 */
} SPI_MODE;

/**
 * @brief SPI Transfer Modes
 */
typedef enum _SPI_TRANSFER_MODE {
    SPI_TRANSFER_STANDARD   = 0,    /**< Standard SPI (single data line) */
    SPI_TRANSFER_DUAL       = 1,    /**< Dual SPI (2 data lines) */
    SPI_TRANSFER_QUAD       = 2,    /**< Quad SPI (4 data lines) */
    SPI_TRANSFER_OCTAL      = 3,    /**< Octal SPI (8 data lines) */
} SPI_TRANSFER_MODE;

/**
 * @brief SPI Bit Order
 */
typedef enum _SPI_BIT_ORDER {
    SPI_BIT_ORDER_MSB_FIRST = 0,    /**< MSB first (most common) */
    SPI_BIT_ORDER_LSB_FIRST = 1,    /**< LSB first */
} SPI_BIT_ORDER;

/**
 * @brief SPI Chip Select Polarity
 */
typedef enum _SPI_CS_POLARITY {
    SPI_CS_ACTIVE_LOW   = 0,        /**< CS active low (most common) */
    SPI_CS_ACTIVE_HIGH  = 1,        /**< CS active high */
} SPI_CS_POLARITY;

/**
 * @brief SPI Controller Capabilities
 */
#define SPI_CAP_MASTER          (1 << 0)    /**< Master mode */
#define SPI_CAP_SLAVE           (1 << 1)    /**< Slave mode */
#define SPI_CAP_DUAL            (1 << 2)    /**< Dual SPI support */
#define SPI_CAP_QUAD            (1 << 3)    /**< Quad SPI support */
#define SPI_CAP_OCTAL           (1 << 4)    /**< Octal SPI support */
#define SPI_CAP_3WIRE           (1 << 5)    /**< 3-wire mode */
#define SPI_CAP_LSB_FIRST       (1 << 6)    /**< LSB first support */
#define SPI_CAP_CS_HIGH         (1 << 7)    /**< CS active high */
#define SPI_CAP_LOOPBACK        (1 << 8)    /**< Loopback mode */
#define SPI_CAP_DMA             (1 << 9)    /**< DMA support */
#define SPI_CAP_MULTI_CS        (1 << 10)   /**< Multiple chip selects */

/**
 * @brief SPI Transfer Flags
 */
#define SPI_FLAG_CS_KEEP_ACTIVE (1 << 0)    /**< Keep CS active after transfer */
#define SPI_FLAG_3WIRE          (1 << 1)    /**< Use 3-wire mode */
#define SPI_FLAG_LSB_FIRST      (1 << 2)    /**< LSB first */
#define SPI_FLAG_LOOPBACK       (1 << 3)    /**< Loopback mode */
#define SPI_FLAG_DUAL           (1 << 4)    /**< Use dual mode */
#define SPI_FLAG_QUAD           (1 << 5)    /**< Use quad mode */

/**
 * @brief SPI Controller Information
 */
typedef struct _SPI_CONTROLLER_INFO {
    UINT16          VendorID;           /**< PCI Vendor ID */
    UINT16          DeviceID;           /**< PCI Device ID */
    UINT32          Capabilities;       /**< Capability flags */
    UINT32          MaxClockHz;         /**< Maximum clock frequency (Hz) */
    UINT32          MinClockHz;         /**< Minimum clock frequency (Hz) */
    UINT32          MaxTransferSize;    /**< Maximum transfer size (bytes) */
    UINT32          NumChipSelects;     /**< Number of chip selects */
    UINT8           FIFODepth;          /**< FIFO depth (if applicable) */
    BOOLEAN         bDMASupport;        /**< DMA support */
    BOOLEAN         bInterruptMode;     /**< Interrupt-driven mode */
} SPI_CONTROLLER_INFO;

/**
 * @brief SPI Device Configuration
 */
typedef struct _SPI_DEVICE_CONFIG {
    UINT32              ChipSelect;         /**< Chip select number */
    UINT32              ClockHz;            /**< Clock frequency (Hz) */
    SPI_MODE            Mode;               /**< SPI mode (CPOL/CPHA) */
    SPI_BIT_ORDER       BitOrder;           /**< Bit order */
    SPI_CS_POLARITY     CSPolarity;         /**< Chip select polarity */
    UINT8               BitsPerWord;        /**< Bits per word (8, 16, 32) */
    SPI_TRANSFER_MODE   TransferMode;       /**< Transfer mode */
    CHAR8               DeviceName[32];     /**< Device name */
} SPI_DEVICE_CONFIG;

/**
 * @brief SPI Transfer Structure
 */
typedef struct _SPI_TRANSFER {
    CONST UINT8    *pTxBuffer;          /**< Transmit buffer */
    UINT8          *pRxBuffer;          /**< Receive buffer */
    UINT32          Length;             /**< Transfer length (bytes) */
    UINT32          Flags;              /**< Transfer flags */
    UINT32          DelayUsecs;         /**< Delay after transfer (microseconds) */
    UINT32          SpeedHz;            /**< Override speed (0 = use default) */
    UINT8           BitsPerWord;        /**< Override bits per word (0 = use default) */
    SPI_TRANSFER_MODE TransferMode;     /**< Transfer mode for this operation */
} SPI_TRANSFER;

/**
 * @brief Common SPI Flash Commands (for reference)
 */
#define SPI_FLASH_CMD_WRITE_ENABLE      0x06    /**< Write Enable */
#define SPI_FLASH_CMD_WRITE_DISABLE     0x04    /**< Write Disable */
#define SPI_FLASH_CMD_READ_STATUS       0x05    /**< Read Status Register */
#define SPI_FLASH_CMD_WRITE_STATUS      0x01    /**< Write Status Register */
#define SPI_FLASH_CMD_READ_DATA         0x03    /**< Read Data */
#define SPI_FLASH_CMD_FAST_READ         0x0B    /**< Fast Read */
#define SPI_FLASH_CMD_PAGE_PROGRAM      0x02    /**< Page Program */
#define SPI_FLASH_CMD_SECTOR_ERASE      0x20    /**< Sector Erase (4KB) */
#define SPI_FLASH_CMD_BLOCK_ERASE_32K   0x52    /**< Block Erase (32KB) */
#define SPI_FLASH_CMD_BLOCK_ERASE_64K   0xD8    /**< Block Erase (64KB) */
#define SPI_FLASH_CMD_CHIP_ERASE        0xC7    /**< Chip Erase */
#define SPI_FLASH_CMD_READ_JEDEC_ID     0x9F    /**< Read JEDEC ID */

/**
 * @brief Known SPI Controller Vendors
 */
#define SPI_VENDOR_INTEL            0x8086
#define SPI_VENDOR_AMD              0x1022
#define SPI_VENDOR_XILINX           0x10EE
#define SPI_VENDOR_ALTERA           0x1172
#define SPI_VENDOR_TI               0x104C
#define SPI_VENDOR_FTDI             0x0403

/**
 * @brief IIOSPIController - SPI Controller interface
 *
 * This interface represents an SPI controller and provides methods
 * for bus configuration, device management, and data transfer.
 */
#undef INTERFACE
#define INTERFACE IIOSPIController

DECLARE_INTERFACE_(IIOSPIController, IIOService)
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

    // IIOSPIController methods

    /**
     * @brief Get controller information
     *
     * Retrieves comprehensive controller information including capabilities,
     * clock limits, and chip select configuration.
     *
     * @param pControllerInfo   Receives controller information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetControllerInfo)(THIS_
        SPI_CONTROLLER_INFO *pControllerInfo
        ) PURE;

    /**
     * @brief Configure device
     *
     * Configures an SPI device on a specific chip select.
     *
     * @param pConfig           Device configuration
     *
     * @retval IO_SUCCESS       Device configured successfully
     * @retval IO_BAD_ARGUMENT  Invalid configuration
     */
    STDMETHOD_(IO_RETURN, ConfigureDevice)(THIS_
        CONST SPI_DEVICE_CONFIG *pConfig
        ) PURE;

    /**
     * @brief Transfer data
     *
     * Performs a full-duplex SPI transfer.
     *
     * @param uChipSelect       Chip select number
     * @param pTransfers        Array of transfer structures
     * @param uCount            Number of transfers
     *
     * @retval IO_SUCCESS       Transfer successful
     * @retval IO_ERROR         Transfer failed
     * @retval IO_TIMEOUT       Transfer timeout
     */
    STDMETHOD_(IO_RETURN, Transfer)(THIS_
        UINT32 uChipSelect,
        SPI_TRANSFER *pTransfers,
        UINT32 uCount
        ) PURE;

    /**
     * @brief Write data
     *
     * Writes data to an SPI device (transmit only).
     *
     * @param uChipSelect       Chip select number
     * @param pBuffer           Data to write
     * @param cbLength          Number of bytes to write
     *
     * @retval IO_SUCCESS       Write successful
     * @retval IO_ERROR         Write failed
     */
    STDMETHOD_(IO_RETURN, Write)(THIS_
        UINT32 uChipSelect,
        CONST UINT8 *pBuffer,
        UINT32 cbLength
        ) PURE;

    /**
     * @brief Read data
     *
     * Reads data from an SPI device (receive only).
     *
     * @param uChipSelect       Chip select number
     * @param pBuffer           Buffer to receive data
     * @param cbLength          Number of bytes to read
     *
     * @retval IO_SUCCESS       Read successful
     * @retval IO_ERROR         Read failed
     */
    STDMETHOD_(IO_RETURN, Read)(THIS_
        UINT32 uChipSelect,
        UINT8 *pBuffer,
        UINT32 cbLength
        ) PURE;

    /**
     * @brief Write and read data
     *
     * Performs a full-duplex write and read operation.
     *
     * @param uChipSelect       Chip select number
     * @param pTxBuffer         Data to transmit
     * @param pRxBuffer         Buffer to receive data
     * @param cbLength          Number of bytes to transfer
     *
     * @retval IO_SUCCESS       Transfer successful
     * @retval IO_ERROR         Transfer failed
     */
    STDMETHOD_(IO_RETURN, WriteRead)(THIS_
        UINT32 uChipSelect,
        CONST UINT8 *pTxBuffer,
        UINT8 *pRxBuffer,
        UINT32 cbLength
        ) PURE;

    /**
     * @brief Set chip select state
     *
     * Manually controls the chip select line.
     *
     * @param uChipSelect       Chip select number
     * @param bActive           TRUE to activate, FALSE to deactivate
     *
     * @retval IO_SUCCESS       CS state set
     * @retval IO_BAD_ARGUMENT  Invalid chip select
     */
    STDMETHOD_(IO_RETURN, SetChipSelect)(THIS_
        UINT32 uChipSelect,
        BOOLEAN bActive
        ) PURE;

    /**
     * @brief Set clock frequency
     *
     * Sets the SPI clock frequency for a chip select.
     *
     * @param uChipSelect       Chip select number
     * @param uFrequencyHz      Desired frequency in Hz
     *
     * @retval IO_SUCCESS       Frequency set
     * @retval IO_UNSUPPORTED   Frequency not supported
     */
    STDMETHOD_(IO_RETURN, SetClock)(THIS_
        UINT32 uChipSelect,
        UINT32 uFrequencyHz
        ) PURE;

    /**
     * @brief Set SPI mode
     *
     * Sets the SPI mode (CPOL/CPHA) for a chip select.
     *
     * @param uChipSelect       Chip select number
     * @param Mode              SPI mode
     *
     * @retval IO_SUCCESS       Mode set
     */
    STDMETHOD_(IO_RETURN, SetMode)(THIS_
        UINT32 uChipSelect,
        SPI_MODE Mode
        ) PURE;

    /**
     * @brief Get device interface
     *
     * Retrieves a device interface for a specific chip select.
     *
     * @param uChipSelect       Chip select number
     * @param ppDevice          Receives device interface
     *
     * @retval IO_SUCCESS       Device interface retrieved
     * @retval IO_NO_DEVICE     No device configured
     */
    STDMETHOD_(IO_RETURN, GetDevice)(THIS_
        UINT32 uChipSelect,
        IIOSPIDevice **ppDevice
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOSPIDevice - SPI Device interface
 *
 * This interface represents an SPI device and provides convenient methods
 * for device-specific operations without specifying chip select each time.
 */
#undef INTERFACE
#define INTERFACE IIOSPIDevice

DECLARE_INTERFACE_(IIOSPIDevice, IIOService)
{
    // All IIOService methods inherited...

    /**
     * @brief Get device configuration
     *
     * Retrieves device configuration information.
     *
     * @param pConfig           Receives device configuration
     *
     * @retval IO_SUCCESS       Configuration retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetConfiguration)(THIS_
        SPI_DEVICE_CONFIG *pConfig
        ) PURE;

    /**
     * @brief Transfer data
     *
     * Performs SPI transfers on this device.
     *
     * @param pTransfers        Array of transfer structures
     * @param uCount            Number of transfers
     *
     * @retval IO_SUCCESS       Transfer successful
     * @retval IO_ERROR         Transfer failed
     */
    STDMETHOD_(IO_RETURN, Transfer)(THIS_
        SPI_TRANSFER *pTransfers,
        UINT32 uCount
        ) PURE;

    /**
     * @brief Write data
     *
     * Writes data to the device.
     *
     * @param pBuffer           Data to write
     * @param cbLength          Number of bytes to write
     *
     * @retval IO_SUCCESS       Write successful
     * @retval IO_ERROR         Write failed
     */
    STDMETHOD_(IO_RETURN, Write)(THIS_
        CONST UINT8 *pBuffer,
        UINT32 cbLength
        ) PURE;

    /**
     * @brief Read data
     *
     * Reads data from the device.
     *
     * @param pBuffer           Buffer to receive data
     * @param cbLength          Number of bytes to read
     *
     * @retval IO_SUCCESS       Read successful
     * @retval IO_ERROR         Read failed
     */
    STDMETHOD_(IO_RETURN, Read)(THIS_
        UINT8 *pBuffer,
        UINT32 cbLength
        ) PURE;

    /**
     * @brief Write and read data
     *
     * Performs a full-duplex write and read.
     *
     * @param pTxBuffer         Data to transmit
     * @param pRxBuffer         Buffer to receive data
     * @param cbLength          Number of bytes to transfer
     *
     * @retval IO_SUCCESS       Transfer successful
     * @retval IO_ERROR         Transfer failed
     */
    STDMETHOD_(IO_RETURN, WriteRead)(THIS_
        CONST UINT8 *pTxBuffer,
        UINT8 *pRxBuffer,
        UINT32 cbLength
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOSPIController methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOSPIController_GetControllerInfo(p,a)     (p)->lpVtbl->GetControllerInfo(p,a)
#define IIOSPIController_ConfigureDevice(p,a)       (p)->lpVtbl->ConfigureDevice(p,a)
#define IIOSPIController_Transfer(p,a,b,c)          (p)->lpVtbl->Transfer(p,a,b,c)
#define IIOSPIController_Write(p,a,b,c)             (p)->lpVtbl->Write(p,a,b,c)
#define IIOSPIController_Read(p,a,b,c)              (p)->lpVtbl->Read(p,a,b,c)
#define IIOSPIController_WriteRead(p,a,b,c,d)       (p)->lpVtbl->WriteRead(p,a,b,c,d)
#define IIOSPIController_SetChipSelect(p,a,b)       (p)->lpVtbl->SetChipSelect(p,a,b)
#define IIOSPIController_SetClock(p,a,b)            (p)->lpVtbl->SetClock(p,a,b)
#define IIOSPIController_SetMode(p,a,b)             (p)->lpVtbl->SetMode(p,a,b)
#define IIOSPIController_GetDevice(p,a,b)           (p)->lpVtbl->GetDevice(p,a,b)

#define IIOSPIDevice_GetConfiguration(p,a)          (p)->lpVtbl->GetConfiguration(p,a)
#define IIOSPIDevice_Transfer(p,a,b)                (p)->lpVtbl->Transfer(p,a,b)
#define IIOSPIDevice_Write(p,a,b)                   (p)->lpVtbl->Write(p,a,b)
#define IIOSPIDevice_Read(p,a,b)                    (p)->lpVtbl->Read(p,a,b)
#define IIOSPIDevice_WriteRead(p,a,b,c)             (p)->lpVtbl->WriteRead(p,a,b,c)

#endif

/**
 * @brief Initialize SPI family driver
 *
 * Initializes the SPI family driver and registers controller classes.
 *
 * @retval IO_SUCCESS   Initialization successful
 * @retval IO_ERROR     Initialization failed
 */
IO_RETURN
SPIInitialize(
    VOID
    );

/**
 * @brief Shutdown SPI family driver
 *
 * Shuts down the SPI family driver and releases resources.
 *
 * @retval IO_SUCCESS   Shutdown successful
 */
IO_RETURN
SPIShutdown(
    VOID
    );

/**
 * @brief Create SPI controller instance
 *
 * Creates an SPI controller interface for a PCI device.
 *
 * @param pPCIDevice        PCI device interface
 * @param ppController      Receives controller interface
 *
 * @retval IO_SUCCESS       Controller created successfully
 * @retval IO_NO_MEMORY     Insufficient memory
 * @retval IO_NO_DEVICE     Not an SPI controller
 */
IO_RETURN
SPIControllerCreate(
    IIOService *pPCIDevice,
    IIOSPIController **ppController
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_SPI_H */
