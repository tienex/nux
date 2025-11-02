/**
 * @file sata.h
 * @brief SATA Family Interface - Serial ATA Storage Driver
 *
 * This header defines the SATA family interface for Serial ATA storage devices,
 * supporting SATA 1.0 (1.5 Gbps), 2.0 (3 Gbps), 3.0 (6 Gbps), and 3.2 (16 Gbps)
 * with AHCI (Advanced Host Controller Interface), NCQ, and port multiplier support.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_SATA_H
#define IOKIT_SATA_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOSATAController interface GUID
 * {E7F8A9B2-5C6D-4E3F-8A9B-1D2E3F4A5B6C}
 */
DEFINE_GUID(IID_IIOSATAController,
    0xE7F8A9B2, 0x5C6D, 0x4E3F, 0x8A, 0x9B, 0x1D, 0x2E, 0x3F, 0x4A, 0x5B, 0x6C);

/**
 * @brief IIOSATADevice interface GUID
 * {F8G9HAB3-6D7E-5F4G-9BAC-2E3F4G5B6C7D}
 */
DEFINE_GUID(IID_IIOSATADevice,
    0xF8G9HAB3, 0x6D7E, 0x5F4G, 0x9B, 0xAC, 0x2E, 0x3F, 0x4G, 0x5B, 0x6C, 0x7D);

/**
 * @brief SATA Specification Versions
 */
typedef enum _SATA_VERSION {
    SATA_VERSION_1_0    = 0x0100,   /**< SATA 1.0 (1.5 Gbps) */
    SATA_VERSION_2_0    = 0x0200,   /**< SATA 2.0 (3 Gbps) */
    SATA_VERSION_3_0    = 0x0300,   /**< SATA 3.0 (6 Gbps) */
    SATA_VERSION_3_1    = 0x0301,   /**< SATA 3.1 (6 Gbps) */
    SATA_VERSION_3_2    = 0x0302,   /**< SATA 3.2 (16 Gbps) */
    SATA_VERSION_3_3    = 0x0303,   /**< SATA 3.3 (16 Gbps) */
    SATA_VERSION_3_4    = 0x0304,   /**< SATA 3.4 (16 Gbps) */
} SATA_VERSION;

/**
 * @brief SATA Transfer Speeds
 */
typedef enum _SATA_SPEED {
    SATA_SPEED_GEN1     = 0,        /**< Gen 1 - 1.5 Gbps */
    SATA_SPEED_GEN2     = 1,        /**< Gen 2 - 3.0 Gbps */
    SATA_SPEED_GEN3     = 2,        /**< Gen 3 - 6.0 Gbps */
    SATA_SPEED_GEN4     = 3,        /**< Gen 4 - 16.0 Gbps */
} SATA_SPEED;

/**
 * @brief SATA Device Types
 */
typedef enum _SATA_DEVICE_TYPE {
    SATA_DEVICE_NONE    = 0,        /**< No device present */
    SATA_DEVICE_ATA     = 1,        /**< ATA device (hard disk) */
    SATA_DEVICE_ATAPI   = 2,        /**< ATAPI device (optical drive) */
    SATA_DEVICE_SEMB    = 3,        /**< Port multiplier SEMB */
    SATA_DEVICE_PM      = 4,        /**< Port multiplier */
} SATA_DEVICE_TYPE;

/**
 * @brief AHCI Controller Capabilities
 */
#define AHCI_CAP_S64A               (1U << 31)  /**< 64-bit Addressing */
#define AHCI_CAP_SNCQ               (1 << 30)   /**< Native Command Queuing */
#define AHCI_CAP_SSNTF              (1 << 29)   /**< SNotification Register */
#define AHCI_CAP_SMPS               (1 << 28)   /**< Mechanical Presence Switch */
#define AHCI_CAP_SSS                (1 << 27)   /**< Staggered Spin-up */
#define AHCI_CAP_SALP               (1 << 26)   /**< Aggressive Link PM */
#define AHCI_CAP_SAL                (1 << 25)   /**< Activity LED */
#define AHCI_CAP_SCLO               (1 << 24)   /**< Command List Override */
#define AHCI_CAP_ISS_SHIFT          20          /**< Interface Speed Support */
#define AHCI_CAP_ISS_MASK           0xF
#define AHCI_CAP_SAM                (1 << 18)   /**< AHCI mode only */
#define AHCI_CAP_SPM                (1 << 17)   /**< Port Multiplier */
#define AHCI_CAP_FBSS               (1 << 16)   /**< FIS-based Switching */
#define AHCI_CAP_PMD                (1 << 15)   /**< PIO Multiple DRQ Block */
#define AHCI_CAP_SSC                (1 << 14)   /**< Slumber State Capable */
#define AHCI_CAP_PSC                (1 << 13)   /**< Partial State Capable */
#define AHCI_CAP_NCS_SHIFT          8           /**< Number of Command Slots */
#define AHCI_CAP_NCS_MASK           0x1F
#define AHCI_CAP_CCCS               (1 << 7)    /**< Command Completion Coalescing */
#define AHCI_CAP_EMS                (1 << 6)    /**< Enclosure Management */
#define AHCI_CAP_SXS                (1 << 5)    /**< External SATA */
#define AHCI_CAP_NP_SHIFT           0           /**< Number of Ports */
#define AHCI_CAP_NP_MASK            0x1F

/**
 * @brief AHCI Global HBA Control
 */
#define AHCI_GHC_AE                 (1U << 31)  /**< AHCI Enable */
#define AHCI_GHC_MRSM               (1 << 2)    /**< MSI Revert to Single Message */
#define AHCI_GHC_IE                 (1 << 1)    /**< Interrupt Enable */
#define AHCI_GHC_HR                 (1 << 0)    /**< HBA Reset */

/**
 * @brief AHCI Port Register Offsets
 */
#define AHCI_PORT_CLB               0x00        /**< Command List Base Address */
#define AHCI_PORT_CLBU              0x04        /**< Command List Base Address Upper */
#define AHCI_PORT_FB                0x08        /**< FIS Base Address */
#define AHCI_PORT_FBU               0x0C        /**< FIS Base Address Upper */
#define AHCI_PORT_IS                0x10        /**< Interrupt Status */
#define AHCI_PORT_IE                0x14        /**< Interrupt Enable */
#define AHCI_PORT_CMD               0x18        /**< Command and Status */
#define AHCI_PORT_TFD               0x20        /**< Task File Data */
#define AHCI_PORT_SIG               0x24        /**< Signature */
#define AHCI_PORT_SSTS              0x28        /**< SATA Status */
#define AHCI_PORT_SCTL              0x2C        /**< SATA Control */
#define AHCI_PORT_SERR              0x30        /**< SATA Error */
#define AHCI_PORT_SACT              0x34        /**< SATA Active */
#define AHCI_PORT_CI                0x38        /**< Command Issue */
#define AHCI_PORT_SNTF              0x3C        /**< SATA Notification */

/**
 * @brief AHCI Port Command and Status
 */
#define AHCI_PORT_CMD_ICC_SHIFT     28          /**< Interface Communication Control */
#define AHCI_PORT_CMD_ICC_MASK      0xF
#define AHCI_PORT_CMD_ASP           (1 << 27)   /**< Aggressive Slumber/Partial */
#define AHCI_PORT_CMD_ALPE          (1 << 26)   /**< Aggressive Link PM Enable */
#define AHCI_PORT_CMD_DLAE          (1 << 25)   /**< Drive LED on ATAPI Enable */
#define AHCI_PORT_CMD_ATAPI         (1 << 24)   /**< Device is ATAPI */
#define AHCI_PORT_CMD_APSTE         (1 << 23)   /**< Auto Partial to Slumber Enable */
#define AHCI_PORT_CMD_FBSCP         (1 << 22)   /**< FIS-based Switching Capable Port */
#define AHCI_PORT_CMD_ESP           (1 << 21)   /**< External SATA Port */
#define AHCI_PORT_CMD_CPD           (1 << 20)   /**< Cold Presence Detect */
#define AHCI_PORT_CMD_MPSP          (1 << 19)   /**< Mechanical Presence Switch */
#define AHCI_PORT_CMD_HPCP          (1 << 18)   /**< Hot Plug Capable Port */
#define AHCI_PORT_CMD_PMA           (1 << 17)   /**< Port Multiplier Attached */
#define AHCI_PORT_CMD_CPS           (1 << 16)   /**< Cold Presence State */
#define AHCI_PORT_CMD_CR            (1 << 15)   /**< Command List Running */
#define AHCI_PORT_CMD_FR            (1 << 14)   /**< FIS Receive Running */
#define AHCI_PORT_CMD_MPSS          (1 << 13)   /**< Mechanical Presence Switch State */
#define AHCI_PORT_CMD_CCS_SHIFT     8           /**< Current Command Slot */
#define AHCI_PORT_CMD_CCS_MASK      0x1F
#define AHCI_PORT_CMD_FRE           (1 << 4)    /**< FIS Receive Enable */
#define AHCI_PORT_CMD_CLO           (1 << 3)    /**< Command List Override */
#define AHCI_PORT_CMD_POD           (1 << 2)    /**< Power On Device */
#define AHCI_PORT_CMD_SUD           (1 << 1)    /**< Spin-Up Device */
#define AHCI_PORT_CMD_ST            (1 << 0)    /**< Start */

/**
 * @brief ATA Commands
 */
#define ATA_CMD_READ_DMA            0xC8        /**< READ DMA */
#define ATA_CMD_READ_DMA_EXT        0x25        /**< READ DMA EXT */
#define ATA_CMD_WRITE_DMA           0xCA        /**< WRITE DMA */
#define ATA_CMD_WRITE_DMA_EXT       0x35        /**< WRITE DMA EXT */
#define ATA_CMD_READ_FPDMA_QUEUED   0x60        /**< READ FPDMA QUEUED (NCQ) */
#define ATA_CMD_WRITE_FPDMA_QUEUED  0x61        /**< WRITE FPDMA QUEUED (NCQ) */
#define ATA_CMD_IDENTIFY_DEVICE     0xEC        /**< IDENTIFY DEVICE */
#define ATA_CMD_IDENTIFY_PACKET     0xA1        /**< IDENTIFY PACKET DEVICE */
#define ATA_CMD_PACKET              0xA0        /**< PACKET */
#define ATA_CMD_SET_FEATURES        0xEF        /**< SET FEATURES */
#define ATA_CMD_FLUSH_CACHE         0xE7        /**< FLUSH CACHE */
#define ATA_CMD_FLUSH_CACHE_EXT     0xEA        /**< FLUSH CACHE EXT */
#define ATA_CMD_STANDBY_IMMEDIATE   0xE0        /**< STANDBY IMMEDIATE */
#define ATA_CMD_IDLE_IMMEDIATE      0xE1        /**< IDLE IMMEDIATE */
#define ATA_CMD_SLEEP               0xE6        /**< SLEEP */
#define ATA_CMD_SMART               0xB0        /**< SMART */

/**
 * @brief ATA Device Identification
 */
typedef struct _ATA_IDENTIFY_DEVICE {
    UINT16  GeneralConfig;          /**< General configuration */
    UINT16  Reserved1[9];
    CHAR8   SerialNumber[20];       /**< Serial number (ASCII) */
    UINT16  Reserved2[3];
    CHAR8   FirmwareRevision[8];    /**< Firmware revision (ASCII) */
    CHAR8   ModelNumber[40];        /**< Model number (ASCII) */
    UINT16  MaxMultiSector;         /**< Maximum sectors per transfer */
    UINT16  Reserved3;
    UINT16  Capabilities[2];        /**< Capabilities */
    UINT16  Reserved4[2];
    UINT16  ValidFields;            /**< Valid fields indicator */
    UINT16  Reserved5[5];
    UINT16  MultiSectorSetting;     /**< Current multi-sector setting */
    UINT32  TotalSectors;           /**< Total addressable sectors (28-bit) */
    UINT16  Reserved6[38];
    UINT64  TotalSectorsExt;        /**< Total addressable sectors (48-bit) */
    UINT16  Reserved7[152];
} ATA_IDENTIFY_DEVICE;

/**
 * @brief SATA Controller Information
 */
typedef struct _SATA_CONTROLLER_INFO {
    SATA_VERSION    Version;            /**< SATA version */
    UINT16          VendorID;           /**< PCI Vendor ID */
    UINT16          DeviceID;           /**< PCI Device ID */
    UINT32          NumPorts;           /**< Number of ports */
    UINT32          NumCommandSlots;    /**< Number of command slots */
    UINT32          PortsImplemented;   /**< Ports implemented bitmask */
    SATA_SPEED      MaxSpeed;           /**< Maximum interface speed */
    BOOLEAN         bNCQSupport;        /**< NCQ (Native Command Queuing) */
    BOOLEAN         bPortMultiplier;    /**< Port Multiplier support */
    BOOLEAN         bHotplug;           /**< Hot-plug support */
    BOOLEAN         bStaggeredSpinup;   /**< Staggered spin-up support */
    BOOLEAN         bALPMSupport;       /**< Aggressive Link PM support */
    BOOLEAN         b64BitDMA;          /**< 64-bit DMA support */
} SATA_CONTROLLER_INFO;

/**
 * @brief SATA Device Information
 */
typedef struct _SATA_DEVICE_INFO {
    UINT32              PortNumber;         /**< Port number */
    SATA_DEVICE_TYPE    DeviceType;         /**< Device type */
    SATA_SPEED          NegotiatedSpeed;    /**< Negotiated link speed */
    CHAR8               SerialNumber[21];   /**< Serial number (null-terminated) */
    CHAR8               ModelNumber[41];    /**< Model number (null-terminated) */
    CHAR8               FirmwareRevision[9];/**< Firmware revision (null-terminated) */
    UINT64              TotalSectors;       /**< Total sectors */
    UINT32              SectorSize;         /**< Sector size in bytes */
    BOOLEAN             bNCQCapable;        /**< NCQ capable */
    BOOLEAN             bRemovable;         /**< Removable media */
    BOOLEAN             bWriteCache;        /**< Write cache enabled */
    UINT32              QueueDepth;         /**< Command queue depth */
} SATA_DEVICE_INFO;

/**
 * @brief Known SATA Controller Vendors
 */
#define SATA_VENDOR_INTEL           0x8086
#define SATA_VENDOR_AMD             0x1022
#define SATA_VENDOR_MARVELL         0x11AB
#define SATA_VENDOR_JMICRON         0x197B
#define SATA_VENDOR_NVIDIA          0x10DE
#define SATA_VENDOR_VIA             0x1106
#define SATA_VENDOR_SIS             0x1039
#define SATA_VENDOR_ATI             0x1002
#define SATA_VENDOR_ASMEDIA         0x1B21
#define SATA_VENDOR_PROMISE         0x105A

/**
 * @brief IIOSATAController - SATA Controller interface
 *
 * This interface represents a SATA host controller (typically AHCI) and
 * provides methods for port management and device enumeration.
 */
#undef INTERFACE
#define INTERFACE IIOSATAController

DECLARE_INTERFACE_(IIOSATAController, IIOService)
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

    // IIOSATAController methods

    /**
     * @brief Get controller information
     *
     * Retrieves comprehensive controller information including AHCI
     * capabilities, port count, and feature support.
     *
     * @param pControllerInfo   Receives controller information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetControllerInfo)(THIS_
        SATA_CONTROLLER_INFO *pControllerInfo
        ) PURE;

    /**
     * @brief Get port count
     *
     * Returns the number of implemented SATA ports.
     *
     * @param puCount           Receives port count
     *
     * @retval IO_SUCCESS       Count retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetPortCount)(THIS_
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Get device on port
     *
     * Retrieves the device interface for a specific port.
     *
     * @param uPort             Port number
     * @param ppDevice          Receives device interface
     *
     * @retval IO_SUCCESS       Device interface retrieved
     * @retval IO_NO_DEVICE     No device on port
     */
    STDMETHOD_(IO_RETURN, GetDevice)(THIS_
        UINT32 uPort,
        IIOSATADevice **ppDevice
        ) PURE;

    /**
     * @brief Reset port
     *
     * Performs a COMRESET on the specified port.
     *
     * @param uPort             Port number
     *
     * @retval IO_SUCCESS       Port reset successful
     * @retval IO_ERROR         Reset failed
     */
    STDMETHOD_(IO_RETURN, ResetPort)(THIS_
        UINT32 uPort
        ) PURE;

    /**
     * @brief Enable/disable port
     *
     * Enables or disables a SATA port.
     *
     * @param uPort             Port number
     * @param bEnable           TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS       Port configured
     */
    STDMETHOD_(IO_RETURN, SetPortEnable)(THIS_
        UINT32 uPort,
        BOOLEAN bEnable
        ) PURE;

    /**
     * @brief Scan ports for devices
     *
     * Scans all ports and enumerates connected devices.
     *
     * @retval IO_SUCCESS       Scan complete
     */
    STDMETHOD_(IO_RETURN, ScanPorts)(THIS) PURE;
};

#undef INTERFACE

/**
 * @brief IIOSATADevice - SATA Device interface
 *
 * This interface represents a SATA device (ATA or ATAPI) and provides
 * methods for I/O operations and device management.
 */
#undef INTERFACE
#define INTERFACE IIOSATADevice

DECLARE_INTERFACE_(IIOSATADevice, IIOService)
{
    // All IIOService methods inherited...

    /**
     * @brief Get device information
     *
     * Retrieves device identification and capability information.
     *
     * @param pDeviceInfo       Receives device information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        SATA_DEVICE_INFO *pDeviceInfo
        ) PURE;

    /**
     * @brief Read sectors
     *
     * Reads sectors from the device using DMA.
     *
     * @param uLBA              Starting logical block address
     * @param uCount            Number of sectors to read
     * @param pBuffer           Buffer to receive data
     * @param cbBuffer          Buffer size in bytes
     *
     * @retval IO_SUCCESS       Read successful
     * @retval IO_ERROR         Read failed
     */
    STDMETHOD_(IO_RETURN, ReadSectors)(THIS_
        UINT64 uLBA,
        UINT32 uCount,
        VOID *pBuffer,
        UINTN cbBuffer
        ) PURE;

    /**
     * @brief Write sectors
     *
     * Writes sectors to the device using DMA.
     *
     * @param uLBA              Starting logical block address
     * @param uCount            Number of sectors to write
     * @param pBuffer           Buffer containing data
     * @param cbBuffer          Buffer size in bytes
     *
     * @retval IO_SUCCESS       Write successful
     * @retval IO_ERROR         Write failed
     */
    STDMETHOD_(IO_RETURN, WriteSectors)(THIS_
        UINT64 uLBA,
        UINT32 uCount,
        CONST VOID *pBuffer,
        UINTN cbBuffer
        ) PURE;

    /**
     * @brief Flush write cache
     *
     * Flushes the device write cache to media.
     *
     * @retval IO_SUCCESS       Flush successful
     * @retval IO_UNSUPPORTED   No write cache
     */
    STDMETHOD_(IO_RETURN, FlushCache)(THIS) PURE;

    /**
     * @brief Set power state
     *
     * Sets the device power state (active, idle, standby, sleep).
     *
     * @param uPowerState       Power state value
     *
     * @retval IO_SUCCESS       Power state set
     * @retval IO_ERROR         Command failed
     */
    STDMETHOD_(IO_RETURN, SetPowerState)(THIS_
        UINT32 uPowerState
        ) PURE;

    /**
     * @brief Execute SMART command
     *
     * Executes a SMART (Self-Monitoring, Analysis and Reporting Technology) command.
     *
     * @param uSubCommand       SMART sub-command
     * @param pBuffer           Data buffer
     * @param cbBuffer          Buffer size
     *
     * @retval IO_SUCCESS       Command successful
     * @retval IO_UNSUPPORTED   SMART not supported
     */
    STDMETHOD_(IO_RETURN, ExecuteSMART)(THIS_
        UINT8 uSubCommand,
        VOID *pBuffer,
        UINTN cbBuffer
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOSATAController methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOSATAController_GetControllerInfo(p,a)    (p)->lpVtbl->GetControllerInfo(p,a)
#define IIOSATAController_GetPortCount(p,a)         (p)->lpVtbl->GetPortCount(p,a)
#define IIOSATAController_GetDevice(p,a,b)          (p)->lpVtbl->GetDevice(p,a,b)
#define IIOSATAController_ResetPort(p,a)            (p)->lpVtbl->ResetPort(p,a)
#define IIOSATAController_SetPortEnable(p,a,b)      (p)->lpVtbl->SetPortEnable(p,a,b)
#define IIOSATAController_ScanPorts(p)              (p)->lpVtbl->ScanPorts(p)

#define IIOSATADevice_GetDeviceInfo(p,a)            (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOSATADevice_ReadSectors(p,a,b,c,d)        (p)->lpVtbl->ReadSectors(p,a,b,c,d)
#define IIOSATADevice_WriteSectors(p,a,b,c,d)       (p)->lpVtbl->WriteSectors(p,a,b,c,d)
#define IIOSATADevice_FlushCache(p)                 (p)->lpVtbl->FlushCache(p)
#define IIOSATADevice_SetPowerState(p,a)            (p)->lpVtbl->SetPowerState(p,a)
#define IIOSATADevice_ExecuteSMART(p,a,b,c)         (p)->lpVtbl->ExecuteSMART(p,a,b,c)

#endif

/**
 * @brief Initialize SATA family driver
 *
 * Initializes the SATA/AHCI family driver and registers controller classes.
 *
 * @retval IO_SUCCESS   Initialization successful
 * @retval IO_ERROR     Initialization failed
 */
IO_RETURN
SATAInitialize(
    VOID
    );

/**
 * @brief Shutdown SATA family driver
 *
 * Shuts down the SATA family driver and releases resources.
 *
 * @retval IO_SUCCESS   Shutdown successful
 */
IO_RETURN
SATAShutdown(
    VOID
    );

/**
 * @brief Create SATA controller instance
 *
 * Creates a SATA controller interface for a PCI device.
 *
 * @param pPCIDevice        PCI device interface
 * @param ppController      Receives controller interface
 *
 * @retval IO_SUCCESS       Controller created successfully
 * @retval IO_NO_MEMORY     Insufficient memory
 * @retval IO_NO_DEVICE     Not a SATA controller
 */
IO_RETURN
SATAControllerCreate(
    IIOService *pPCIDevice,
    IIOSATAController **ppController
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_SATA_H */
