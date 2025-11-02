/**
 * @file storage.h
 * @brief Storage Family Interface - Unified Block Storage Abstraction
 *
 * This header defines the Storage family interface providing a unified abstraction
 * layer for all block storage devices regardless of underlying protocol (NVMe, SATA,
 * SCSI, SAS). This allows upper layers to interact with storage devices through a
 * common interface without protocol-specific knowledge.
 *
 * The Storage family sits ABOVE protocol-specific drivers and provides:
 * - Protocol-agnostic block operations (Read, Write, Flush, Trim)
 * - Unified device enumeration and capabilities reporting
 * - Common error handling and status reporting
 * - Storage controller and device abstraction
 * - Support for advanced features (NCQ, SMART, encryption, TRIM/DISCARD)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_STORAGE_H
#define IOKIT_STORAGE_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOStorageDevice interface GUID
 * {A1B2C3D4-E5F6-4A5B-8C7D-9E0F1A2B3C4D}
 */
DEFINE_GUID(IID_IIOStorageDevice,
    0xA1B2C3D4, 0xE5F6, 0x4A5B, 0x8C, 0x7D, 0x9E, 0x0F, 0x1A, 0x2B, 0x3C, 0x4D);

/**
 * @brief IIOStorageController interface GUID
 * {B2C3D4E5-F6A7-4B6C-9D8E-0F1A2B3C4D5E}
 */
DEFINE_GUID(IID_IIOStorageController,
    0xB2C3D4E5, 0xF6A7, 0x4B6C, 0x9D, 0x8E, 0x0F, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E);

/**
 * @brief Storage Protocol Types
 *
 * Identifies the underlying storage protocol/interface used by the device.
 */
typedef enum _STORAGE_PROTOCOL {
    STORAGE_PROTOCOL_UNKNOWN    = 0x00,     /**< Unknown or unidentified protocol */
    STORAGE_PROTOCOL_NVME       = 0x01,     /**< NVM Express (PCIe attached) */
    STORAGE_PROTOCOL_SATA       = 0x02,     /**< Serial ATA */
    STORAGE_PROTOCOL_SCSI       = 0x03,     /**< SCSI (Small Computer System Interface) */
    STORAGE_PROTOCOL_SAS        = 0x04,     /**< Serial Attached SCSI */
    STORAGE_PROTOCOL_PATA       = 0x05,     /**< Parallel ATA (IDE/EIDE) - legacy */
    STORAGE_PROTOCOL_USB        = 0x06,     /**< USB Mass Storage */
    STORAGE_PROTOCOL_VIRTIO     = 0x07,     /**< VirtIO Block Device */
    STORAGE_PROTOCOL_EMMC       = 0x08,     /**< eMMC (embedded MultiMediaCard) */
    STORAGE_PROTOCOL_SD         = 0x09,     /**< SD Card */
    STORAGE_PROTOCOL_NVMEOF     = 0x0A,     /**< NVMe over Fabrics */
    STORAGE_PROTOCOL_ISCSI      = 0x0B,     /**< iSCSI (SCSI over IP) */
} STORAGE_PROTOCOL;

/**
 * @brief Storage Device Type
 */
typedef enum _STORAGE_DEVICE_TYPE {
    STORAGE_TYPE_UNKNOWN        = 0x00,     /**< Unknown device type */
    STORAGE_TYPE_HDD            = 0x01,     /**< Hard Disk Drive (rotating media) */
    STORAGE_TYPE_SSD            = 0x02,     /**< Solid State Drive */
    STORAGE_TYPE_HYBRID         = 0x03,     /**< Hybrid Drive (SSHD) */
    STORAGE_TYPE_OPTICAL        = 0x04,     /**< Optical Drive (CD/DVD/BD) */
    STORAGE_TYPE_TAPE           = 0x05,     /**< Tape Drive */
    STORAGE_TYPE_VIRTUAL        = 0x06,     /**< Virtual/RAM Disk */
    STORAGE_TYPE_NVDIMM         = 0x07,     /**< Non-Volatile DIMM */
} STORAGE_DEVICE_TYPE;

/**
 * @brief Storage Device Capabilities (Bitfield)
 *
 * Flags indicating which advanced features the storage device supports.
 */
#define STORAGE_CAP_NCQ                 0x00000001  /**< Native Command Queuing */
#define STORAGE_CAP_SMART               0x00000002  /**< S.M.A.R.T. monitoring */
#define STORAGE_CAP_TRIM                0x00000004  /**< TRIM/DISCARD support */
#define STORAGE_CAP_UNMAP               0x00000008  /**< SCSI UNMAP support */
#define STORAGE_CAP_WRITE_SAME          0x00000010  /**< Write Same command */
#define STORAGE_CAP_WRITE_ZEROES        0x00000020  /**< Write Zeroes command */
#define STORAGE_CAP_FLUSH_CACHE         0x00000040  /**< Cache flush support */
#define STORAGE_CAP_VOLATILE_CACHE      0x00000080  /**< Has volatile write cache */
#define STORAGE_CAP_FUA                 0x00000100  /**< Force Unit Access */
#define STORAGE_CAP_SANITIZE            0x00000200  /**< Sanitize/Crypto Erase */
#define STORAGE_CAP_SECURE_ERASE        0x00000400  /**< Secure Erase support */
#define STORAGE_CAP_ENCRYPTION          0x00000800  /**< Hardware encryption */
#define STORAGE_CAP_SELF_ENCRYPTING     0x00001000  /**< Self-Encrypting Drive (SED) */
#define STORAGE_CAP_TCG_OPAL            0x00002000  /**< TCG Opal encryption */
#define STORAGE_CAP_POWER_MANAGEMENT    0x00004000  /**< Advanced power management */
#define STORAGE_CAP_HOT_PLUG            0x00008000  /**< Hot-plug capable */
#define STORAGE_CAP_REMOVABLE           0x00010000  /**< Removable media */
#define STORAGE_CAP_READ_ONLY           0x00020000  /**< Read-only device */
#define STORAGE_CAP_WRITE_PROTECT       0x00040000  /**< Write protection support */
#define STORAGE_CAP_ATOMIC_WRITE        0x00080000  /**< Atomic write support */
#define STORAGE_CAP_ZONED               0x00100000  /**< Zoned storage (ZNS/ZBC) */
#define STORAGE_CAP_STREAMS             0x00200000  /**< Stream directives */
#define STORAGE_CAP_METADATA            0x00400000  /**< Metadata support */
#define STORAGE_CAP_DIF                 0x00800000  /**< Data Integrity Field (T10 DIF) */

/**
 * @brief Storage Device Status Flags
 */
#define STORAGE_STATUS_READY            0x00000001  /**< Device ready for I/O */
#define STORAGE_STATUS_BUSY             0x00000002  /**< Device busy */
#define STORAGE_STATUS_ERROR            0x00000004  /**< Error condition */
#define STORAGE_STATUS_MEDIA_PRESENT    0x00000008  /**< Media present (removable) */
#define STORAGE_STATUS_WRITE_PROTECTED  0x00000010  /**< Write protected */
#define STORAGE_STATUS_FORMATTING       0x00000020  /**< Format in progress */
#define STORAGE_STATUS_SANITIZING       0x00000040  /**< Sanitize in progress */
#define STORAGE_STATUS_LOCKED           0x00000080  /**< Security locked */
#define STORAGE_STATUS_FROZEN           0x00000100  /**< Security frozen */

/**
 * @brief Storage Command Flags
 */
#define STORAGE_CMD_FLAG_FUA            0x00000001  /**< Force Unit Access (bypass cache) */
#define STORAGE_CMD_FLAG_DPO            0x00000002  /**< Disable Page Out (cache hint) */
#define STORAGE_CMD_FLAG_SYNC           0x00000004  /**< Synchronous operation */
#define STORAGE_CMD_FLAG_BARRIER        0x00000008  /**< Barrier (order guarantee) */
#define STORAGE_CMD_FLAG_URGENT         0x00000010  /**< High priority/urgent */
#define STORAGE_CMD_FLAG_BACKGROUND     0x00000020  /**< Background/low priority */

/**
 * @brief Storage Device Information
 *
 * Comprehensive information structure describing a storage device's
 * characteristics, capabilities, and current state.
 */
typedef struct _STORAGE_DEVICE_INFO {
    STORAGE_PROTOCOL    Protocol;               /**< Storage protocol type */
    STORAGE_DEVICE_TYPE DeviceType;             /**< Device type (HDD/SSD/etc) */

    // Capacity and Geometry
    UINT64              TotalCapacity;          /**< Total capacity in bytes */
    UINT64              UsableCapacity;         /**< Usable capacity in bytes */
    UINT32              BlockSize;              /**< Block/sector size in bytes */
    UINT32              PhysicalBlockSize;      /**< Physical block size (if different) */
    UINT64              TotalBlocks;            /**< Total number of blocks */

    // Identification
    CHAR8               Vendor[40];             /**< Vendor/manufacturer */
    CHAR8               Model[40];              /**< Model number/name */
    CHAR8               SerialNumber[40];       /**< Serial number */
    CHAR8               FirmwareRevision[16];   /**< Firmware revision */

    // Capabilities and Features
    UINT32              Capabilities;           /**< Capability flags (STORAGE_CAP_*) */
    UINT32              MaxTransferSize;        /**< Maximum transfer size (bytes) */
    UINT32              OptimalTransferSize;    /**< Optimal transfer size (bytes) */
    UINT32              MaxQueueDepth;          /**< Maximum command queue depth */
    UINT32              NumQueues;              /**< Number of I/O queues (NVMe) */

    // Performance Characteristics
    UINT32              RotationRate;           /**< Rotation rate (RPM, 0=SSD, 1=non-rotating) */
    UINT8               FormFactor;             /**< Form factor (2.5", 3.5", M.2, etc) */

    // Status and State
    UINT32              Status;                 /**< Current status flags */
    BOOLEAN             bRemovable;             /**< Removable media flag */
    BOOLEAN             bHotPluggable;          /**< Hot-pluggable flag */
    BOOLEAN             bMediaPresent;          /**< Media present (removable only) */
    BOOLEAN             bWriteProtected;        /**< Write protected */

    // Health and Reliability (SMART-like)
    UINT8               HealthPercentage;       /**< Health status (0-100%, 255=unknown) */
    UINT64              PowerOnHours;           /**< Power-on hours */
    UINT64              DataUnitsRead;          /**< Total data units read */
    UINT64              DataUnitsWritten;       /**< Total data units written */

    // Protocol-Specific Information
    UINT32              ProtocolVersion;        /**< Protocol version (format varies) */
    VOID               *pProtocolInfo;          /**< Protocol-specific info pointer */
    UINTN               cbProtocolInfo;         /**< Size of protocol info */
} STORAGE_DEVICE_INFO;

/**
 * @brief Storage Controller Information
 *
 * Information about the storage controller/host adapter that manages
 * one or more storage devices.
 */
typedef struct _STORAGE_CONTROLLER_INFO {
    STORAGE_PROTOCOL    Protocol;               /**< Controller protocol type */
    CHAR8               ControllerName[64];     /**< Controller name/description */
    CHAR8               VendorName[40];         /**< Controller vendor */
    CHAR8               FirmwareVersion[16];    /**< Controller firmware version */

    // Controller Capabilities
    UINT32              MaxDevices;             /**< Maximum devices supported */
    UINT32              NumPorts;               /**< Number of ports */
    UINT32              MaxTransferSize;        /**< Maximum transfer size */
    UINT32              MaxQueueDepth;          /**< Maximum queue depth */
    UINT32              Capabilities;           /**< Capability flags */

    // PCIe/Bus Information (if applicable)
    UINT16              VendorID;               /**< PCI Vendor ID */
    UINT16              DeviceID;               /**< PCI Device ID */
    UINT8               BusNumber;              /**< Bus number */
    UINT8               DeviceNumber;           /**< Device number */
    UINT8               FunctionNumber;         /**< Function number */

    // Status
    UINT32              NumAttachedDevices;     /**< Currently attached devices */
    BOOLEAN             bHotPlugSupport;        /**< Hot-plug support */
    BOOLEAN             bRaidCapable;           /**< RAID capable */
} STORAGE_CONTROLLER_INFO;

/**
 * @brief Storage I/O Statistics
 */
typedef struct _STORAGE_IO_STATS {
    UINT64              ReadOperations;         /**< Total read operations */
    UINT64              WriteOperations;        /**< Total write operations */
    UINT64              FlushOperations;        /**< Total flush operations */
    UINT64              TrimOperations;         /**< Total TRIM/UNMAP operations */
    UINT64              BytesRead;              /**< Total bytes read */
    UINT64              BytesWritten;           /**< Total bytes written */
    UINT64              ReadErrors;             /**< Read error count */
    UINT64              WriteErrors;            /**< Write error count */
    UINT64              AverageReadLatencyNs;   /**< Average read latency (ns) */
    UINT64              AverageWriteLatencyNs;  /**< Average write latency (ns) */
} STORAGE_IO_STATS;

/**
 * @brief Storage SMART Attribute (simplified)
 */
typedef struct _STORAGE_SMART_ATTRIBUTE {
    UINT8               ID;                     /**< Attribute ID */
    UINT16              Flags;                  /**< Attribute flags */
    UINT8               Current;                /**< Current value */
    UINT8               Worst;                  /**< Worst value */
    UINT64              RawValue;               /**< Raw value */
    UINT8               Threshold;              /**< Threshold value */
    CHAR8               Name[32];               /**< Attribute name */
} STORAGE_SMART_ATTRIBUTE;

/**
 * @brief Storage SMART Information
 */
typedef struct _STORAGE_SMART_INFO {
    BOOLEAN             bSupported;             /**< SMART supported */
    BOOLEAN             bEnabled;               /**< SMART enabled */
    BOOLEAN             bHealthOK;              /**< Overall health status */
    UINT8               HealthPercentage;       /**< Health percentage (0-100) */
    UINT32              Temperature;            /**< Temperature (Celsius) */
    UINT32              NumAttributes;          /**< Number of attributes */
    STORAGE_SMART_ATTRIBUTE *pAttributes;       /**< Array of attributes */
} STORAGE_SMART_INFO;

//
// Forward declarations
//
DECLARE_INTERFACE_(IIOStorageDevice, IIOService);
DECLARE_INTERFACE_(IIOStorageController, IIOService);

/**
 * @brief IIOStorageDevice - Storage Device Interface
 *
 * This interface represents a single storage device (disk, SSD, etc.) and
 * provides protocol-agnostic methods for block I/O operations and device
 * management.
 */
#undef INTERFACE
#define INTERFACE IIOStorageDevice

DECLARE_INTERFACE_(IIOStorageDevice, IIOService)
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

    // IIOStorageDevice methods

    /**
     * @brief Get device information
     *
     * Retrieves comprehensive information about the storage device including
     * capacity, capabilities, identification, and health status.
     *
     * @param pDeviceInfo   Receives device information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_NOT_READY     Device not ready
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        STORAGE_DEVICE_INFO *pDeviceInfo
        ) PURE;

    /**
     * @brief Read blocks from device
     *
     * Reads one or more blocks from the storage device. This is the primary
     * read operation used by all upper layers.
     *
     * @param uStartBlock       Starting block address (LBA)
     * @param uNumBlocks        Number of blocks to read
     * @param pBuffer           Buffer to receive data
     * @param cbBuffer          Size of buffer in bytes
     * @param uFlags            Command flags (STORAGE_CMD_FLAG_*)
     * @param puBytesRead       Receives actual bytes read (may be NULL)
     *
     * @retval IO_SUCCESS       Read completed successfully
     * @retval IO_IO_ERROR      I/O error occurred
     * @retval IO_BAD_ARGUMENT  Invalid arguments
     * @retval IO_NOT_READY     Device not ready
     * @retval IO_NO_MEDIA      No media present (removable)
     */
    STDMETHOD_(IO_RETURN, Read)(THIS_
        UINT64 uStartBlock,
        UINT32 uNumBlocks,
        VOID *pBuffer,
        UINTN cbBuffer,
        UINT32 uFlags,
        UINTN *puBytesRead
        ) PURE;

    /**
     * @brief Write blocks to device
     *
     * Writes one or more blocks to the storage device. This is the primary
     * write operation used by all upper layers.
     *
     * @param uStartBlock       Starting block address (LBA)
     * @param uNumBlocks        Number of blocks to write
     * @param pBuffer           Buffer containing data to write
     * @param cbBuffer          Size of buffer in bytes
     * @param uFlags            Command flags (STORAGE_CMD_FLAG_*)
     * @param puBytesWritten    Receives actual bytes written (may be NULL)
     *
     * @retval IO_SUCCESS       Write completed successfully
     * @retval IO_IO_ERROR      I/O error occurred
     * @retval IO_BAD_ARGUMENT  Invalid arguments
     * @retval IO_NOT_READY     Device not ready
     * @retval IO_NOT_WRITABLE  Device is read-only or write-protected
     */
    STDMETHOD_(IO_RETURN, Write)(THIS_
        UINT64 uStartBlock,
        UINT32 uNumBlocks,
        CONST VOID *pBuffer,
        UINTN cbBuffer,
        UINT32 uFlags,
        UINTN *puBytesWritten
        ) PURE;

    /**
     * @brief Flush device cache
     *
     * Flushes the device's volatile write cache, ensuring all buffered
     * writes are committed to persistent storage.
     *
     * @retval IO_SUCCESS       Cache flushed successfully
     * @retval IO_UNSUPPORTED   Device has no volatile cache
     * @retval IO_IO_ERROR      Flush failed
     */
    STDMETHOD_(IO_RETURN, Flush)(THIS) PURE;

    /**
     * @brief TRIM/DISCARD blocks
     *
     * Notifies the device that specified blocks are no longer in use and
     * can be erased (SSD) or deallocated. This is essential for SSD
     * performance and longevity.
     *
     * @param uStartBlock       Starting block address (LBA)
     * @param uNumBlocks        Number of blocks to trim
     *
     * @retval IO_SUCCESS       TRIM completed successfully
     * @retval IO_UNSUPPORTED   TRIM not supported
     * @retval IO_IO_ERROR      TRIM failed
     */
    STDMETHOD_(IO_RETURN, Trim)(THIS_
        UINT64 uStartBlock,
        UINT32 uNumBlocks
        ) PURE;

    /**
     * @brief Write zeroes to blocks
     *
     * Efficiently writes zeroes to the specified block range. Devices
     * may optimize this operation internally.
     *
     * @param uStartBlock       Starting block address (LBA)
     * @param uNumBlocks        Number of blocks to zero
     * @param uFlags            Command flags
     *
     * @retval IO_SUCCESS       Zeroes written successfully
     * @retval IO_UNSUPPORTED   Write Zeroes not supported
     * @retval IO_IO_ERROR      Operation failed
     */
    STDMETHOD_(IO_RETURN, WriteZeroes)(THIS_
        UINT64 uStartBlock,
        UINT32 uNumBlocks,
        UINT32 uFlags
        ) PURE;

    /**
     * @brief Get I/O statistics
     *
     * Retrieves I/O performance statistics for the device.
     *
     * @param pStats            Receives I/O statistics
     * @param bReset            Reset statistics after reading
     *
     * @retval IO_SUCCESS       Statistics retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetIOStats)(THIS_
        STORAGE_IO_STATS *pStats,
        BOOLEAN bReset
        ) PURE;

    /**
     * @brief Get SMART information
     *
     * Retrieves S.M.A.R.T. monitoring data and health information.
     *
     * @param pSmartInfo        Receives SMART information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_UNSUPPORTED   SMART not supported
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetSMARTInfo)(THIS_
        STORAGE_SMART_INFO *pSmartInfo
        ) PURE;

    /**
     * @brief Perform secure erase
     *
     * Securely erases all data on the device. This operation is
     * irreversible and may take significant time to complete.
     *
     * @param bEnhanced         Use enhanced secure erase if available
     *
     * @retval IO_SUCCESS       Erase completed successfully
     * @retval IO_UNSUPPORTED   Secure erase not supported
     * @retval IO_IO_ERROR      Erase failed
     */
    STDMETHOD_(IO_RETURN, SecureErase)(THIS_
        BOOLEAN bEnhanced
        ) PURE;

    /**
     * @brief Sanitize device
     *
     * Performs sanitization (crypto erase or block erase) to remove
     * all user data. More thorough than secure erase.
     *
     * @retval IO_SUCCESS       Sanitize completed successfully
     * @retval IO_UNSUPPORTED   Sanitize not supported
     * @retval IO_IO_ERROR      Sanitize failed
     */
    STDMETHOD_(IO_RETURN, Sanitize)(THIS) PURE;

    /**
     * @brief Set power state
     *
     * Changes the device power state for power management.
     *
     * @param uPowerState       Power state index (device-specific)
     *
     * @retval IO_SUCCESS       Power state changed successfully
     * @retval IO_UNSUPPORTED   Power management not supported
     * @retval IO_BAD_ARGUMENT  Invalid power state
     */
    STDMETHOD_(IO_RETURN, SetPowerState)(THIS_
        UINT32 uPowerState
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOStorageController - Storage Controller Interface
 *
 * This interface represents a storage controller (host adapter) that manages
 * one or more storage devices. Examples include NVMe controllers, SATA
 * controllers, SAS HBAs, etc.
 */
#undef INTERFACE
#define INTERFACE IIOStorageController

DECLARE_INTERFACE_(IIOStorageController, IIOService)
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

    // IIOStorageController methods

    /**
     * @brief Get controller information
     *
     * Retrieves information about the storage controller.
     *
     * @param pControllerInfo   Receives controller information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetControllerInfo)(THIS_
        STORAGE_CONTROLLER_INFO *pControllerInfo
        ) PURE;

    /**
     * @brief Enumerate attached devices
     *
     * Scans for and enumerates all storage devices attached to this
     * controller. Creates device instances for each discovered device.
     *
     * @retval IO_SUCCESS       Enumeration completed successfully
     * @retval IO_ERROR         Enumeration failed
     */
    STDMETHOD_(IO_RETURN, EnumerateDevices)(THIS) PURE;

    /**
     * @brief Get device count
     *
     * Returns the number of storage devices currently attached to
     * this controller.
     *
     * @param puCount           Receives device count
     *
     * @retval IO_SUCCESS       Count retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDeviceCount)(THIS_
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Get storage device by index
     *
     * Retrieves a storage device interface by index.
     *
     * @param uIndex            Device index (0-based)
     * @param ppDevice          Receives storage device interface
     *
     * @retval IO_SUCCESS       Device retrieved successfully
     * @retval IO_NO_DEVICE     Invalid index
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDevice)(THIS_
        UINT32 uIndex,
        IIOStorageDevice **ppDevice
        ) PURE;

    /**
     * @brief Reset controller
     *
     * Performs a controller-level reset. All attached devices will
     * be re-enumerated after the reset.
     *
     * @retval IO_SUCCESS       Reset completed successfully
     * @retval IO_ERROR         Reset failed
     */
    STDMETHOD_(IO_RETURN, ResetController)(THIS) PURE;

    /**
     * @brief Reset port/device
     *
     * Resets a specific port or device on the controller.
     *
     * @param uPortIndex        Port/device index
     *
     * @retval IO_SUCCESS       Port reset successfully
     * @retval IO_BAD_ARGUMENT  Invalid port index
     * @retval IO_ERROR         Reset failed
     */
    STDMETHOD_(IO_RETURN, ResetPort)(THIS_
        UINT32 uPortIndex
        ) PURE;

    /**
     * @brief Get port status
     *
     * Retrieves the status of a specific port (link state, device
     * attached, etc.)
     *
     * @param uPortIndex        Port index
     * @param puStatus          Receives port status flags
     *
     * @retval IO_SUCCESS       Status retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetPortStatus)(THIS_
        UINT32 uPortIndex,
        UINT32 *puStatus
        ) PURE;

    /**
     * @brief Enable/disable hot-plug support
     *
     * Enables or disables hot-plug detection on the controller.
     *
     * @param bEnable           TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS       Hot-plug configured successfully
     * @retval IO_UNSUPPORTED   Hot-plug not supported
     */
    STDMETHOD_(IO_RETURN, SetHotPlugEnable)(THIS_
        BOOLEAN bEnable
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOStorageDevice methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOStorageDevice_GetDeviceInfo(p,a)         (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOStorageDevice_Read(p,a,b,c,d,e,f)        (p)->lpVtbl->Read(p,a,b,c,d,e,f)
#define IIOStorageDevice_Write(p,a,b,c,d,e,f)       (p)->lpVtbl->Write(p,a,b,c,d,e,f)
#define IIOStorageDevice_Flush(p)                   (p)->lpVtbl->Flush(p)
#define IIOStorageDevice_Trim(p,a,b)                (p)->lpVtbl->Trim(p,a,b)
#define IIOStorageDevice_WriteZeroes(p,a,b,c)       (p)->lpVtbl->WriteZeroes(p,a,b,c)
#define IIOStorageDevice_GetIOStats(p,a,b)          (p)->lpVtbl->GetIOStats(p,a,b)
#define IIOStorageDevice_GetSMARTInfo(p,a)          (p)->lpVtbl->GetSMARTInfo(p,a)
#define IIOStorageDevice_SecureErase(p,a)           (p)->lpVtbl->SecureErase(p,a)
#define IIOStorageDevice_Sanitize(p)                (p)->lpVtbl->Sanitize(p)
#define IIOStorageDevice_SetPowerState(p,a)         (p)->lpVtbl->SetPowerState(p,a)

#define IIOStorageController_GetControllerInfo(p,a) (p)->lpVtbl->GetControllerInfo(p,a)
#define IIOStorageController_EnumerateDevices(p)    (p)->lpVtbl->EnumerateDevices(p)
#define IIOStorageController_GetDeviceCount(p,a)    (p)->lpVtbl->GetDeviceCount(p,a)
#define IIOStorageController_GetDevice(p,a,b)       (p)->lpVtbl->GetDevice(p,a,b)
#define IIOStorageController_ResetController(p)     (p)->lpVtbl->ResetController(p)
#define IIOStorageController_ResetPort(p,a)         (p)->lpVtbl->ResetPort(p,a)
#define IIOStorageController_GetPortStatus(p,a,b)   (p)->lpVtbl->GetPortStatus(p,a,b)
#define IIOStorageController_SetHotPlugEnable(p,a)  (p)->lpVtbl->SetHotPlugEnable(p,a)

#endif

/**
 * @brief Initialize Storage family subsystem
 *
 * Initializes the storage abstraction layer and registers it with IOKit.
 *
 * @retval IO_SUCCESS   Initialization successful
 * @retval IO_ERROR     Initialization failed
 */
IO_RETURN
StorageInitialize(
    VOID
    );

/**
 * @brief Shutdown Storage family subsystem
 *
 * Shuts down the storage abstraction layer and releases resources.
 *
 * @retval IO_SUCCESS   Shutdown successful
 */
IO_RETURN
StorageShutdown(
    VOID
    );

/**
 * @brief Create a storage device instance
 *
 * Creates a storage device interface wrapping a protocol-specific device.
 *
 * @param pProtocolDevice   Protocol-specific device (NVMe, SATA, etc.)
 * @param Protocol          Storage protocol type
 * @param ppDevice          Receives storage device interface
 *
 * @retval IO_SUCCESS           Device created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid argument
 */
IO_RETURN
StorageDeviceCreate(
    IIOService *pProtocolDevice,
    STORAGE_PROTOCOL Protocol,
    IIOStorageDevice **ppDevice
    );

/**
 * @brief Create a storage controller instance
 *
 * Creates a storage controller interface wrapping a protocol-specific controller.
 *
 * @param pProtocolController   Protocol-specific controller
 * @param Protocol              Storage protocol type
 * @param ppController          Receives storage controller interface
 *
 * @retval IO_SUCCESS           Controller created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid argument
 */
IO_RETURN
StorageControllerCreate(
    IIOService *pProtocolController,
    STORAGE_PROTOCOL Protocol,
    IIOStorageController **ppController
    );

/**
 * @brief Detect storage protocol from device
 *
 * Helper function to detect the storage protocol of a device by examining
 * its properties and characteristics.
 *
 * @param pDevice           Device to examine
 * @param pProtocol         Receives detected protocol type
 *
 * @retval IO_SUCCESS       Protocol detected successfully
 * @retval IO_NO_MATCH      Protocol could not be determined
 * @retval IO_BAD_ARGUMENT  Invalid argument
 */
IO_RETURN
StorageDetectProtocol(
    IIOService *pDevice,
    STORAGE_PROTOCOL *pProtocol
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_STORAGE_H */
