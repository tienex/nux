/**
 * @file scsi.h
 * @brief SCSI/SAS Family Interface - SCSI and Serial Attached SCSI Driver
 *
 * This header defines the SCSI family interface for SCSI and SAS storage devices,
 * supporting SCSI-1/2/3, Ultra SCSI, Wide SCSI, and SAS-1/2/3/4 with full
 * command set implementation for storage, tape, and CD/DVD devices.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_SCSI_H
#define IOKIT_SCSI_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOSCSIController interface GUID
 * {A9B8C7D6-5E4F-3A2B-8C9D-1E2F3A4B5C6D}
 */
DEFINE_GUID(IID_IIOSCSIController,
    0xA9B8C7D6, 0x5E4F, 0x3A2B, 0x8C, 0x9D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D);

/**
 * @brief IIOSCSIDevice interface GUID
 * {BACBD8E7-6F5G-4B3C-9DAE-2F3G4B5C6D7E}
 */
DEFINE_GUID(IID_IIOSCSIDevice,
    0xBACBD8E7, 0x6F5G, 0x4B3C, 0x9D, 0xAE, 0x2F, 0x3G, 0x4B, 0x5C, 0x6D, 0x7E);

/**
 * @brief SCSI/SAS Protocol Versions
 */
typedef enum _SCSI_PROTOCOL {
    SCSI_PROTOCOL_SCSI1     = 0x01,     /**< SCSI-1 */
    SCSI_PROTOCOL_SCSI2     = 0x02,     /**< SCSI-2 */
    SCSI_PROTOCOL_SCSI3     = 0x03,     /**< SCSI-3 (SPC) */
    SCSI_PROTOCOL_SPC2      = 0x04,     /**< SCSI Primary Commands-2 */
    SCSI_PROTOCOL_SPC3      = 0x05,     /**< SCSI Primary Commands-3 */
    SCSI_PROTOCOL_SPC4      = 0x06,     /**< SCSI Primary Commands-4 */
    SCSI_PROTOCOL_SPC5      = 0x07,     /**< SCSI Primary Commands-5 */
} SCSI_PROTOCOL;

/**
 * @brief SAS Link Speeds
 */
typedef enum _SAS_SPEED {
    SAS_SPEED_1_5_GBPS      = 0,        /**< SAS-1: 1.5 Gbps */
    SAS_SPEED_3_0_GBPS      = 1,        /**< SAS-1: 3.0 Gbps */
    SAS_SPEED_6_0_GBPS      = 2,        /**< SAS-2: 6.0 Gbps */
    SAS_SPEED_12_0_GBPS     = 3,        /**< SAS-3: 12.0 Gbps */
    SAS_SPEED_22_5_GBPS     = 4,        /**< SAS-4: 22.5 Gbps */
} SAS_SPEED;

/**
 * @brief Fibre Channel Link Speeds
 */
typedef enum _FC_SPEED {
    FC_SPEED_1_GBPS         = 0,        /**< FC-1: 1 Gbps */
    FC_SPEED_2_GBPS         = 1,        /**< FC-2: 2 Gbps */
    FC_SPEED_4_GBPS         = 2,        /**< FC-4: 4 Gbps */
    FC_SPEED_8_GBPS         = 3,        /**< FC-8: 8 Gbps */
    FC_SPEED_16_GBPS        = 4,        /**< FC-16: 16 Gbps */
    FC_SPEED_32_GBPS        = 5,        /**< FC-32: 32 Gbps */
    FC_SPEED_64_GBPS        = 6,        /**< FC-64: 64 Gbps */
    FC_SPEED_128_GBPS       = 7,        /**< FC-128: 128 Gbps */
} FC_SPEED;

/**
 * @brief Fibre Channel Topology
 */
typedef enum _FC_TOPOLOGY {
    FC_TOPOLOGY_UNKNOWN     = 0,        /**< Unknown topology */
    FC_TOPOLOGY_P2P         = 1,        /**< Point-to-Point */
    FC_TOPOLOGY_FABRIC      = 2,        /**< Fabric (switched) */
    FC_TOPOLOGY_LOOP        = 3,        /**< Arbitrated Loop (FC-AL) */
} FC_TOPOLOGY;

/**
 * @brief SCSI Device Types
 */
typedef enum _SCSI_DEVICE_TYPE {
    SCSI_DEVICE_DIRECT_ACCESS   = 0x00, /**< Direct-access device (disk) */
    SCSI_DEVICE_SEQUENTIAL      = 0x01, /**< Sequential-access device (tape) */
    SCSI_DEVICE_PRINTER         = 0x02, /**< Printer device */
    SCSI_DEVICE_PROCESSOR       = 0x03, /**< Processor device */
    SCSI_DEVICE_WRITE_ONCE      = 0x04, /**< Write-once device */
    SCSI_DEVICE_CD_DVD          = 0x05, /**< CD-ROM/DVD device */
    SCSI_DEVICE_SCANNER         = 0x06, /**< Scanner device */
    SCSI_DEVICE_OPTICAL         = 0x07, /**< Optical memory device */
    SCSI_DEVICE_MEDIA_CHANGER   = 0x08, /**< Medium changer device */
    SCSI_DEVICE_COMMUNICATIONS  = 0x09, /**< Communications device */
    SCSI_DEVICE_RAID            = 0x0C, /**< Storage array controller */
    SCSI_DEVICE_ENCLOSURE       = 0x0D, /**< Enclosure services device */
    SCSI_DEVICE_RBC             = 0x0E, /**< Simplified direct-access */
    SCSI_DEVICE_OPTICAL_CARD    = 0x0F, /**< Optical card reader/writer */
    SCSI_DEVICE_BRIDGE          = 0x10, /**< Bridge controller */
    SCSI_DEVICE_OSD             = 0x11, /**< Object-based storage */
    SCSI_DEVICE_AUTOMATION      = 0x12, /**< Automation/Drive interface */
    SCSI_DEVICE_WELL_KNOWN_LU   = 0x1E, /**< Well known logical unit */
    SCSI_DEVICE_UNKNOWN         = 0x1F, /**< Unknown or no device */
} SCSI_DEVICE_TYPE;

/**
 * @brief SCSI Commands (partial list)
 */
#define SCSI_CMD_TEST_UNIT_READY        0x00    /**< Test Unit Ready */
#define SCSI_CMD_REQUEST_SENSE          0x03    /**< Request Sense */
#define SCSI_CMD_FORMAT_UNIT            0x04    /**< Format Unit */
#define SCSI_CMD_READ6                  0x08    /**< Read (6) */
#define SCSI_CMD_WRITE6                 0x0A    /**< Write (6) */
#define SCSI_CMD_INQUIRY                0x12    /**< Inquiry */
#define SCSI_CMD_MODE_SELECT6           0x15    /**< Mode Select (6) */
#define SCSI_CMD_MODE_SENSE6            0x1A    /**< Mode Sense (6) */
#define SCSI_CMD_START_STOP_UNIT        0x1B    /**< Start/Stop Unit */
#define SCSI_CMD_SEND_DIAGNOSTIC        0x1D    /**< Send Diagnostic */
#define SCSI_CMD_READ_CAPACITY10        0x25    /**< Read Capacity (10) */
#define SCSI_CMD_READ10                 0x28    /**< Read (10) */
#define SCSI_CMD_WRITE10                0x2A    /**< Write (10) */
#define SCSI_CMD_VERIFY10               0x2F    /**< Verify (10) */
#define SCSI_CMD_SYNCHRONIZE_CACHE      0x35    /**< Synchronize Cache */
#define SCSI_CMD_READ_TOC               0x43    /**< Read TOC/PMA/ATIP */
#define SCSI_CMD_LOG_SELECT             0x4C    /**< Log Select */
#define SCSI_CMD_LOG_SENSE              0x4D    /**< Log Sense */
#define SCSI_CMD_MODE_SELECT10          0x55    /**< Mode Select (10) */
#define SCSI_CMD_MODE_SENSE10           0x5A    /**< Mode Sense (10) */
#define SCSI_CMD_READ16                 0x88    /**< Read (16) */
#define SCSI_CMD_WRITE16                0x8A    /**< Write (16) */
#define SCSI_CMD_VERIFY16               0x8F    /**< Verify (16) */
#define SCSI_CMD_SERVICE_ACTION_IN16    0x9E    /**< Service Action In (16) */
#define SCSI_CMD_REPORT_LUNS            0xA0    /**< Report LUNs */
#define SCSI_CMD_READ12                 0xA8    /**< Read (12) */
#define SCSI_CMD_WRITE12                0xAA    /**< Write (12) */

/**
 * @brief SCSI Status Codes
 */
#define SCSI_STATUS_GOOD                0x00    /**< Good */
#define SCSI_STATUS_CHECK_CONDITION     0x02    /**< Check Condition */
#define SCSI_STATUS_CONDITION_MET       0x04    /**< Condition Met */
#define SCSI_STATUS_BUSY                0x08    /**< Busy */
#define SCSI_STATUS_INTERMEDIATE        0x10    /**< Intermediate */
#define SCSI_STATUS_RESERVATION_CONFLICT 0x18   /**< Reservation Conflict */
#define SCSI_STATUS_TASK_SET_FULL       0x28    /**< Task Set Full */
#define SCSI_STATUS_ACA_ACTIVE          0x30    /**< ACA Active */
#define SCSI_STATUS_TASK_ABORTED        0x40    /**< Task Aborted */

/**
 * @brief SCSI Sense Keys
 */
#define SCSI_SENSE_NO_SENSE             0x00    /**< No Sense */
#define SCSI_SENSE_RECOVERED_ERROR      0x01    /**< Recovered Error */
#define SCSI_SENSE_NOT_READY            0x02    /**< Not Ready */
#define SCSI_SENSE_MEDIUM_ERROR         0x03    /**< Medium Error */
#define SCSI_SENSE_HARDWARE_ERROR       0x04    /**< Hardware Error */
#define SCSI_SENSE_ILLEGAL_REQUEST      0x05    /**< Illegal Request */
#define SCSI_SENSE_UNIT_ATTENTION       0x06    /**< Unit Attention */
#define SCSI_SENSE_DATA_PROTECT         0x07    /**< Data Protect */
#define SCSI_SENSE_BLANK_CHECK          0x08    /**< Blank Check */
#define SCSI_SENSE_VENDOR_SPECIFIC      0x09    /**< Vendor Specific */
#define SCSI_SENSE_COPY_ABORTED         0x0A    /**< Copy Aborted */
#define SCSI_SENSE_ABORTED_COMMAND      0x0B    /**< Aborted Command */
#define SCSI_SENSE_VOLUME_OVERFLOW      0x0D    /**< Volume Overflow */
#define SCSI_SENSE_MISCOMPARE           0x0E    /**< Miscompare */

/**
 * @brief SCSI Command Descriptor Block (CDB) - maximum size
 */
#define SCSI_MAX_CDB_SIZE               16

/**
 * @brief SCSI Inquiry Data
 */
typedef struct _SCSI_INQUIRY_DATA {
    UINT8   DeviceType;             /**< Peripheral device type */
    UINT8   RMB;                    /**< Removable media bit */
    UINT8   Version;                /**< SCSI version */
    UINT8   ResponseDataFormat;     /**< Response data format */
    UINT8   AdditionalLength;       /**< Additional length */
    UINT8   Flags[3];               /**< Flags */
    CHAR8   VendorID[8];            /**< Vendor identification */
    CHAR8   ProductID[16];          /**< Product identification */
    CHAR8   ProductRevision[4];     /**< Product revision level */
} SCSI_INQUIRY_DATA;

/**
 * @brief SCSI Sense Data
 */
typedef struct _SCSI_SENSE_DATA {
    UINT8   ResponseCode;           /**< Response code */
    UINT8   SegmentNumber;          /**< Segment number */
    UINT8   SenseKey;               /**< Sense key */
    UINT8   Information[4];         /**< Information */
    UINT8   AdditionalSenseLength;  /**< Additional sense length */
    UINT8   CommandSpecific[4];     /**< Command-specific information */
    UINT8   ASC;                    /**< Additional sense code */
    UINT8   ASCQ;                   /**< Additional sense code qualifier */
    UINT8   FRUC;                   /**< Field replaceable unit code */
    UINT8   SenseKeySpecific[3];    /**< Sense key specific */
} SCSI_SENSE_DATA;

/**
 * @brief SCSI Controller Information
 */
typedef struct _SCSI_CONTROLLER_INFO {
    SCSI_PROTOCOL   Protocol;           /**< SCSI protocol version */
    SAS_SPEED       MaxSpeed;           /**< Maximum link speed (SAS) */
    FC_SPEED        FCMaxSpeed;         /**< Maximum link speed (FC) */
    FC_TOPOLOGY     FCTopology;         /**< Fibre Channel topology */
    UINT16          VendorID;           /**< PCI Vendor ID */
    UINT16          DeviceID;           /**< PCI Device ID */
    UINT32          MaxTargets;         /**< Maximum target IDs */
    UINT32          MaxLUNs;            /**< Maximum LUNs per target */
    UINT32          MaxTransferSize;    /**< Maximum transfer size */
    UINT32          MaxQueueDepth;      /**< Maximum command queue depth */
    BOOLEAN         bWideSupport;       /**< Wide SCSI support (16-bit) */
    BOOLEAN         bTaggedQueuing;     /**< Tagged command queuing */
    BOOLEAN         bSASSupport;        /**< SAS support */
    BOOLEAN         bFCSupport;         /**< Fibre Channel support */
    BOOLEAN         bFCoESupport;       /**< FCoE (FC over Ethernet) support */
    BOOLEAN         bHotplug;           /**< Hot-plug support */
    BOOLEAN         bExpander;          /**< SAS expander support */
    UINT64          WWN;                /**< World Wide Name (FC) */
    UINT64          WWPN;               /**< World Wide Port Name (FC) */
} SCSI_CONTROLLER_INFO;

/**
 * @brief SCSI Device Information
 */
typedef struct _SCSI_DEVICE_INFO {
    UINT32              TargetID;           /**< Target ID */
    UINT32              LUN;                /**< Logical Unit Number */
    SCSI_DEVICE_TYPE    DeviceType;         /**< Device type */
    CHAR8               VendorID[9];        /**< Vendor ID (null-terminated) */
    CHAR8               ProductID[17];      /**< Product ID (null-terminated) */
    CHAR8               Revision[5];        /**< Revision (null-terminated) */
    UINT64              TotalBlocks;        /**< Total blocks (if applicable) */
    UINT32              BlockSize;          /**< Block size in bytes */
    BOOLEAN             bRemovable;         /**< Removable media */
    BOOLEAN             bWriteProtected;    /**< Write protected */
    UINT8               SCSIVersion;        /**< SCSI version */
    UINT32              QueueDepth;         /**< Command queue depth */
} SCSI_DEVICE_INFO;

/**
 * @brief SCSI Command Structure
 */
typedef struct _SCSI_COMMAND {
    UINT8       CDB[SCSI_MAX_CDB_SIZE]; /**< Command Descriptor Block */
    UINT8       CDBLength;              /**< CDB length in bytes */
    VOID       *pDataBuffer;            /**< Data buffer */
    UINT32      DataLength;             /**< Data length in bytes */
    BOOLEAN     bDataIn;                /**< TRUE for read, FALSE for write */
    UINT32      TimeoutMs;              /**< Timeout in milliseconds */
    UINT8       Status;                 /**< Command status */
    SCSI_SENSE_DATA SenseData;          /**< Sense data (if error) */
} SCSI_COMMAND;

/**
 * @brief Known SCSI/SAS Controller Vendors
 */
#define SCSI_VENDOR_LSI             0x1000  /**< LSI Logic / Broadcom */
#define SCSI_VENDOR_ADAPTEC         0x9004  /**< Adaptec */
#define SCSI_VENDOR_QLOGIC          0x1077  /**< QLogic */
#define SCSI_VENDOR_ARECA           0x17D3  /**< Areca */
#define SCSI_VENDOR_HIGHPOINT       0x1103  /**< HighPoint */
#define SCSI_VENDOR_PROMISE         0x105A  /**< Promise */
#define SCSI_VENDOR_MYLEX           0x1069  /**< Mylex */
#define SCSI_VENDOR_HP              0x103C  /**< HP */
#define SCSI_VENDOR_DELL            0x1028  /**< Dell */
#define SCSI_VENDOR_IBM             0x1014  /**< IBM */

/**
 * @brief IIOSCSIController - SCSI Controller interface
 *
 * This interface represents a SCSI/SAS host adapter and provides methods
 * for device enumeration, command submission, and bus management.
 */
#undef INTERFACE
#define INTERFACE IIOSCSIController

DECLARE_INTERFACE_(IIOSCSIController, IIOService)
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

    // IIOSCSIController methods

    /**
     * @brief Get controller information
     *
     * Retrieves comprehensive controller information including protocol
     * version, capabilities, and limits.
     *
     * @param pControllerInfo   Receives controller information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetControllerInfo)(THIS_
        SCSI_CONTROLLER_INFO *pControllerInfo
        ) PURE;

    /**
     * @brief Get device count
     *
     * Returns the number of discovered SCSI devices.
     *
     * @param puCount           Receives device count
     *
     * @retval IO_SUCCESS       Count retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetDeviceCount)(THIS_
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Get device interface
     *
     * Retrieves the device interface for a specific target/LUN.
     *
     * @param uTarget           Target ID
     * @param uLUN              Logical Unit Number
     * @param ppDevice          Receives device interface
     *
     * @retval IO_SUCCESS       Device interface retrieved
     * @retval IO_NO_DEVICE     Device not found
     */
    STDMETHOD_(IO_RETURN, GetDevice)(THIS_
        UINT32 uTarget,
        UINT32 uLUN,
        IIOSCSIDevice **ppDevice
        ) PURE;

    /**
     * @brief Reset SCSI bus
     *
     * Performs a SCSI bus reset operation.
     *
     * @retval IO_SUCCESS       Bus reset successful
     * @retval IO_ERROR         Reset failed
     */
    STDMETHOD_(IO_RETURN, ResetBus)(THIS) PURE;

    /**
     * @brief Reset target
     *
     * Performs a target reset operation.
     *
     * @param uTarget           Target ID
     *
     * @retval IO_SUCCESS       Target reset successful
     * @retval IO_ERROR         Reset failed
     */
    STDMETHOD_(IO_RETURN, ResetTarget)(THIS_
        UINT32 uTarget
        ) PURE;

    /**
     * @brief Scan bus for devices
     *
     * Scans the SCSI bus and enumerates all devices.
     *
     * @retval IO_SUCCESS       Scan complete
     */
    STDMETHOD_(IO_RETURN, ScanBus)(THIS) PURE;

    /**
     * @brief Submit SCSI command
     *
     * Submits a raw SCSI command to a target device.
     *
     * @param uTarget           Target ID
     * @param uLUN              Logical Unit Number
     * @param pCommand          SCSI command structure
     *
     * @retval IO_SUCCESS       Command completed successfully
     * @retval IO_ERROR         Command failed
     */
    STDMETHOD_(IO_RETURN, SubmitCommand)(THIS_
        UINT32 uTarget,
        UINT32 uLUN,
        SCSI_COMMAND *pCommand
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOSCSIDevice - SCSI Device interface
 *
 * This interface represents a SCSI logical unit and provides methods
 * for standard SCSI operations.
 */
#undef INTERFACE
#define INTERFACE IIOSCSIDevice

DECLARE_INTERFACE_(IIOSCSIDevice, IIOService)
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
        SCSI_DEVICE_INFO *pDeviceInfo
        ) PURE;

    /**
     * @brief Read blocks
     *
     * Reads blocks from the device (for direct-access devices).
     *
     * @param uLBA              Starting logical block address
     * @param uBlocks           Number of blocks to read
     * @param pBuffer           Buffer to receive data
     * @param cbBuffer          Buffer size in bytes
     *
     * @retval IO_SUCCESS       Read successful
     * @retval IO_ERROR         Read failed
     */
    STDMETHOD_(IO_RETURN, ReadBlocks)(THIS_
        UINT64 uLBA,
        UINT32 uBlocks,
        VOID *pBuffer,
        UINTN cbBuffer
        ) PURE;

    /**
     * @brief Write blocks
     *
     * Writes blocks to the device (for direct-access devices).
     *
     * @param uLBA              Starting logical block address
     * @param uBlocks           Number of blocks to write
     * @param pBuffer           Buffer containing data
     * @param cbBuffer          Buffer size in bytes
     *
     * @retval IO_SUCCESS       Write successful
     * @retval IO_ERROR         Write failed
     */
    STDMETHOD_(IO_RETURN, WriteBlocks)(THIS_
        UINT64 uLBA,
        UINT32 uBlocks,
        CONST VOID *pBuffer,
        UINTN cbBuffer
        ) PURE;

    /**
     * @brief Execute SCSI command
     *
     * Executes a SCSI command on this device.
     *
     * @param pCommand          SCSI command structure
     *
     * @retval IO_SUCCESS       Command successful
     * @retval IO_ERROR         Command failed
     */
    STDMETHOD_(IO_RETURN, ExecuteCommand)(THIS_
        SCSI_COMMAND *pCommand
        ) PURE;

    /**
     * @brief Request sense data
     *
     * Retrieves sense data from the device.
     *
     * @param pSenseData        Receives sense data
     *
     * @retval IO_SUCCESS       Sense data retrieved
     * @retval IO_ERROR         Command failed
     */
    STDMETHOD_(IO_RETURN, RequestSense)(THIS_
        SCSI_SENSE_DATA *pSenseData
        ) PURE;

    /**
     * @brief Test unit ready
     *
     * Tests if the device is ready for operation.
     *
     * @retval IO_SUCCESS       Device ready
     * @retval IO_NOT_READY     Device not ready
     */
    STDMETHOD_(IO_RETURN, TestUnitReady)(THIS) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOSCSIController methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOSCSIController_GetControllerInfo(p,a)    (p)->lpVtbl->GetControllerInfo(p,a)
#define IIOSCSIController_GetDeviceCount(p,a)       (p)->lpVtbl->GetDeviceCount(p,a)
#define IIOSCSIController_GetDevice(p,a,b,c)        (p)->lpVtbl->GetDevice(p,a,b,c)
#define IIOSCSIController_ResetBus(p)               (p)->lpVtbl->ResetBus(p)
#define IIOSCSIController_ResetTarget(p,a)          (p)->lpVtbl->ResetTarget(p,a)
#define IIOSCSIController_ScanBus(p)                (p)->lpVtbl->ScanBus(p)
#define IIOSCSIController_SubmitCommand(p,a,b,c)    (p)->lpVtbl->SubmitCommand(p,a,b,c)

#define IIOSCSIDevice_GetDeviceInfo(p,a)            (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOSCSIDevice_ReadBlocks(p,a,b,c,d)         (p)->lpVtbl->ReadBlocks(p,a,b,c,d)
#define IIOSCSIDevice_WriteBlocks(p,a,b,c,d)        (p)->lpVtbl->WriteBlocks(p,a,b,c,d)
#define IIOSCSIDevice_ExecuteCommand(p,a)           (p)->lpVtbl->ExecuteCommand(p,a)
#define IIOSCSIDevice_RequestSense(p,a)             (p)->lpVtbl->RequestSense(p,a)
#define IIOSCSIDevice_TestUnitReady(p)              (p)->lpVtbl->TestUnitReady(p)

#endif

/**
 * @brief Initialize SCSI/SAS family driver
 *
 * Initializes the SCSI/SAS family driver and registers controller classes.
 *
 * @retval IO_SUCCESS   Initialization successful
 * @retval IO_ERROR     Initialization failed
 */
IO_RETURN
SCSIInitialize(
    VOID
    );

/**
 * @brief Shutdown SCSI/SAS family driver
 *
 * Shuts down the SCSI family driver and releases resources.
 *
 * @retval IO_SUCCESS   Shutdown successful
 */
IO_RETURN
SCSIShutdown(
    VOID
    );

/**
 * @brief Create SCSI controller instance
 *
 * Creates a SCSI controller interface for a PCI device.
 *
 * @param pPCIDevice        PCI device interface
 * @param ppController      Receives controller interface
 *
 * @retval IO_SUCCESS       Controller created successfully
 * @retval IO_NO_MEMORY     Insufficient memory
 * @retval IO_NO_DEVICE     Not a SCSI controller
 */
IO_RETURN
SCSIControllerCreate(
    IIOService *pPCIDevice,
    IIOSCSIController **ppController
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_SCSI_H */
