/**
 * @file i2c.h
 * @brief I2C/SMBus Family Interface - Inter-Integrated Circuit Bus Driver
 *
 * This header defines the I2C/SMBus family interface for I2C bus controllers
 * and devices, supporting standard, fast, fast-plus, and high-speed modes,
 * with SMBus 1.0/1.1/2.0/3.0 compatibility and multi-master support.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_I2C_H
#define IOKIT_I2C_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOI2CController interface GUID
 * {B1C2D3E4-6F7A-4C5B-9E8D-2A3B4C5D6E7F}
 */
DEFINE_GUID(IID_IIOI2CController,
    0xB1C2D3E4, 0x6F7A, 0x4C5B, 0x9E, 0x8D, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F);

/**
 * @brief IIOI2CDevice interface GUID
 * {C2D3E4F5-7A8B-5D6E-AF9E-3B4C5D6E7F8A}
 */
DEFINE_GUID(IID_IIOI2CDevice,
    0xC2D3E4F5, 0x7A8B, 0x5D6E, 0xAF, 0x9E, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A);

/**
 * @brief I2C Speed Modes
 */
typedef enum _I2C_SPEED_MODE {
    I2C_SPEED_STANDARD      = 0,        /**< Standard mode: 100 kHz */
    I2C_SPEED_FAST          = 1,        /**< Fast mode: 400 kHz */
    I2C_SPEED_FAST_PLUS     = 2,        /**< Fast mode plus: 1 MHz */
    I2C_SPEED_HIGH_SPEED    = 3,        /**< High-speed mode: 3.4 MHz */
    I2C_SPEED_ULTRA_FAST    = 4,        /**< Ultra fast mode: 5 MHz */
} I2C_SPEED_MODE;

/**
 * @brief SMBus Protocol Versions
 */
typedef enum _SMBUS_VERSION {
    SMBUS_VERSION_1_0       = 0x10,     /**< SMBus 1.0 */
    SMBUS_VERSION_1_1       = 0x11,     /**< SMBus 1.1 */
    SMBUS_VERSION_2_0       = 0x20,     /**< SMBus 2.0 */
    SMBUS_VERSION_3_0       = 0x30,     /**< SMBus 3.0 */
    SMBUS_VERSION_3_1       = 0x31,     /**< SMBus 3.1 */
} SMBUS_VERSION;

/**
 * @brief I2C Address Modes
 */
typedef enum _I2C_ADDRESS_MODE {
    I2C_ADDRESS_7BIT        = 0,        /**< 7-bit addressing */
    I2C_ADDRESS_10BIT       = 1,        /**< 10-bit addressing */
} I2C_ADDRESS_MODE;

/**
 * @brief I2C Transfer Flags
 */
#define I2C_FLAG_READ           (1 << 0)    /**< Read operation */
#define I2C_FLAG_WRITE          (1 << 1)    /**< Write operation */
#define I2C_FLAG_NO_START       (1 << 2)    /**< No START condition */
#define I2C_FLAG_NO_STOP        (1 << 3)    /**< No STOP condition */
#define I2C_FLAG_IGNORE_NAK     (1 << 4)    /**< Ignore NAK */
#define I2C_FLAG_10BIT_ADDR     (1 << 5)    /**< 10-bit address */

/**
 * @brief SMBus Commands
 */
typedef enum _SMBUS_COMMAND {
    SMBUS_CMD_QUICK         = 0,        /**< Quick Command */
    SMBUS_CMD_BYTE          = 1,        /**< Send/Receive Byte */
    SMBUS_CMD_BYTE_DATA     = 2,        /**< Read/Write Byte Data */
    SMBUS_CMD_WORD_DATA     = 3,        /**< Read/Write Word Data */
    SMBUS_CMD_BLOCK_DATA    = 4,        /**< Read/Write Block Data */
    SMBUS_CMD_PROC_CALL     = 5,        /**< Process Call */
    SMBUS_CMD_BLOCK_PROC    = 6,        /**< Block Process Call */
    SMBUS_CMD_I2C_BLOCK     = 7,        /**< I2C Block Read/Write */
} SMBUS_COMMAND;

/**
 * @brief I2C Controller Capabilities
 */
#define I2C_CAP_STANDARD_MODE   (1 << 0)    /**< Standard mode (100 kHz) */
#define I2C_CAP_FAST_MODE       (1 << 1)    /**< Fast mode (400 kHz) */
#define I2C_CAP_FAST_PLUS       (1 << 2)    /**< Fast mode plus (1 MHz) */
#define I2C_CAP_HIGH_SPEED      (1 << 3)    /**< High-speed mode (3.4 MHz) */
#define I2C_CAP_10BIT_ADDR      (1 << 4)    /**< 10-bit addressing */
#define I2C_CAP_MULTI_MASTER    (1 << 5)    /**< Multi-master support */
#define I2C_CAP_CLOCK_STRETCH   (1 << 6)    /**< Clock stretching */
#define I2C_CAP_SMBUS           (1 << 7)    /**< SMBus compatibility */
#define I2C_CAP_PEC             (1 << 8)    /**< Packet Error Checking */
#define I2C_CAP_ALERT           (1 << 9)    /**< SMBus Alert support */

/**
 * @brief I2C Controller Information
 */
typedef struct _I2C_CONTROLLER_INFO {
    UINT16          VendorID;           /**< PCI Vendor ID */
    UINT16          DeviceID;           /**< PCI Device ID */
    UINT32          Capabilities;       /**< Capability flags */
    I2C_SPEED_MODE  MaxSpeed;           /**< Maximum speed mode */
    SMBUS_VERSION   SMBusVersion;       /**< SMBus version (if supported) */
    UINT32          MaxTransferSize;    /**< Maximum transfer size */
    UINT32          ClockFrequency;     /**< Clock frequency in Hz */
    BOOLEAN         bSMBusSupport;      /**< SMBus compatibility */
    BOOLEAN         bMultiMaster;       /**< Multi-master capable */
    BOOLEAN         bDMASupport;        /**< DMA support */
} I2C_CONTROLLER_INFO;

/**
 * @brief I2C Device Information
 */
typedef struct _I2C_DEVICE_INFO {
    UINT16              Address;            /**< Device address */
    I2C_ADDRESS_MODE    AddressMode;        /**< Address mode (7-bit/10-bit) */
    I2C_SPEED_MODE      MaxSpeed;           /**< Maximum supported speed */
    CHAR8               DeviceName[32];     /**< Device name */
    UINT32              Capabilities;       /**< Device capabilities */
} I2C_DEVICE_INFO;

/**
 * @brief I2C Transfer Message
 */
typedef struct _I2C_MESSAGE {
    UINT16      Address;            /**< Device address */
    UINT16      Flags;              /**< Transfer flags */
    UINT16      Length;             /**< Data length */
    UINT8      *pData;              /**< Data buffer */
} I2C_MESSAGE;

/**
 * @brief SMBus Transfer
 */
typedef struct _SMBUS_TRANSFER {
    UINT8       Address;            /**< Device address */
    SMBUS_COMMAND Command;          /**< SMBus command type */
    UINT8       CommandCode;        /**< Command code/register */
    UINT8       DataLength;         /**< Data length (for block operations) */
    UINT8       Data[32];           /**< Data buffer */
    BOOLEAN     bRead;              /**< TRUE for read, FALSE for write */
    BOOLEAN     bUsePEC;            /**< Use Packet Error Checking */
} SMBUS_TRANSFER;

/**
 * @brief Known I2C/SMBus Controller Vendors
 */
#define I2C_VENDOR_INTEL            0x8086
#define I2C_VENDOR_AMD              0x1022
#define I2C_VENDOR_NVIDIA           0x10DE
#define I2C_VENDOR_VIA              0x1106
#define I2C_VENDOR_SIS              0x1039
#define I2C_VENDOR_ATI              0x1002

/**
 * @brief Common I2C Device Types/Addresses
 */
#define I2C_ADDR_EEPROM_BASE        0x50    /**< EEPROM base address */
#define I2C_ADDR_RTC_DS1307         0x68    /**< DS1307 RTC */
#define I2C_ADDR_RTC_PCF8563        0x51    /**< PCF8563 RTC */
#define I2C_ADDR_TEMP_LM75          0x48    /**< LM75 temperature sensor */
#define I2C_ADDR_TEMP_LM92          0x4C    /**< LM92 temperature sensor */
#define I2C_ADDR_GPIO_PCF8574       0x20    /**< PCF8574 GPIO expander */
#define I2C_ADDR_ADC_ADS1115        0x48    /**< ADS1115 ADC */

/**
 * @brief IIOI2CController - I2C Controller interface
 *
 * This interface represents an I2C/SMBus controller and provides methods
 * for bus operations, device enumeration, and transfer management.
 */
#undef INTERFACE
#define INTERFACE IIOI2CController

DECLARE_INTERFACE_(IIOI2CController, IIOService)
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

    // IIOI2CController methods

    /**
     * @brief Get controller information
     *
     * Retrieves comprehensive controller information including capabilities,
     * speed modes, and SMBus support.
     *
     * @param pControllerInfo   Receives controller information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetControllerInfo)(THIS_
        I2C_CONTROLLER_INFO *pControllerInfo
        ) PURE;

    /**
     * @brief Set bus speed
     *
     * Sets the I2C bus clock speed.
     *
     * @param SpeedMode         Desired speed mode
     *
     * @retval IO_SUCCESS       Speed set successfully
     * @retval IO_UNSUPPORTED   Speed mode not supported
     */
    STDMETHOD_(IO_RETURN, SetSpeed)(THIS_
        I2C_SPEED_MODE SpeedMode
        ) PURE;

    /**
     * @brief Transfer messages
     *
     * Performs I2C transfer operations (read/write).
     *
     * @param pMessages         Array of I2C messages
     * @param uCount            Number of messages
     *
     * @retval IO_SUCCESS       Transfer successful
     * @retval IO_ERROR         Transfer failed
     * @retval IO_TIMEOUT       Transfer timeout
     */
    STDMETHOD_(IO_RETURN, Transfer)(THIS_
        I2C_MESSAGE *pMessages,
        UINT32 uCount
        ) PURE;

    /**
     * @brief Read from device
     *
     * Reads data from an I2C device.
     *
     * @param uAddress          Device address
     * @param pBuffer           Buffer to receive data
     * @param cbLength          Number of bytes to read
     *
     * @retval IO_SUCCESS       Read successful
     * @retval IO_ERROR         Read failed
     */
    STDMETHOD_(IO_RETURN, Read)(THIS_
        UINT16 uAddress,
        UINT8 *pBuffer,
        UINT16 cbLength
        ) PURE;

    /**
     * @brief Write to device
     *
     * Writes data to an I2C device.
     *
     * @param uAddress          Device address
     * @param pBuffer           Data to write
     * @param cbLength          Number of bytes to write
     *
     * @retval IO_SUCCESS       Write successful
     * @retval IO_ERROR         Write failed
     */
    STDMETHOD_(IO_RETURN, Write)(THIS_
        UINT16 uAddress,
        CONST UINT8 *pBuffer,
        UINT16 cbLength
        ) PURE;

    /**
     * @brief Read register
     *
     * Reads a register from an I2C device.
     *
     * @param uAddress          Device address
     * @param uRegister         Register address
     * @param pValue            Receives register value
     *
     * @retval IO_SUCCESS       Read successful
     * @retval IO_ERROR         Read failed
     */
    STDMETHOD_(IO_RETURN, ReadRegister)(THIS_
        UINT16 uAddress,
        UINT8 uRegister,
        UINT8 *pValue
        ) PURE;

    /**
     * @brief Write register
     *
     * Writes a register to an I2C device.
     *
     * @param uAddress          Device address
     * @param uRegister         Register address
     * @param uValue            Value to write
     *
     * @retval IO_SUCCESS       Write successful
     * @retval IO_ERROR         Write failed
     */
    STDMETHOD_(IO_RETURN, WriteRegister)(THIS_
        UINT16 uAddress,
        UINT8 uRegister,
        UINT8 uValue
        ) PURE;

    /**
     * @brief SMBus transfer
     *
     * Performs an SMBus protocol transfer.
     *
     * @param pTransfer         SMBus transfer structure
     *
     * @retval IO_SUCCESS       Transfer successful
     * @retval IO_UNSUPPORTED   SMBus not supported
     * @retval IO_ERROR         Transfer failed
     */
    STDMETHOD_(IO_RETURN, SMBusTransfer)(THIS_
        SMBUS_TRANSFER *pTransfer
        ) PURE;

    /**
     * @brief Scan bus for devices
     *
     * Scans the I2C bus for responding devices.
     *
     * @param pAddresses        Array to receive device addresses
     * @param puCount           On input: max addresses; On output: actual count
     *
     * @retval IO_SUCCESS       Scan complete
     */
    STDMETHOD_(IO_RETURN, ScanBus)(THIS_
        UINT16 *pAddresses,
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Reset bus
     *
     * Resets the I2C bus (clears stuck conditions).
     *
     * @retval IO_SUCCESS       Bus reset successful
     */
    STDMETHOD_(IO_RETURN, ResetBus)(THIS) PURE;
};

#undef INTERFACE

/**
 * @brief IIOI2CDevice - I2C Device interface
 *
 * This interface represents an I2C device and provides convenient methods
 * for device-specific operations.
 */
#undef INTERFACE
#define INTERFACE IIOI2CDevice

DECLARE_INTERFACE_(IIOI2CDevice, IIOService)
{
    // All IIOService methods inherited...

    /**
     * @brief Get device information
     *
     * Retrieves device address and capability information.
     *
     * @param pDeviceInfo       Receives device information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        I2C_DEVICE_INFO *pDeviceInfo
        ) PURE;

    /**
     * @brief Read from device
     *
     * Reads data from the I2C device.
     *
     * @param pBuffer           Buffer to receive data
     * @param cbLength          Number of bytes to read
     *
     * @retval IO_SUCCESS       Read successful
     * @retval IO_ERROR         Read failed
     */
    STDMETHOD_(IO_RETURN, Read)(THIS_
        UINT8 *pBuffer,
        UINT16 cbLength
        ) PURE;

    /**
     * @brief Write to device
     *
     * Writes data to the I2C device.
     *
     * @param pBuffer           Data to write
     * @param cbLength          Number of bytes to write
     *
     * @retval IO_SUCCESS       Write successful
     * @retval IO_ERROR         Write failed
     */
    STDMETHOD_(IO_RETURN, Write)(THIS_
        CONST UINT8 *pBuffer,
        UINT16 cbLength
        ) PURE;

    /**
     * @brief Read register
     *
     * Reads a register from the device.
     *
     * @param uRegister         Register address
     * @param pValue            Receives register value
     *
     * @retval IO_SUCCESS       Read successful
     * @retval IO_ERROR         Read failed
     */
    STDMETHOD_(IO_RETURN, ReadRegister)(THIS_
        UINT8 uRegister,
        UINT8 *pValue
        ) PURE;

    /**
     * @brief Write register
     *
     * Writes a register to the device.
     *
     * @param uRegister         Register address
     * @param uValue            Value to write
     *
     * @retval IO_SUCCESS       Write successful
     * @retval IO_ERROR         Write failed
     */
    STDMETHOD_(IO_RETURN, WriteRegister)(THIS_
        UINT8 uRegister,
        UINT8 uValue
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOI2CController methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOI2CController_GetControllerInfo(p,a)     (p)->lpVtbl->GetControllerInfo(p,a)
#define IIOI2CController_SetSpeed(p,a)              (p)->lpVtbl->SetSpeed(p,a)
#define IIOI2CController_Transfer(p,a,b)            (p)->lpVtbl->Transfer(p,a,b)
#define IIOI2CController_Read(p,a,b,c)              (p)->lpVtbl->Read(p,a,b,c)
#define IIOI2CController_Write(p,a,b,c)             (p)->lpVtbl->Write(p,a,b,c)
#define IIOI2CController_ReadRegister(p,a,b,c)      (p)->lpVtbl->ReadRegister(p,a,b,c)
#define IIOI2CController_WriteRegister(p,a,b,c)     (p)->lpVtbl->WriteRegister(p,a,b,c)
#define IIOI2CController_SMBusTransfer(p,a)         (p)->lpVtbl->SMBusTransfer(p,a)
#define IIOI2CController_ScanBus(p,a,b)             (p)->lpVtbl->ScanBus(p,a,b)
#define IIOI2CController_ResetBus(p)                (p)->lpVtbl->ResetBus(p)

#define IIOI2CDevice_GetDeviceInfo(p,a)             (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOI2CDevice_Read(p,a,b)                    (p)->lpVtbl->Read(p,a,b)
#define IIOI2CDevice_Write(p,a,b)                   (p)->lpVtbl->Write(p,a,b)
#define IIOI2CDevice_ReadRegister(p,a,b)            (p)->lpVtbl->ReadRegister(p,a,b)
#define IIOI2CDevice_WriteRegister(p,a,b)           (p)->lpVtbl->WriteRegister(p,a,b)

#endif

/**
 * @brief Initialize I2C/SMBus family driver
 *
 * Initializes the I2C/SMBus family driver and registers controller classes.
 *
 * @retval IO_SUCCESS   Initialization successful
 * @retval IO_ERROR     Initialization failed
 */
IO_RETURN
I2CInitialize(
    VOID
    );

/**
 * @brief Shutdown I2C/SMBus family driver
 *
 * Shuts down the I2C family driver and releases resources.
 *
 * @retval IO_SUCCESS   Shutdown successful
 */
IO_RETURN
I2CShutdown(
    VOID
    );

/**
 * @brief Create I2C controller instance
 *
 * Creates an I2C controller interface for a PCI device.
 *
 * @param pPCIDevice        PCI device interface
 * @param ppController      Receives controller interface
 *
 * @retval IO_SUCCESS       Controller created successfully
 * @retval IO_NO_MEMORY     Insufficient memory
 * @retval IO_NO_DEVICE     Not an I2C controller
 */
IO_RETURN
I2CControllerCreate(
    IIOService *pPCIDevice,
    IIOI2CController **ppController
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_I2C_H */
