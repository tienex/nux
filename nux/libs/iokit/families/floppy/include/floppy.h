/**
 * @file floppy.h
 * @brief Floppy Family Interface - Floppy Disk and High-Capacity Removable Media Driver
 *
 * This header defines the Floppy family interface for floppy disk controllers and drives,
 * supporting traditional 3.5"/5.25" floppy disks, high-capacity formats (Zip, Jaz, LS-120),
 * and various interface types (ISA, USB, parallel port, SCSI, ATAPI).
 *
 * The Floppy family provides:
 * - Standard floppy disk support (360KB, 720KB, 1.2MB, 1.44MB, 2.88MB)
 * - High-capacity floppy formats (Zip 100/250/750MB, Jaz 1/2GB, LS-120/240)
 * - Multiple interface types (ISA FDC, USB, parallel port, SCSI, ATAPI)
 * - Media detection and format identification
 * - Read/write/format operations with CHS geometry
 * - Write protection and media presence detection
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_FLOPPY_H
#define IOKIT_FLOPPY_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOFloppyController interface GUID
 * {D5E6F7A8-B9C0-4D1E-A2F3-B4C5D6E7F8A9}
 */
DEFINE_GUID(IID_IIOFloppyController,
    0xD5E6F7A8, 0xB9C0, 0x4D1E, 0xA2, 0xF3, 0xB4, 0xC5, 0xD6, 0xE7, 0xF8, 0xA9);

/**
 * @brief IIOFloppyDrive interface GUID
 * {E6F7A8B9-C0D1-4E2F-B3A4-C5D6E7F8A9B0}
 */
DEFINE_GUID(IID_IIOFloppyDrive,
    0xE6F7A8B9, 0xC0D1, 0x4E2F, 0xB3, 0xA4, 0xC5, 0xD6, 0xE7, 0xF8, 0xA9, 0xB0);

/**
 * @brief Floppy Controller Types
 */
typedef enum _FLOPPY_CONTROLLER_TYPE {
    FLOPPY_CONTROLLER_UNKNOWN       = 0x00,     /**< Unknown controller */
    FLOPPY_CONTROLLER_ISA_FDC       = 0x01,     /**< ISA Floppy Disk Controller (PC standard) */
    FLOPPY_CONTROLLER_USB           = 0x02,     /**< USB floppy drive */
    FLOPPY_CONTROLLER_PARALLEL      = 0x03,     /**< Parallel port floppy (BackPack, etc.) */
    FLOPPY_CONTROLLER_SCSI          = 0x04,     /**< SCSI-attached floppy/removable */
    FLOPPY_CONTROLLER_ATAPI         = 0x05,     /**< ATAPI/IDE-attached floppy */
    FLOPPY_CONTROLLER_EMBEDDED      = 0x06,     /**< Embedded/integrated controller */
} FLOPPY_CONTROLLER_TYPE;

/**
 * @brief Floppy Drive Types (Physical Format)
 */
typedef enum _FLOPPY_DRIVE_TYPE {
    FLOPPY_DRIVE_UNKNOWN            = 0x00,     /**< Unknown drive type */
    FLOPPY_DRIVE_360K               = 0x01,     /**< 5.25" 360KB drive */
    FLOPPY_DRIVE_1200K              = 0x02,     /**< 5.25" 1.2MB drive */
    FLOPPY_DRIVE_720K               = 0x03,     /**< 3.5" 720KB drive */
    FLOPPY_DRIVE_1440K              = 0x04,     /**< 3.5" 1.44MB drive */
    FLOPPY_DRIVE_2880K              = 0x05,     /**< 3.5" 2.88MB drive (ED) */
    FLOPPY_DRIVE_ZIP_100            = 0x10,     /**< Iomega Zip 100MB */
    FLOPPY_DRIVE_ZIP_250            = 0x11,     /**< Iomega Zip 250MB */
    FLOPPY_DRIVE_ZIP_750            = 0x12,     /**< Iomega Zip 750MB */
    FLOPPY_DRIVE_JAZ_1GB            = 0x13,     /**< Iomega Jaz 1GB */
    FLOPPY_DRIVE_JAZ_2GB            = 0x14,     /**< Iomega Jaz 2GB */
    FLOPPY_DRIVE_LS120              = 0x20,     /**< LS-120 SuperDisk 120MB */
    FLOPPY_DRIVE_LS240              = 0x21,     /**< LS-240 SuperDisk 240MB */
    FLOPPY_DRIVE_HIFD               = 0x30,     /**< Sony HiFD 200MB */
    FLOPPY_DRIVE_UHD144             = 0x31,     /**< Caleb UHD144 144MB */
} FLOPPY_DRIVE_TYPE;

/**
 * @brief Floppy Media Types (Logical Format)
 */
typedef enum _FLOPPY_MEDIA_TYPE {
    FLOPPY_MEDIA_UNKNOWN            = 0x00,     /**< Unknown or no media */
    // Standard 5.25" formats
    FLOPPY_MEDIA_5_360K             = 0x01,     /**< 5.25" 360KB (DD, 40 tracks, 9 sectors) */
    FLOPPY_MEDIA_5_1200K            = 0x02,     /**< 5.25" 1.2MB (HD, 80 tracks, 15 sectors) */
    // Standard 3.5" formats
    FLOPPY_MEDIA_3_720K             = 0x03,     /**< 3.5" 720KB (DD, 80 tracks, 9 sectors) */
    FLOPPY_MEDIA_3_1440K            = 0x04,     /**< 3.5" 1.44MB (HD, 80 tracks, 18 sectors) */
    FLOPPY_MEDIA_3_2880K            = 0x05,     /**< 3.5" 2.88MB (ED, 80 tracks, 36 sectors) */
    // DMF (Distribution Media Format) - Microsoft
    FLOPPY_MEDIA_3_DMF              = 0x06,     /**< 3.5" DMF 1.68MB (21 sectors) */
    // High-capacity formats
    FLOPPY_MEDIA_ZIP_100            = 0x10,     /**< Iomega Zip 100MB */
    FLOPPY_MEDIA_ZIP_250            = 0x11,     /**< Iomega Zip 250MB */
    FLOPPY_MEDIA_ZIP_750            = 0x12,     /**< Iomega Zip 750MB */
    FLOPPY_MEDIA_JAZ_1GB            = 0x13,     /**< Iomega Jaz 1GB */
    FLOPPY_MEDIA_JAZ_2GB            = 0x14,     /**< Iomega Jaz 2GB */
    FLOPPY_MEDIA_LS120              = 0x20,     /**< LS-120 SuperDisk 120MB */
    FLOPPY_MEDIA_LS240              = 0x21,     /**< LS-240 SuperDisk 240MB */
    FLOPPY_MEDIA_LS120_FLOPPY       = 0x22,     /**< Standard 1.44MB floppy in LS-120 */
    FLOPPY_MEDIA_HIFD               = 0x30,     /**< Sony HiFD 200MB */
    FLOPPY_MEDIA_UHD144             = 0x31,     /**< Caleb UHD144 144MB */
} FLOPPY_MEDIA_TYPE;

/**
 * @brief Floppy Interface Types
 */
typedef enum _FLOPPY_INTERFACE_TYPE {
    FLOPPY_INTERFACE_ISA            = 0x00,     /**< ISA bus (PC standard FDC) */
    FLOPPY_INTERFACE_USB            = 0x01,     /**< USB interface */
    FLOPPY_INTERFACE_PARALLEL       = 0x02,     /**< Parallel port */
    FLOPPY_INTERFACE_SCSI           = 0x03,     /**< SCSI interface */
    FLOPPY_INTERFACE_ATAPI          = 0x04,     /**< ATAPI/IDE interface */
} FLOPPY_INTERFACE_TYPE;

/**
 * @brief Floppy Drive Capabilities (Bitfield)
 */
#define FLOPPY_CAP_READ                 0x00000001  /**< Read support */
#define FLOPPY_CAP_WRITE                0x00000002  /**< Write support */
#define FLOPPY_CAP_FORMAT               0x00000004  /**< Format support */
#define FLOPPY_CAP_VERIFY               0x00000008  /**< Verify after write */
#define FLOPPY_CAP_WRITE_PROTECT_DETECT 0x00000010  /**< Write protect detection */
#define FLOPPY_CAP_MEDIA_DETECT         0x00000020  /**< Media presence detection */
#define FLOPPY_CAP_CHANGE_LINE          0x00000040  /**< Disk change line support */
#define FLOPPY_CAP_PERPENDICULAR        0x00000080  /**< Perpendicular recording (2.88MB) */
#define FLOPPY_CAP_MULTIPLE_FORMATS     0x00000100  /**< Multiple format support */
#define FLOPPY_CAP_EJECT                0x00000200  /**< Motorized eject */
#define FLOPPY_CAP_LOCK                 0x00000400  /**< Door lock support */
#define FLOPPY_CAP_HOT_PLUG             0x00000800  /**< Hot-plug capable */
#define FLOPPY_CAP_ATAPI_PACKET         0x00001000  /**< ATAPI packet commands */
#define FLOPPY_CAP_VARIABLE_SPEED       0x00002000  /**< Variable rotation speed */

/**
 * @brief Floppy Drive Status Flags
 */
#define FLOPPY_STATUS_READY             0x00000001  /**< Drive ready */
#define FLOPPY_STATUS_MEDIA_PRESENT     0x00000002  /**< Media inserted */
#define FLOPPY_STATUS_WRITE_PROTECTED   0x00000004  /**< Write protected */
#define FLOPPY_STATUS_MOTOR_ON          0x00000008  /**< Motor running */
#define FLOPPY_STATUS_HEAD_LOADED       0x00000010  /**< Head loaded */
#define FLOPPY_STATUS_TRACK_0           0x00000020  /**< At track 0 */
#define FLOPPY_STATUS_INDEX             0x00000040  /**< Index hole detected */
#define FLOPPY_STATUS_MEDIA_CHANGED     0x00000080  /**< Media changed since last access */
#define FLOPPY_STATUS_DOOR_LOCKED       0x00000100  /**< Door locked */
#define FLOPPY_STATUS_BUSY              0x00000200  /**< Drive busy */
#define FLOPPY_STATUS_ERROR             0x00000400  /**< Error condition */
#define FLOPPY_STATUS_SEEK_ERROR        0x00000800  /**< Seek error */
#define FLOPPY_STATUS_CRC_ERROR         0x00001000  /**< CRC/ECC error */
#define FLOPPY_STATUS_TIMEOUT           0x00002000  /**< Operation timeout */

/**
 * @brief Floppy Drive Geometry (CHS addressing)
 */
typedef struct _FLOPPY_GEOMETRY {
    UINT32              Cylinders;              /**< Number of cylinders (tracks) */
    UINT32              Heads;                  /**< Number of heads (sides) */
    UINT32              SectorsPerTrack;        /**< Sectors per track */
    UINT32              BytesPerSector;         /**< Bytes per sector (typically 512) */
    UINT64              TotalSectors;           /**< Total sectors on media */
    UINT64              TotalBytes;             /**< Total capacity in bytes */
    UINT32              DataRate;               /**< Data transfer rate (Kbps) */
    UINT8               Gap3Length;             /**< Gap3 length for format */
    UINT8               FormatFillByte;         /**< Fill byte for format (0xF6, 0xE5, etc.) */
    BOOLEAN             bPerpendicular;         /**< Perpendicular recording mode */
} FLOPPY_GEOMETRY;

/**
 * @brief Floppy Controller Information
 */
typedef struct _FLOPPY_CONTROLLER_INFO {
    FLOPPY_CONTROLLER_TYPE  ControllerType;     /**< Controller type */
    FLOPPY_INTERFACE_TYPE   InterfaceType;      /**< Interface type */
    CHAR8                   ControllerName[64]; /**< Controller name */
    CHAR8                   VendorName[40];     /**< Vendor name */
    CHAR8                   FirmwareVersion[16];/**< Firmware version (if applicable) */

    // Controller Capabilities
    UINT32              MaxDrives;              /**< Maximum drives supported */
    UINT32              Capabilities;           /**< Capability flags */
    BOOLEAN             bDMA;                   /**< DMA support */
    BOOLEAN             bFIFO;                  /**< FIFO buffer support */
    BOOLEAN             bEnhanced;              /**< Enhanced FDC (82077/82078) */

    // ISA FDC specific
    UINT16              IOBase;                 /**< I/O base address (ISA) */
    UINT8               IRQ;                    /**< IRQ number (ISA) */
    UINT8               DMAChannel;             /**< DMA channel (ISA) */

    // USB/SCSI/ATAPI specific
    UINT16              VendorID;               /**< USB/PCI Vendor ID */
    UINT16              ProductID;              /**< USB/PCI Product ID */
} FLOPPY_CONTROLLER_INFO;

/**
 * @brief Floppy Drive Information
 */
typedef struct _FLOPPY_DRIVE_INFO {
    FLOPPY_DRIVE_TYPE   DriveType;              /**< Physical drive type */
    FLOPPY_MEDIA_TYPE   MediaType;              /**< Current media type */
    CHAR8               VendorName[40];         /**< Drive vendor */
    CHAR8               ModelName[40];          /**< Drive model */
    CHAR8               SerialNumber[40];       /**< Serial number */
    CHAR8               FirmwareVersion[16];    /**< Firmware version */

    // Drive Characteristics
    UINT32              Capabilities;           /**< Drive capabilities */
    UINT32              Status;                 /**< Current drive status */
    FLOPPY_GEOMETRY     Geometry;               /**< Current media geometry */
    BOOLEAN             bRemovable;             /**< Removable media (always TRUE) */
    BOOLEAN             bMediaPresent;          /**< Media present */
    BOOLEAN             bWriteProtected;        /**< Write protected */
    BOOLEAN             bFormatted;             /**< Media formatted */

    // Performance Characteristics
    UINT32              RPM;                    /**< Rotation speed (RPM) */
    UINT32              SeekTimeMs;             /**< Average seek time (ms) */
    UINT32              MotorSpinUpMs;          /**< Motor spin-up time (ms) */
    UINT32              MotorSpinDownMs;        /**< Motor spin-down time (ms) */
} FLOPPY_DRIVE_INFO;

/**
 * @brief Floppy Format Parameters
 */
typedef struct _FLOPPY_FORMAT_PARAMS {
    FLOPPY_MEDIA_TYPE   MediaType;              /**< Target media type */
    UINT32              Cylinders;              /**< Cylinders to format */
    UINT32              Heads;                  /**< Heads to format */
    UINT32              SectorsPerTrack;        /**< Sectors per track */
    UINT32              BytesPerSector;         /**< Bytes per sector */
    UINT8               Gap3Length;             /**< Gap3 length */
    UINT8               FormatFillByte;         /**< Fill byte (0xF6 for standard) */
    BOOLEAN             bVerify;                /**< Verify after format */
    BOOLEAN             bQuickFormat;           /**< Quick format (high-capacity) */
} FLOPPY_FORMAT_PARAMS;

/**
 * @brief Floppy I/O Statistics
 */
typedef struct _FLOPPY_IO_STATS {
    UINT64              ReadOperations;         /**< Total read operations */
    UINT64              WriteOperations;        /**< Total write operations */
    UINT64              FormatOperations;       /**< Total format operations */
    UINT64              SeekOperations;         /**< Total seek operations */
    UINT64              SectorsRead;            /**< Total sectors read */
    UINT64              SectorsWritten;         /**< Total sectors written */
    UINT64              ReadErrors;             /**< Read error count */
    UINT64              WriteErrors;            /**< Write error count */
    UINT64              SeekErrors;             /**< Seek error count */
    UINT64              CRCErrors;              /**< CRC error count */
    UINT64              TimeoutErrors;          /**< Timeout error count */
    UINT64              MediaChanges;           /**< Media change count */
} FLOPPY_IO_STATS;

//
// Forward declarations
//
DECLARE_INTERFACE_(IIOFloppyController, IIOService);
DECLARE_INTERFACE_(IIOFloppyDrive, IIOService);

/**
 * @brief IIOFloppyController - Floppy Controller Interface
 *
 * This interface represents a floppy disk controller and provides methods
 * for controller management, drive enumeration, and low-level FDC operations.
 */
#undef INTERFACE
#define INTERFACE IIOFloppyController

DECLARE_INTERFACE_(IIOFloppyController, IIOService)
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

    // IIOFloppyController methods

    /**
     * @brief Get controller information
     *
     * Retrieves information about the floppy controller.
     *
     * @param pControllerInfo   Receives controller information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetControllerInfo)(THIS_
        FLOPPY_CONTROLLER_INFO *pControllerInfo
        ) PURE;

    /**
     * @brief Enumerate attached drives
     *
     * Scans for and enumerates all floppy drives attached to this controller.
     *
     * @retval IO_SUCCESS       Enumeration completed successfully
     * @retval IO_ERROR         Enumeration failed
     */
    STDMETHOD_(IO_RETURN, EnumerateDrives)(THIS) PURE;

    /**
     * @brief Get drive count
     *
     * Returns the number of floppy drives attached to this controller.
     *
     * @param puCount           Receives drive count
     *
     * @retval IO_SUCCESS       Count retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDriveCount)(THIS_
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Get floppy drive by index
     *
     * Retrieves a floppy drive interface by index.
     *
     * @param uIndex            Drive index (0-based)
     * @param ppDrive           Receives floppy drive interface
     *
     * @retval IO_SUCCESS       Drive retrieved successfully
     * @retval IO_NO_DEVICE     Invalid index
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDrive)(THIS_
        UINT32 uIndex,
        IIOFloppyDrive **ppDrive
        ) PURE;

    /**
     * @brief Reset controller
     *
     * Performs a controller-level reset.
     *
     * @retval IO_SUCCESS       Reset completed successfully
     * @retval IO_ERROR         Reset failed
     */
    STDMETHOD_(IO_RETURN, ResetController)(THIS) PURE;

    /**
     * @brief Enable/disable DMA
     *
     * Enables or disables DMA transfers for the controller.
     *
     * @param bEnable           TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS       DMA configured successfully
     * @retval IO_UNSUPPORTED   DMA not supported
     */
    STDMETHOD_(IO_RETURN, SetDMAEnable)(THIS_
        BOOLEAN bEnable
        ) PURE;

    /**
     * @brief Set data transfer rate
     *
     * Sets the controller data transfer rate.
     *
     * @param uDataRate         Data rate in Kbps (250, 300, 500, 1000)
     *
     * @retval IO_SUCCESS       Data rate set successfully
     * @retval IO_BAD_ARGUMENT  Invalid data rate
     */
    STDMETHOD_(IO_RETURN, SetDataRate)(THIS_
        UINT32 uDataRate
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOFloppyDrive - Floppy Drive Interface
 *
 * This interface represents a single floppy drive and provides methods
 * for media operations, reading, writing, and formatting.
 */
#undef INTERFACE
#define INTERFACE IIOFloppyDrive

DECLARE_INTERFACE_(IIOFloppyDrive, IIOService)
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

    // IIOFloppyDrive methods

    /**
     * @brief Get drive information
     *
     * Retrieves comprehensive information about the floppy drive.
     *
     * @param pDriveInfo        Receives drive information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDriveInfo)(THIS_
        FLOPPY_DRIVE_INFO *pDriveInfo
        ) PURE;

    /**
     * @brief Detect media type
     *
     * Detects the type of media currently inserted in the drive.
     *
     * @param pMediaType        Receives detected media type
     *
     * @retval IO_SUCCESS       Media type detected successfully
     * @retval IO_NO_MEDIA      No media present
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, DetectMediaType)(THIS_
        FLOPPY_MEDIA_TYPE *pMediaType
        ) PURE;

    /**
     * @brief Get media geometry
     *
     * Retrieves the geometry of the current media.
     *
     * @param pGeometry         Receives media geometry
     *
     * @retval IO_SUCCESS       Geometry retrieved successfully
     * @retval IO_NO_MEDIA      No media present
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetMediaGeometry)(THIS_
        FLOPPY_GEOMETRY *pGeometry
        ) PURE;

    /**
     * @brief Read sectors (CHS addressing)
     *
     * Reads sectors using Cylinder/Head/Sector addressing.
     *
     * @param uCylinder         Cylinder (track) number
     * @param uHead             Head (side) number
     * @param uSector           Sector number (1-based)
     * @param uCount            Number of sectors to read
     * @param pBuffer           Buffer to receive data
     * @param cbBuffer          Size of buffer in bytes
     * @param puBytesRead       Receives actual bytes read (may be NULL)
     *
     * @retval IO_SUCCESS       Read completed successfully
     * @retval IO_IO_ERROR      I/O error occurred
     * @retval IO_NO_MEDIA      No media present
     * @retval IO_BAD_ARGUMENT  Invalid arguments
     */
    STDMETHOD_(IO_RETURN, ReadSectorsCHS)(THIS_
        UINT32 uCylinder,
        UINT32 uHead,
        UINT32 uSector,
        UINT32 uCount,
        VOID *pBuffer,
        UINTN cbBuffer,
        UINTN *puBytesRead
        ) PURE;

    /**
     * @brief Write sectors (CHS addressing)
     *
     * Writes sectors using Cylinder/Head/Sector addressing.
     *
     * @param uCylinder         Cylinder (track) number
     * @param uHead             Head (side) number
     * @param uSector           Sector number (1-based)
     * @param uCount            Number of sectors to write
     * @param pBuffer           Buffer containing data to write
     * @param cbBuffer          Size of buffer in bytes
     * @param puBytesWritten    Receives actual bytes written (may be NULL)
     *
     * @retval IO_SUCCESS       Write completed successfully
     * @retval IO_IO_ERROR      I/O error occurred
     * @retval IO_NO_MEDIA      No media present
     * @retval IO_NOT_WRITABLE  Media is write-protected
     * @retval IO_BAD_ARGUMENT  Invalid arguments
     */
    STDMETHOD_(IO_RETURN, WriteSectorsCHS)(THIS_
        UINT32 uCylinder,
        UINT32 uHead,
        UINT32 uSector,
        UINT32 uCount,
        CONST VOID *pBuffer,
        UINTN cbBuffer,
        UINTN *puBytesWritten
        ) PURE;

    /**
     * @brief Read sectors (LBA addressing)
     *
     * Reads sectors using Logical Block Addressing.
     *
     * @param uLBA              Logical block address (0-based)
     * @param uCount            Number of sectors to read
     * @param pBuffer           Buffer to receive data
     * @param cbBuffer          Size of buffer in bytes
     * @param puBytesRead       Receives actual bytes read (may be NULL)
     *
     * @retval IO_SUCCESS       Read completed successfully
     * @retval IO_IO_ERROR      I/O error occurred
     * @retval IO_NO_MEDIA      No media present
     * @retval IO_BAD_ARGUMENT  Invalid arguments
     */
    STDMETHOD_(IO_RETURN, ReadSectorsLBA)(THIS_
        UINT64 uLBA,
        UINT32 uCount,
        VOID *pBuffer,
        UINTN cbBuffer,
        UINTN *puBytesRead
        ) PURE;

    /**
     * @brief Write sectors (LBA addressing)
     *
     * Writes sectors using Logical Block Addressing.
     *
     * @param uLBA              Logical block address (0-based)
     * @param uCount            Number of sectors to write
     * @param pBuffer           Buffer containing data to write
     * @param cbBuffer          Size of buffer in bytes
     * @param puBytesWritten    Receives actual bytes written (may be NULL)
     *
     * @retval IO_SUCCESS       Write completed successfully
     * @retval IO_IO_ERROR      I/O error occurred
     * @retval IO_NO_MEDIA      No media present
     * @retval IO_NOT_WRITABLE  Media is write-protected
     * @retval IO_BAD_ARGUMENT  Invalid arguments
     */
    STDMETHOD_(IO_RETURN, WriteSectorsLBA)(THIS_
        UINT64 uLBA,
        UINT32 uCount,
        CONST VOID *pBuffer,
        UINTN cbBuffer,
        UINTN *puBytesWritten
        ) PURE;

    /**
     * @brief Format media
     *
     * Formats the media according to the specified parameters.
     *
     * @param pParams           Format parameters
     *
     * @retval IO_SUCCESS       Format completed successfully
     * @retval IO_IO_ERROR      Format failed
     * @retval IO_NO_MEDIA      No media present
     * @retval IO_NOT_WRITABLE  Media is write-protected
     * @retval IO_BAD_ARGUMENT  Invalid parameters
     */
    STDMETHOD_(IO_RETURN, FormatMedia)(THIS_
        CONST FLOPPY_FORMAT_PARAMS *pParams
        ) PURE;

    /**
     * @brief Verify sectors
     *
     * Verifies that sectors are readable and data integrity is intact.
     *
     * @param uLBA              Starting LBA
     * @param uCount            Number of sectors to verify
     *
     * @retval IO_SUCCESS       Verification successful
     * @retval IO_IO_ERROR      Verification failed
     * @retval IO_NO_MEDIA      No media present
     */
    STDMETHOD_(IO_RETURN, VerifySectors)(THIS_
        UINT64 uLBA,
        UINT32 uCount
        ) PURE;

    /**
     * @brief Seek to cylinder
     *
     * Seeks the drive head to the specified cylinder.
     *
     * @param uCylinder         Target cylinder number
     *
     * @retval IO_SUCCESS       Seek completed successfully
     * @retval IO_IO_ERROR      Seek failed
     * @retval IO_NO_MEDIA      No media present
     */
    STDMETHOD_(IO_RETURN, SeekToCylinder)(THIS_
        UINT32 uCylinder
        ) PURE;

    /**
     * @brief Recalibrate drive
     *
     * Recalibrates the drive by seeking to track 0.
     *
     * @retval IO_SUCCESS       Recalibration successful
     * @retval IO_IO_ERROR      Recalibration failed
     */
    STDMETHOD_(IO_RETURN, Recalibrate)(THIS) PURE;

    /**
     * @brief Motor control
     *
     * Controls the drive motor (spin up/down).
     *
     * @param bMotorOn          TRUE to spin up, FALSE to spin down
     *
     * @retval IO_SUCCESS       Motor control successful
     * @retval IO_UNSUPPORTED   Motor control not supported
     */
    STDMETHOD_(IO_RETURN, SetMotor)(THIS_
        BOOLEAN bMotorOn
        ) PURE;

    /**
     * @brief Eject media
     *
     * Ejects the media (if drive supports motorized eject).
     *
     * @retval IO_SUCCESS       Media ejected successfully
     * @retval IO_UNSUPPORTED   Eject not supported
     * @retval IO_ERROR         Eject failed
     */
    STDMETHOD_(IO_RETURN, EjectMedia)(THIS) PURE;

    /**
     * @brief Lock/unlock door
     *
     * Locks or unlocks the drive door (high-capacity drives).
     *
     * @param bLock             TRUE to lock, FALSE to unlock
     *
     * @retval IO_SUCCESS       Lock state changed successfully
     * @retval IO_UNSUPPORTED   Door lock not supported
     */
    STDMETHOD_(IO_RETURN, SetDoorLock)(THIS_
        BOOLEAN bLock
        ) PURE;

    /**
     * @brief Get I/O statistics
     *
     * Retrieves I/O performance statistics for the drive.
     *
     * @param pStats            Receives I/O statistics
     * @param bReset            Reset statistics after reading
     *
     * @retval IO_SUCCESS       Statistics retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetIOStats)(THIS_
        FLOPPY_IO_STATS *pStats,
        BOOLEAN bReset
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOFloppyController methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOFloppyController_GetControllerInfo(p,a)     (p)->lpVtbl->GetControllerInfo(p,a)
#define IIOFloppyController_EnumerateDrives(p)         (p)->lpVtbl->EnumerateDrives(p)
#define IIOFloppyController_GetDriveCount(p,a)         (p)->lpVtbl->GetDriveCount(p,a)
#define IIOFloppyController_GetDrive(p,a,b)            (p)->lpVtbl->GetDrive(p,a,b)
#define IIOFloppyController_ResetController(p)         (p)->lpVtbl->ResetController(p)
#define IIOFloppyController_SetDMAEnable(p,a)          (p)->lpVtbl->SetDMAEnable(p,a)
#define IIOFloppyController_SetDataRate(p,a)           (p)->lpVtbl->SetDataRate(p,a)

#define IIOFloppyDrive_GetDriveInfo(p,a)               (p)->lpVtbl->GetDriveInfo(p,a)
#define IIOFloppyDrive_DetectMediaType(p,a)            (p)->lpVtbl->DetectMediaType(p,a)
#define IIOFloppyDrive_GetMediaGeometry(p,a)           (p)->lpVtbl->GetMediaGeometry(p,a)
#define IIOFloppyDrive_ReadSectorsCHS(p,a,b,c,d,e,f,g) (p)->lpVtbl->ReadSectorsCHS(p,a,b,c,d,e,f,g)
#define IIOFloppyDrive_WriteSectorsCHS(p,a,b,c,d,e,f,g)(p)->lpVtbl->WriteSectorsCHS(p,a,b,c,d,e,f,g)
#define IIOFloppyDrive_ReadSectorsLBA(p,a,b,c,d,e)     (p)->lpVtbl->ReadSectorsLBA(p,a,b,c,d,e)
#define IIOFloppyDrive_WriteSectorsLBA(p,a,b,c,d,e)    (p)->lpVtbl->WriteSectorsLBA(p,a,b,c,d,e)
#define IIOFloppyDrive_FormatMedia(p,a)                (p)->lpVtbl->FormatMedia(p,a)
#define IIOFloppyDrive_VerifySectors(p,a,b)            (p)->lpVtbl->VerifySectors(p,a,b)
#define IIOFloppyDrive_SeekToCylinder(p,a)             (p)->lpVtbl->SeekToCylinder(p,a)
#define IIOFloppyDrive_Recalibrate(p)                  (p)->lpVtbl->Recalibrate(p)
#define IIOFloppyDrive_SetMotor(p,a)                   (p)->lpVtbl->SetMotor(p,a)
#define IIOFloppyDrive_EjectMedia(p)                   (p)->lpVtbl->EjectMedia(p)
#define IIOFloppyDrive_SetDoorLock(p,a)                (p)->lpVtbl->SetDoorLock(p,a)
#define IIOFloppyDrive_GetIOStats(p,a,b)               (p)->lpVtbl->GetIOStats(p,a,b)

#endif

/**
 * @brief Initialize Floppy family subsystem
 *
 * Initializes the floppy driver subsystem and registers it with IOKit.
 *
 * @retval IO_SUCCESS   Initialization successful
 * @retval IO_ERROR     Initialization failed
 */
IO_RETURN
FloppyInitialize(
    VOID
    );

/**
 * @brief Shutdown Floppy family subsystem
 *
 * Shuts down the floppy driver subsystem and releases resources.
 *
 * @retval IO_SUCCESS   Shutdown successful
 */
IO_RETURN
FloppyShutdown(
    VOID
    );

/**
 * @brief Create a floppy controller instance
 *
 * Creates a floppy controller interface.
 *
 * @param ControllerType    Controller type
 * @param pProvider         Provider service (ISA/USB/etc.)
 * @param ppController      Receives floppy controller interface
 *
 * @retval IO_SUCCESS           Controller created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid argument
 */
IO_RETURN
FloppyControllerCreate(
    FLOPPY_CONTROLLER_TYPE ControllerType,
    IIOService *pProvider,
    IIOFloppyController **ppController
    );

/**
 * @brief Create a floppy drive instance
 *
 * Creates a floppy drive interface.
 *
 * @param pController       Parent controller
 * @param DriveType         Drive type
 * @param uDriveNumber      Drive number (0-3)
 * @param ppDrive           Receives floppy drive interface
 *
 * @retval IO_SUCCESS           Drive created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid argument
 */
IO_RETURN
FloppyDriveCreate(
    IIOFloppyController *pController,
    FLOPPY_DRIVE_TYPE DriveType,
    UINT32 uDriveNumber,
    IIOFloppyDrive **ppDrive
    );

/**
 * @brief Get geometry for media type
 *
 * Helper function to get standard geometry for a media type.
 *
 * @param MediaType         Media type
 * @param pGeometry         Receives geometry information
 *
 * @retval IO_SUCCESS       Geometry retrieved successfully
 * @retval IO_BAD_ARGUMENT  Invalid media type
 */
IO_RETURN
FloppyGetMediaGeometry(
    FLOPPY_MEDIA_TYPE MediaType,
    FLOPPY_GEOMETRY *pGeometry
    );

/**
 * @brief Convert CHS to LBA
 *
 * Converts Cylinder/Head/Sector address to Logical Block Address.
 *
 * @param pGeometry         Media geometry
 * @param uCylinder         Cylinder number
 * @param uHead             Head number
 * @param uSector           Sector number (1-based)
 * @param puLBA             Receives LBA (0-based)
 *
 * @retval IO_SUCCESS       Conversion successful
 * @retval IO_BAD_ARGUMENT  Invalid arguments
 */
IO_RETURN
FloppyCHSToLBA(
    CONST FLOPPY_GEOMETRY *pGeometry,
    UINT32 uCylinder,
    UINT32 uHead,
    UINT32 uSector,
    UINT64 *puLBA
    );

/**
 * @brief Convert LBA to CHS
 *
 * Converts Logical Block Address to Cylinder/Head/Sector address.
 *
 * @param pGeometry         Media geometry
 * @param uLBA              Logical block address (0-based)
 * @param puCylinder        Receives cylinder number
 * @param puHead            Receives head number
 * @param puSector          Receives sector number (1-based)
 *
 * @retval IO_SUCCESS       Conversion successful
 * @retval IO_BAD_ARGUMENT  Invalid arguments
 */
IO_RETURN
FloppyLBAToCHS(
    CONST FLOPPY_GEOMETRY *pGeometry,
    UINT64 uLBA,
    UINT32 *puCylinder,
    UINT32 *puHead,
    UINT32 *puSector
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_FLOPPY_H */
