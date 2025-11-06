/**
 * @file nvme.h
 * @brief NVMe Family Interface - NVM Express Storage Driver
 *
 * This header defines the NVMe family interface for NVM Express storage devices,
 * supporting NVMe 1.0/1.1/1.2/1.3/1.4/2.0 specifications with admin and I/O
 * command queues, namespace management, and PCIe attachment.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_NVME_H
#define IOKIT_NVME_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIONVMeController interface GUID
 * {C5D8E9F3-4A7B-4E6C-9D8A-1F2E3D4C5B6A}
 */
DEFINE_GUID(IID_IIONVMeController,
    0xC5D8E9F3, 0x4A7B, 0x4E6C, 0x9D, 0x8A, 0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A);

/**
 * @brief IIONVMeNamespace interface GUID
 * {D6E9FA04-5B8C-4F7D-8E9B-2F3E4D5C6B7A}
 */
DEFINE_GUID(IID_IIONVMeNamespace,
    0xD6E9FA04, 0x5B8C, 0x4F7D, 0x8E, 0x9B, 0x2F, 0x3E, 0x4D, 0x5C, 0x6B, 0x7A);

/**
 * @brief NVMe Specification Versions
 */
typedef enum _NVME_VERSION {
    NVME_VERSION_1_0    = 0x00010000,   /**< NVMe 1.0 */
    NVME_VERSION_1_1    = 0x00010100,   /**< NVMe 1.1 */
    NVME_VERSION_1_2    = 0x00010200,   /**< NVMe 1.2 */
    NVME_VERSION_1_2_1  = 0x00010201,   /**< NVMe 1.2.1 */
    NVME_VERSION_1_3    = 0x00010300,   /**< NVMe 1.3 */
    NVME_VERSION_1_4    = 0x00010400,   /**< NVMe 1.4 */
    NVME_VERSION_2_0    = 0x00020000,   /**< NVMe 2.0 */
} NVME_VERSION;

/**
 * @brief NVMe Controller Capabilities
 */
#define NVME_CAP_MQES_SHIFT         0       /**< Maximum Queue Entries Supported */
#define NVME_CAP_MQES_MASK          0xFFFF
#define NVME_CAP_CQR_SHIFT          16      /**< Contiguous Queues Required */
#define NVME_CAP_CQR_MASK           0x1
#define NVME_CAP_AMS_SHIFT          17      /**< Arbitration Mechanism Supported */
#define NVME_CAP_AMS_MASK           0x3
#define NVME_CAP_TO_SHIFT           24      /**< Timeout */
#define NVME_CAP_TO_MASK            0xFF
#define NVME_CAP_DSTRD_SHIFT        32      /**< Doorbell Stride */
#define NVME_CAP_DSTRD_MASK         0xF
#define NVME_CAP_NSSRS_SHIFT        36      /**< NVM Subsystem Reset Supported */
#define NVME_CAP_CSS_SHIFT          37      /**< Command Sets Supported */
#define NVME_CAP_CSS_MASK           0xFF
#define NVME_CAP_BPS_SHIFT          45      /**< Boot Partition Support */
#define NVME_CAP_MPSMIN_SHIFT       48      /**< Memory Page Size Minimum */
#define NVME_CAP_MPSMIN_MASK        0xF
#define NVME_CAP_MPSMAX_SHIFT       52      /**< Memory Page Size Maximum */
#define NVME_CAP_MPSMAX_MASK        0xF

/**
 * @brief NVMe Controller Configuration
 */
#define NVME_CC_EN                  (1 << 0)    /**< Enable */
#define NVME_CC_CSS_SHIFT           4           /**< I/O Command Set Selected */
#define NVME_CC_CSS_MASK            0x7
#define NVME_CC_MPS_SHIFT           7           /**< Memory Page Size */
#define NVME_CC_MPS_MASK            0xF
#define NVME_CC_AMS_SHIFT           11          /**< Arbitration Mechanism Selected */
#define NVME_CC_AMS_MASK            0x7
#define NVME_CC_SHN_SHIFT           14          /**< Shutdown Notification */
#define NVME_CC_SHN_MASK            0x3
#define NVME_CC_IOSQES_SHIFT        16          /**< I/O Submission Queue Entry Size */
#define NVME_CC_IOSQES_MASK         0xF
#define NVME_CC_IOCQES_SHIFT        20          /**< I/O Completion Queue Entry Size */
#define NVME_CC_IOCQES_MASK         0xF

/**
 * @brief NVMe Controller Status
 */
#define NVME_CSTS_RDY               (1 << 0)    /**< Ready */
#define NVME_CSTS_CFS               (1 << 1)    /**< Controller Fatal Status */
#define NVME_CSTS_SHST_SHIFT        2           /**< Shutdown Status */
#define NVME_CSTS_SHST_MASK         0x3
#define NVME_CSTS_NSSRO             (1 << 4)    /**< NVM Subsystem Reset Occurred */
#define NVME_CSTS_PP                (1 << 5)    /**< Processing Paused */

/**
 * @brief NVMe Admin Commands
 */
typedef enum _NVME_ADMIN_OPCODE {
    NVME_ADMIN_DELETE_SQ        = 0x00,     /**< Delete I/O Submission Queue */
    NVME_ADMIN_CREATE_SQ        = 0x01,     /**< Create I/O Submission Queue */
    NVME_ADMIN_GET_LOG_PAGE     = 0x02,     /**< Get Log Page */
    NVME_ADMIN_DELETE_CQ        = 0x04,     /**< Delete I/O Completion Queue */
    NVME_ADMIN_CREATE_CQ        = 0x05,     /**< Create I/O Completion Queue */
    NVME_ADMIN_IDENTIFY         = 0x06,     /**< Identify */
    NVME_ADMIN_ABORT_CMD        = 0x08,     /**< Abort */
    NVME_ADMIN_SET_FEATURES     = 0x09,     /**< Set Features */
    NVME_ADMIN_GET_FEATURES     = 0x0A,     /**< Get Features */
    NVME_ADMIN_ASYNC_EVENT      = 0x0C,     /**< Asynchronous Event Request */
    NVME_ADMIN_NS_MGMT          = 0x0D,     /**< Namespace Management */
    NVME_ADMIN_FW_COMMIT        = 0x10,     /**< Firmware Commit */
    NVME_ADMIN_FW_DOWNLOAD      = 0x11,     /**< Firmware Image Download */
    NVME_ADMIN_NS_ATTACH        = 0x15,     /**< Namespace Attachment */
    NVME_ADMIN_KEEP_ALIVE       = 0x18,     /**< Keep Alive */
    NVME_ADMIN_FORMAT_NVM       = 0x80,     /**< Format NVM */
    NVME_ADMIN_SECURITY_SEND    = 0x81,     /**< Security Send */
    NVME_ADMIN_SECURITY_RECV    = 0x82,     /**< Security Receive */
} NVME_ADMIN_OPCODE;

/**
 * @brief NVMe I/O Commands
 */
typedef enum _NVME_IO_OPCODE {
    NVME_CMD_FLUSH              = 0x00,     /**< Flush */
    NVME_CMD_WRITE              = 0x01,     /**< Write */
    NVME_CMD_READ               = 0x02,     /**< Read */
    NVME_CMD_WRITE_UNCOR        = 0x04,     /**< Write Uncorrectable */
    NVME_CMD_COMPARE            = 0x05,     /**< Compare */
    NVME_CMD_WRITE_ZEROES       = 0x08,     /**< Write Zeroes */
    NVME_CMD_DSM                = 0x09,     /**< Dataset Management */
    NVME_CMD_RESV_REGISTER      = 0x0D,     /**< Reservation Register */
    NVME_CMD_RESV_REPORT        = 0x0E,     /**< Reservation Report */
    NVME_CMD_RESV_ACQUIRE       = 0x11,     /**< Reservation Acquire */
    NVME_CMD_RESV_RELEASE       = 0x15,     /**< Reservation Release */
} NVME_IO_OPCODE;

/**
 * @brief NVMe Queue Entry Sizes
 */
#define NVME_SQ_ENTRY_SIZE          64      /**< Submission Queue Entry Size */
#define NVME_CQ_ENTRY_SIZE          16      /**< Completion Queue Entry Size */
#define NVME_MAX_QUEUE_ENTRIES      4096    /**< Maximum Queue Entries */

/**
 * @brief NVMe Controller Register Offsets
 */
#define NVME_REG_CAP                0x00    /**< Controller Capabilities (64-bit) */
#define NVME_REG_VS                 0x08    /**< Version (32-bit) */
#define NVME_REG_INTMS              0x0C    /**< Interrupt Mask Set (32-bit) */
#define NVME_REG_INTMC              0x10    /**< Interrupt Mask Clear (32-bit) */
#define NVME_REG_CC                 0x14    /**< Controller Configuration (32-bit) */
#define NVME_REG_CSTS               0x1C    /**< Controller Status (32-bit) */
#define NVME_REG_NSSR               0x20    /**< NVM Subsystem Reset (32-bit) */
#define NVME_REG_AQA                0x24    /**< Admin Queue Attributes (32-bit) */
#define NVME_REG_ASQ                0x28    /**< Admin Submission Queue (64-bit) */
#define NVME_REG_ACQ                0x30    /**< Admin Completion Queue (64-bit) */

/**
 * @brief NVMe Controller Identification
 */
typedef struct _NVME_CONTROLLER_ID {
    UINT16  VendorID;               /**< PCI Vendor ID */
    UINT16  SubsysVendorID;         /**< PCI Subsystem Vendor ID */
    CHAR8   SerialNumber[20];       /**< Serial Number */
    CHAR8   ModelNumber[40];        /**< Model Number */
    CHAR8   FirmwareRevision[8];    /**< Firmware Revision */
    UINT8   RAB;                    /**< Recommended Arbitration Burst */
    UINT8   IEEE[3];                /**< IEEE OUI Identifier */
    UINT8   CMIC;                   /**< Controller Multi-Path I/O */
    UINT8   MDTS;                   /**< Maximum Data Transfer Size */
    UINT16  ControllerID;           /**< Controller ID */
    UINT32  Version;                /**< NVMe Version */
    UINT32  RTD3R;                  /**< RTD3 Resume Latency */
    UINT32  RTD3E;                  /**< RTD3 Entry Latency */
    UINT32  OAES;                   /**< Optional Async Events Supported */
    UINT32  CTRATT;                 /**< Controller Attributes */
} NVME_CONTROLLER_ID;

/**
 * @brief NVMe Namespace Information
 */
typedef struct _NVME_NAMESPACE_INFO {
    UINT32  NamespaceID;            /**< Namespace Identifier */
    UINT64  Size;                   /**< Total Size (in LBAs) */
    UINT64  Capacity;               /**< Capacity (in LBAs) */
    UINT64  Utilization;            /**< Utilization (in LBAs) */
    UINT8   Features;               /**< Namespace Features */
    UINT8   NumLBAFormats;          /**< Number of LBA Formats */
    UINT8   FormatIndex;            /**< Formatted LBA Size Index */
    UINT8   DataProtSettings;       /**< Data Protection Settings */
    UINT32  LBASize;                /**< LBA Size in bytes */
    BOOLEAN bWriteProtected;        /**< Write Protected */
    BOOLEAN bShared;                /**< Shared Namespace */
} NVME_NAMESPACE_INFO;

/**
 * @brief NVMe Controller Information
 */
typedef struct _NVME_CONTROLLER_INFO {
    NVME_VERSION        Version;            /**< NVMe version */
    UINT16              VendorID;           /**< PCI Vendor ID */
    UINT16              DeviceID;           /**< PCI Device ID */
    CHAR8               SerialNumber[21];   /**< Serial Number (null-terminated) */
    CHAR8               ModelNumber[41];    /**< Model Number (null-terminated) */
    CHAR8               FirmwareRevision[9];/**< Firmware Revision (null-terminated) */
    UINT32              MaxTransferSize;    /**< Maximum Transfer Size */
    UINT32              MaxQueueEntries;    /**< Maximum Queue Entries */
    UINT32              NumQueues;          /**< Number of I/O Queues */
    UINT32              NumNamespaces;      /**< Number of Namespaces */
    BOOLEAN             bVolatileWriteCache;/**< Volatile Write Cache Present */
    BOOLEAN             bMultiPathIO;       /**< Multi-Path I/O Support */
    BOOLEAN             bSecuritySupport;   /**< Security Send/Receive Support */
    BOOLEAN             bFormatSupport;     /**< Format NVM Support */
} NVME_CONTROLLER_INFO;

/**
 * @brief NVMe Command Completion Status
 */
typedef struct _NVME_COMPLETION_STATUS {
    UINT16  StatusField;            /**< Status Field */
    UINT16  CommandID;              /**< Command Identifier */
    UINT32  DW0;                    /**< DW0 (command specific) */
} NVME_COMPLETION_STATUS;

/**
 * @brief Known NVMe Controller Vendors
 */
#define NVME_VENDOR_INTEL           0x8086
#define NVME_VENDOR_SAMSUNG         0x144D
#define NVME_VENDOR_SANDISK         0x15B7
#define NVME_VENDOR_WDC             0x1B96
#define NVME_VENDOR_MICRON          0x1344
#define NVME_VENDOR_SK_HYNIX        0x1C5C
#define NVME_VENDOR_KINGSTON        0x2646
#define NVME_VENDOR_ADATA           0x1CC1
#define NVME_VENDOR_CRUCIAL         0x1344
#define NVME_VENDOR_SEAGATE         0x1BB1
#define NVME_VENDOR_KIOXIA          0x1E0F
#define NVME_VENDOR_PHISON          0x1987
#define NVME_VENDOR_SILICON_MOTION  0x126F
#define NVME_VENDOR_REALTEK         0x10EC
#define NVME_VENDOR_CORSAIR         0x1987
#define NVME_VENDOR_PLEXTOR         0x1989
#define NVME_VENDOR_TRANSCEND       0x1E49
#define NVME_VENDOR_LEXAR           0x1CB4
#define NVME_VENDOR_MUSHKIN         0x1987
#define NVME_VENDOR_OCZ             0x1B85

/**
 * @brief IIONVMeController - NVMe Controller interface
 *
 * This interface represents an NVMe controller and provides methods for
 * admin commands, queue management, and namespace enumeration.
 */
#undef INTERFACE
#define INTERFACE IIONVMeController

DECLARE_INTERFACE_(IIONVMeController, IIOService)
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

    // IIONVMeController methods

    /**
     * @brief Get controller information
     *
     * Retrieves comprehensive controller information including version,
     * capabilities, and identification data.
     *
     * @param pControllerInfo   Receives controller information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetControllerInfo)(THIS_
        NVME_CONTROLLER_INFO *pControllerInfo
        ) PURE;

    /**
     * @brief Identify controller
     *
     * Executes the Identify Controller admin command.
     *
     * @param pControllerID     Receives controller identification data
     *
     * @retval IO_SUCCESS       Command successful
     * @retval IO_ERROR         Command failed
     */
    STDMETHOD_(IO_RETURN, IdentifyController)(THIS_
        NVME_CONTROLLER_ID *pControllerID
        ) PURE;

    /**
     * @brief Get namespace count
     *
     * Returns the number of active namespaces.
     *
     * @param puCount           Receives namespace count
     *
     * @retval IO_SUCCESS       Count retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetNamespaceCount)(THIS_
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Get namespace interface
     *
     * Retrieves an interface to a specific namespace.
     *
     * @param uNamespaceID      Namespace identifier (1-based)
     * @param ppNamespace       Receives namespace interface
     *
     * @retval IO_SUCCESS       Namespace interface retrieved
     * @retval IO_NO_DEVICE     Namespace not found
     */
    STDMETHOD_(IO_RETURN, GetNamespace)(THIS_
        UINT32 uNamespaceID,
        IIONVMeNamespace **ppNamespace
        ) PURE;

    /**
     * @brief Create I/O queue pair
     *
     * Creates submission and completion queue pair for I/O operations.
     *
     * @param uQueueSize        Queue size (entries)
     * @param puQueueID         Receives queue identifier
     *
     * @retval IO_SUCCESS       Queue pair created
     * @retval IO_NO_RESOURCES  Insufficient resources
     */
    STDMETHOD_(IO_RETURN, CreateIOQueue)(THIS_
        UINT32 uQueueSize,
        UINT32 *puQueueID
        ) PURE;

    /**
     * @brief Delete I/O queue pair
     *
     * Deletes a previously created I/O queue pair.
     *
     * @param uQueueID          Queue identifier
     *
     * @retval IO_SUCCESS       Queue pair deleted
     */
    STDMETHOD_(IO_RETURN, DeleteIOQueue)(THIS_
        UINT32 uQueueID
        ) PURE;

    /**
     * @brief Set feature
     *
     * Executes the Set Features admin command.
     *
     * @param uFeatureID        Feature identifier
     * @param uValue            Feature value
     *
     * @retval IO_SUCCESS       Feature set successfully
     * @retval IO_UNSUPPORTED   Feature not supported
     */
    STDMETHOD_(IO_RETURN, SetFeature)(THIS_
        UINT32 uFeatureID,
        UINT32 uValue
        ) PURE;

    /**
     * @brief Get feature
     *
     * Executes the Get Features admin command.
     *
     * @param uFeatureID        Feature identifier
     * @param puValue           Receives feature value
     *
     * @retval IO_SUCCESS       Feature retrieved successfully
     * @retval IO_UNSUPPORTED   Feature not supported
     */
    STDMETHOD_(IO_RETURN, GetFeature)(THIS_
        UINT32 uFeatureID,
        UINT32 *puValue
        ) PURE;

    /**
     * @brief Reset controller
     *
     * Performs a controller-level reset.
     *
     * @retval IO_SUCCESS       Reset successful
     * @retval IO_ERROR         Reset failed
     */
    STDMETHOD_(IO_RETURN, ResetController)(THIS) PURE;
};

#undef INTERFACE

/**
 * @brief IIONVMeNamespace - NVMe Namespace interface
 *
 * This interface represents an NVMe namespace (logical storage unit)
 * and provides methods for I/O operations.
 */
#undef INTERFACE
#define INTERFACE IIONVMeNamespace

DECLARE_INTERFACE_(IIONVMeNamespace, IIOService)
{
    // All IIOService methods inherited...

    /**
     * @brief Get namespace information
     *
     * Retrieves namespace configuration and capacity information.
     *
     * @param pNamespaceInfo    Receives namespace information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetNamespaceInfo)(THIS_
        NVME_NAMESPACE_INFO *pNamespaceInfo
        ) PURE;

    /**
     * @brief Read data
     *
     * Reads data from the namespace.
     *
     * @param uLBA              Starting logical block address
     * @param uNumBlocks        Number of blocks to read
     * @param pBuffer           Buffer to receive data
     * @param cbBuffer          Buffer size in bytes
     *
     * @retval IO_SUCCESS       Read successful
     * @retval IO_ERROR         Read failed
     */
    STDMETHOD_(IO_RETURN, Read)(THIS_
        UINT64 uLBA,
        UINT32 uNumBlocks,
        VOID *pBuffer,
        UINTN cbBuffer
        ) PURE;

    /**
     * @brief Write data
     *
     * Writes data to the namespace.
     *
     * @param uLBA              Starting logical block address
     * @param uNumBlocks        Number of blocks to write
     * @param pBuffer           Buffer containing data
     * @param cbBuffer          Buffer size in bytes
     *
     * @retval IO_SUCCESS       Write successful
     * @retval IO_ERROR         Write failed
     */
    STDMETHOD_(IO_RETURN, Write)(THIS_
        UINT64 uLBA,
        UINT32 uNumBlocks,
        CONST VOID *pBuffer,
        UINTN cbBuffer
        ) PURE;

    /**
     * @brief Flush cache
     *
     * Flushes the volatile write cache.
     *
     * @retval IO_SUCCESS       Flush successful
     * @retval IO_UNSUPPORTED   No volatile cache
     */
    STDMETHOD_(IO_RETURN, Flush)(THIS) PURE;

    /**
     * @brief Format namespace
     *
     * Formats the namespace with specified parameters.
     *
     * @param uLBAFormat        LBA format index
     * @param bSecureErase      Perform secure erase
     *
     * @retval IO_SUCCESS       Format successful
     * @retval IO_ERROR         Format failed
     */
    STDMETHOD_(IO_RETURN, Format)(THIS_
        UINT8 uLBAFormat,
        BOOLEAN bSecureErase
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIONVMeController methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIONVMeController_GetControllerInfo(p,a)    (p)->lpVtbl->GetControllerInfo(p,a)
#define IIONVMeController_IdentifyController(p,a)   (p)->lpVtbl->IdentifyController(p,a)
#define IIONVMeController_GetNamespaceCount(p,a)    (p)->lpVtbl->GetNamespaceCount(p,a)
#define IIONVMeController_GetNamespace(p,a,b)       (p)->lpVtbl->GetNamespace(p,a,b)
#define IIONVMeController_CreateIOQueue(p,a,b)      (p)->lpVtbl->CreateIOQueue(p,a,b)
#define IIONVMeController_DeleteIOQueue(p,a)        (p)->lpVtbl->DeleteIOQueue(p,a)
#define IIONVMeController_SetFeature(p,a,b)         (p)->lpVtbl->SetFeature(p,a,b)
#define IIONVMeController_GetFeature(p,a,b)         (p)->lpVtbl->GetFeature(p,a,b)
#define IIONVMeController_ResetController(p)        (p)->lpVtbl->ResetController(p)

#define IIONVMeNamespace_GetNamespaceInfo(p,a)      (p)->lpVtbl->GetNamespaceInfo(p,a)
#define IIONVMeNamespace_Read(p,a,b,c,d)            (p)->lpVtbl->Read(p,a,b,c,d)
#define IIONVMeNamespace_Write(p,a,b,c,d)           (p)->lpVtbl->Write(p,a,b,c,d)
#define IIONVMeNamespace_Flush(p)                   (p)->lpVtbl->Flush(p)
#define IIONVMeNamespace_Format(p,a,b)              (p)->lpVtbl->Format(p,a,b)

#endif

/**
 * @brief Initialize NVMe family driver
 *
 * Initializes the NVMe family driver and registers controller classes.
 *
 * @retval IO_SUCCESS   Initialization successful
 * @retval IO_ERROR     Initialization failed
 */
IO_RETURN
NVMeInitialize(
    VOID
    );

/**
 * @brief Shutdown NVMe family driver
 *
 * Shuts down the NVMe family driver and releases resources.
 *
 * @retval IO_SUCCESS   Shutdown successful
 */
IO_RETURN
NVMeShutdown(
    VOID
    );

/**
 * @brief Create NVMe controller instance
 *
 * Creates an NVMe controller interface for a PCI device.
 *
 * @param pPCIDevice        PCI device interface
 * @param ppController      Receives controller interface
 *
 * @retval IO_SUCCESS       Controller created successfully
 * @retval IO_NO_MEMORY     Insufficient memory
 * @retval IO_NO_DEVICE     Not an NVMe controller
 */
IO_RETURN
NVMeControllerCreate(
    IIOService *pPCIDevice,
    IIONVMeController **ppController
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_NVME_H */
