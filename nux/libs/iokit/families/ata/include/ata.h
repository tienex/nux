/**
 * @file ata.h
 * @brief ATA/IDE Family Interface - ATA/ATAPI/IDE Driver
 *
 * This header defines the ATA family interface for IDE/ATA/ATAPI/PATA storage
 * devices, supporting the complete ATA command set from the earliest ST506/ST412
 * interfaces through modern PATA implementations. This includes support for:
 * - ST506/ST412 (MFM/RLL - 1980, 5 Mbps)
 * - ESDI (Enhanced Small Device Interface - 1983-1990s, 10-24 Mbps)
 * - IDE/ATA-1 (1986-1994, PIO modes 0-2)
 * - ATA-2/EIDE (1996, PIO modes 3-4, DMA modes 0-2)
 * - ATA-3/ATAPI (1997, S.M.A.R.T., security features)
 * - ATA-4/Ultra DMA (1998, UDMA modes 0-2, up to 33 MB/s)
 * - ATA-5 (2000, UDMA modes 3-4, up to 66 MB/s)
 * - ATA-6 (2002, UDMA mode 5, up to 100 MB/s, 48-bit LBA)
 * - ATA-7 (2005, UDMA mode 6, up to 133 MB/s)
 * - ATAPI (CD/DVD/Tape drives using SCSI commands over ATA)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_ATA_H
#define IOKIT_ATA_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOATAController interface GUID
 * {C9D8E7F6-4A3B-5C2D-9E8F-0A1B2C3D4E5F}
 */
DEFINE_GUID(IID_IIOATAController,
    0xC9D8E7F6, 0x4A3B, 0x5C2D, 0x9E, 0x8F, 0x0A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F);

/**
 * @brief IIOATADevice interface GUID
 * {DAE9F807-5B4C-6D3E-AF90-1B2C3D4E5F60}
 */
DEFINE_GUID(IID_IIOATADevice,
    0xDAE9F807, 0x5B4C, 0x6D3E, 0xAF, 0x90, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F, 0x60);

/**
 * @brief ATA Protocol/Interface Versions
 */
typedef enum _ATA_PROTOCOL {
    ATA_PROTOCOL_ST506        = 0x01,     /**< ST506/ST412 MFM/RLL (1980) */
    ATA_PROTOCOL_ESDI         = 0x02,     /**< ESDI (1983) */
    ATA_PROTOCOL_IDE          = 0x03,     /**< IDE/ATA-1 (1986) */
    ATA_PROTOCOL_EIDE         = 0x04,     /**< EIDE/ATA-2 (1996) */
    ATA_PROTOCOL_ATAPI        = 0x05,     /**< ATAPI/ATA-3 (1997) */
    ATA_PROTOCOL_UDMA33       = 0x06,     /**< Ultra DMA/33 ATA-4 (1998) */
    ATA_PROTOCOL_UDMA66       = 0x07,     /**< Ultra DMA/66 ATA-5 (2000) */
    ATA_PROTOCOL_UDMA100      = 0x08,     /**< Ultra DMA/100 ATA-6 (2002) */
    ATA_PROTOCOL_UDMA133      = 0x09,     /**< Ultra DMA/133 ATA-7 (2005) */
    ATA_PROTOCOL_PATA         = 0x0A,     /**< Generic Parallel ATA */
    ATA_PROTOCOL_AOE          = 0x0B,     /**< ATA over Ethernet (2005) */
} ATA_PROTOCOL;

/**
 * @brief ATA Device Types
 */
typedef enum _ATA_DEVICE_TYPE {
    ATA_DEVICE_UNKNOWN        = 0x00,     /**< Unknown device type */
    ATA_DEVICE_HDD            = 0x01,     /**< ATA Hard Disk Drive */
    ATA_DEVICE_CDROM          = 0x02,     /**< ATAPI CD-ROM drive */
    ATA_DEVICE_DVDROM         = 0x03,     /**< ATAPI DVD-ROM drive */
    ATA_DEVICE_ZIP            = 0x04,     /**< ATAPI Zip drive */
    ATA_DEVICE_TAPE           = 0x05,     /**< ATAPI Tape drive */
    ATA_DEVICE_OPTICAL        = 0x06,     /**< ATAPI Optical drive */
    ATA_DEVICE_CDRW           = 0x07,     /**< ATAPI CD-RW drive */
    ATA_DEVICE_DVDRW          = 0x08,     /**< ATAPI DVD±RW drive */
    ATA_DEVICE_MO             = 0x09,     /**< ATAPI Magneto-Optical */
    ATA_DEVICE_LS120          = 0x0A,     /**< ATAPI LS-120 SuperDisk */
} ATA_DEVICE_TYPE;

/**
 * @brief ATA PIO Transfer Modes
 */
typedef enum _ATA_PIO_MODE {
    ATA_PIO_MODE_0            = 0,        /**< PIO Mode 0: 3.3 MB/s (ATA-1) */
    ATA_PIO_MODE_1            = 1,        /**< PIO Mode 1: 5.2 MB/s (ATA-1) */
    ATA_PIO_MODE_2            = 2,        /**< PIO Mode 2: 8.3 MB/s (ATA-1) */
    ATA_PIO_MODE_3            = 3,        /**< PIO Mode 3: 11.1 MB/s (ATA-2) */
    ATA_PIO_MODE_4            = 4,        /**< PIO Mode 4: 16.7 MB/s (ATA-2) */
} ATA_PIO_MODE;

/**
 * @brief ATA Multiword DMA Modes
 */
typedef enum _ATA_MWDMA_MODE {
    ATA_MWDMA_MODE_0          = 0,        /**< Multiword DMA Mode 0: 4.2 MB/s */
    ATA_MWDMA_MODE_1          = 1,        /**< Multiword DMA Mode 1: 13.3 MB/s */
    ATA_MWDMA_MODE_2          = 2,        /**< Multiword DMA Mode 2: 16.7 MB/s */
} ATA_MWDMA_MODE;

/**
 * @brief ATA Ultra DMA Modes
 */
typedef enum _ATA_UDMA_MODE {
    ATA_UDMA_MODE_0           = 0,        /**< UDMA Mode 0: 16.7 MB/s (ATA-4) */
    ATA_UDMA_MODE_1           = 1,        /**< UDMA Mode 1: 25.0 MB/s (ATA-4) */
    ATA_UDMA_MODE_2           = 2,        /**< UDMA Mode 2: 33.3 MB/s (ATA-4) */
    ATA_UDMA_MODE_3           = 3,        /**< UDMA Mode 3: 44.4 MB/s (ATA-5) */
    ATA_UDMA_MODE_4           = 4,        /**< UDMA Mode 4: 66.7 MB/s (ATA-5) */
    ATA_UDMA_MODE_5           = 5,        /**< UDMA Mode 5: 100 MB/s (ATA-6) */
    ATA_UDMA_MODE_6           = 6,        /**< UDMA Mode 6: 133 MB/s (ATA-7) */
} ATA_UDMA_MODE;

/**
 * @brief ATA Transfer Mode Types
 */
typedef enum _ATA_TRANSFER_TYPE {
    ATA_TRANSFER_PIO          = 0x00,     /**< PIO (Programmed I/O) */
    ATA_TRANSFER_MWDMA        = 0x01,     /**< Multiword DMA */
    ATA_TRANSFER_UDMA         = 0x02,     /**< Ultra DMA */
} ATA_TRANSFER_TYPE;

/**
 * @brief ATA Register Addresses (Task File Registers)
 */
#define ATA_REG_DATA              0x00    /**< Data register (16-bit) */
#define ATA_REG_ERROR             0x01    /**< Error register (read) */
#define ATA_REG_FEATURES          0x01    /**< Features register (write) */
#define ATA_REG_SECTOR_COUNT      0x02    /**< Sector count register */
#define ATA_REG_LBA_LOW           0x03    /**< LBA low (bits 0-7) */
#define ATA_REG_LBA_MID           0x04    /**< LBA mid (bits 8-15) */
#define ATA_REG_LBA_HIGH          0x05    /**< LBA high (bits 16-23) */
#define ATA_REG_DEVICE            0x06    /**< Device/head register */
#define ATA_REG_STATUS            0x07    /**< Status register (read) */
#define ATA_REG_COMMAND           0x07    /**< Command register (write) */
#define ATA_REG_ALT_STATUS        0x206   /**< Alternate status (read) */
#define ATA_REG_DEVICE_CONTROL    0x206   /**< Device control (write) */
#define ATA_REG_DRIVE_ADDRESS     0x207   /**< Drive address register */

/**
 * @brief ATA Status Register Bits
 */
#define ATA_STATUS_ERR            0x01    /**< Error */
#define ATA_STATUS_IDX            0x02    /**< Index (obsolete) */
#define ATA_STATUS_CORR           0x04    /**< Corrected data (obsolete) */
#define ATA_STATUS_DRQ            0x08    /**< Data request */
#define ATA_STATUS_DSC            0x10    /**< Drive seek complete (obsolete) */
#define ATA_STATUS_DWF            0x20    /**< Drive write fault */
#define ATA_STATUS_DRDY           0x40    /**< Drive ready */
#define ATA_STATUS_BSY            0x80    /**< Busy */

/**
 * @brief ATA Error Register Bits
 */
#define ATA_ERROR_AMNF            0x01    /**< Address mark not found */
#define ATA_ERROR_TK0NF           0x02    /**< Track 0 not found */
#define ATA_ERROR_ABRT            0x04    /**< Aborted command */
#define ATA_ERROR_MCR             0x08    /**< Media change request */
#define ATA_ERROR_IDNF            0x10    /**< ID not found */
#define ATA_ERROR_MC              0x20    /**< Media changed */
#define ATA_ERROR_UNC             0x40    /**< Uncorrectable data error */
#define ATA_ERROR_BBK             0x80    /**< Bad block detected */

/**
 * @brief ATA Device Control Register Bits
 */
#define ATA_DEVCTL_nIEN           0x02    /**< Interrupt enable (negated) */
#define ATA_DEVCTL_SRST           0x04    /**< Software reset */
#define ATA_DEVCTL_HOB            0x80    /**< High order byte (48-bit LBA) */

/**
 * @brief ATA Device/Head Register Bits
 */
#define ATA_DEVICE_DEV            0x10    /**< Device select (0=Master, 1=Slave) */
#define ATA_DEVICE_LBA            0x40    /**< LBA mode enabled */

/**
 * @brief ATA Commands
 */
#define ATA_CMD_NOP                     0x00    /**< NOP */
#define ATA_CMD_DEVICE_RESET            0x08    /**< Device Reset (ATAPI) */
#define ATA_CMD_READ_SECTORS            0x20    /**< Read sectors with retry */
#define ATA_CMD_READ_SECTORS_NORETRY    0x21    /**< Read sectors without retry */
#define ATA_CMD_READ_LONG               0x22    /**< Read long with retry */
#define ATA_CMD_READ_LONG_NORETRY       0x23    /**< Read long without retry */
#define ATA_CMD_READ_EXT                0x24    /**< Read sectors ext (48-bit) */
#define ATA_CMD_READ_DMA_EXT            0x25    /**< Read DMA ext (48-bit) */
#define ATA_CMD_READ_DMA_QUEUED_EXT     0x26    /**< Read DMA queued ext */
#define ATA_CMD_READ_NATIVE_MAX_EXT     0x27    /**< Read native max address ext */
#define ATA_CMD_READ_MULTIPLE_EXT       0x29    /**< Read multiple ext */
#define ATA_CMD_WRITE_SECTORS           0x30    /**< Write sectors with retry */
#define ATA_CMD_WRITE_SECTORS_NORETRY   0x31    /**< Write sectors without retry */
#define ATA_CMD_WRITE_LONG              0x32    /**< Write long with retry */
#define ATA_CMD_WRITE_LONG_NORETRY      0x33    /**< Write long without retry */
#define ATA_CMD_WRITE_EXT               0x34    /**< Write sectors ext (48-bit) */
#define ATA_CMD_WRITE_DMA_EXT           0x35    /**< Write DMA ext (48-bit) */
#define ATA_CMD_WRITE_DMA_QUEUED_EXT    0x36    /**< Write DMA queued ext */
#define ATA_CMD_SET_MAX_EXT             0x37    /**< Set max address ext */
#define ATA_CMD_CFA_WRITE_SECTORS       0x38    /**< CFA write sectors */
#define ATA_CMD_WRITE_MULTIPLE_EXT      0x39    /**< Write multiple ext */
#define ATA_CMD_WRITE_VERIFY            0x3C    /**< Write verify */
#define ATA_CMD_VERIFY_SECTORS          0x40    /**< Verify sectors with retry */
#define ATA_CMD_VERIFY_SECTORS_NORETRY  0x41    /**< Verify sectors without retry */
#define ATA_CMD_VERIFY_EXT              0x42    /**< Verify sectors ext */
#define ATA_CMD_FORMAT_TRACK            0x50    /**< Format track */
#define ATA_CMD_SEEK                    0x70    /**< Seek */
#define ATA_CMD_CFA_TRANSLATE_SECTOR    0x87    /**< CFA translate sector */
#define ATA_CMD_EXECUTE_DEVICE_DIAG     0x90    /**< Execute device diagnostic */
#define ATA_CMD_INIT_DEVICE_PARAMS      0x91    /**< Initialize device parameters */
#define ATA_CMD_DOWNLOAD_MICROCODE      0x92    /**< Download microcode */
#define ATA_CMD_PACKET                  0xA0    /**< ATAPI Packet */
#define ATA_CMD_IDENTIFY_PACKET         0xA1    /**< Identify ATAPI device */
#define ATA_CMD_SERVICE                 0xA2    /**< Service */
#define ATA_CMD_SMART                   0xB0    /**< S.M.A.R.T. */
#define ATA_CMD_CFA_ERASE_SECTORS       0xC0    /**< CFA erase sectors */
#define ATA_CMD_READ_MULTIPLE           0xC4    /**< Read multiple */
#define ATA_CMD_WRITE_MULTIPLE          0xC5    /**< Write multiple */
#define ATA_CMD_SET_MULTIPLE_MODE       0xC6    /**< Set multiple mode */
#define ATA_CMD_READ_DMA_QUEUED         0xC7    /**< Read DMA queued */
#define ATA_CMD_READ_DMA                0xC8    /**< Read DMA with retry */
#define ATA_CMD_READ_DMA_NORETRY        0xC9    /**< Read DMA without retry */
#define ATA_CMD_WRITE_DMA               0xCA    /**< Write DMA with retry */
#define ATA_CMD_WRITE_DMA_NORETRY       0xCB    /**< Write DMA without retry */
#define ATA_CMD_WRITE_DMA_QUEUED        0xCC    /**< Write DMA queued */
#define ATA_CMD_CFA_WRITE_MULTIPLE      0xCD    /**< CFA write multiple */
#define ATA_CMD_STANDBY_IMMEDIATE       0xE0    /**< Standby immediate */
#define ATA_CMD_IDLE_IMMEDIATE          0xE1    /**< Idle immediate */
#define ATA_CMD_STANDBY                 0xE2    /**< Standby */
#define ATA_CMD_IDLE                    0xE3    /**< Idle */
#define ATA_CMD_READ_BUFFER             0xE4    /**< Read buffer */
#define ATA_CMD_CHECK_POWER_MODE        0xE5    /**< Check power mode */
#define ATA_CMD_SLEEP                   0xE6    /**< Sleep */
#define ATA_CMD_FLUSH_CACHE             0xE7    /**< Flush cache */
#define ATA_CMD_WRITE_BUFFER            0xE8    /**< Write buffer */
#define ATA_CMD_FLUSH_CACHE_EXT         0xEA    /**< Flush cache ext */
#define ATA_CMD_IDENTIFY                0xEC    /**< Identify device */
#define ATA_CMD_SET_FEATURES            0xEF    /**< Set features */
#define ATA_CMD_SECURITY_SET_PASSWORD   0xF1    /**< Security set password */
#define ATA_CMD_SECURITY_UNLOCK         0xF2    /**< Security unlock */
#define ATA_CMD_SECURITY_ERASE_PREPARE  0xF3    /**< Security erase prepare */
#define ATA_CMD_SECURITY_ERASE_UNIT     0xF4    /**< Security erase unit */
#define ATA_CMD_SECURITY_FREEZE_LOCK    0xF5    /**< Security freeze lock */
#define ATA_CMD_SECURITY_DISABLE_PASS   0xF6    /**< Security disable password */
#define ATA_CMD_READ_NATIVE_MAX         0xF8    /**< Read native max address */
#define ATA_CMD_SET_MAX                 0xF9    /**< Set max address */

/**
 * @brief ATA SMART Sub-commands (Feature register values)
 */
#define ATA_SMART_READ_DATA             0xD0    /**< Read SMART data */
#define ATA_SMART_READ_THRESHOLD        0xD1    /**< Read SMART thresholds */
#define ATA_SMART_AUTOSAVE              0xD2    /**< Enable/disable autosave */
#define ATA_SMART_SAVE                  0xD3    /**< Save SMART attributes */
#define ATA_SMART_EXECUTE_OFFLINE       0xD4    /**< Execute offline immediate */
#define ATA_SMART_READ_LOG              0xD5    /**< Read SMART log */
#define ATA_SMART_WRITE_LOG             0xD6    /**< Write SMART log */
#define ATA_SMART_ENABLE                0xD8    /**< Enable SMART */
#define ATA_SMART_DISABLE               0xD9    /**< Disable SMART */
#define ATA_SMART_STATUS                0xDA    /**< Return SMART status */

/**
 * @brief ATA SET FEATURES Sub-commands
 */
#define ATA_FEATURE_SET_TRANSFER_MODE   0x03    /**< Set transfer mode */
#define ATA_FEATURE_ENABLE_WCACHE       0x02    /**< Enable write cache */
#define ATA_FEATURE_DISABLE_WCACHE      0x82    /**< Disable write cache */
#define ATA_FEATURE_ENABLE_LOOKAHEAD    0xAA    /**< Enable read lookahead */
#define ATA_FEATURE_DISABLE_LOOKAHEAD   0x55    /**< Disable read lookahead */

//
// ============================================================================
//                      ATA over Ethernet (AoE) Protocol Support
// ============================================================================
//

/**
 * @brief AoE EtherType
 *
 * AoE uses its own EtherType for Layer 2 Ethernet transmission.
 * This avoids the overhead of TCP/IP, UDP, or iSCSI protocols.
 */
#define AOE_ETHERTYPE                   0x88A2    /**< AoE EtherType */

/**
 * @brief AoE Protocol Version
 */
#define AOE_VERSION_1                   0x01      /**< AoE Version 1 */

/**
 * @brief AoE Command Codes
 *
 * AoE supports two main command types: ATA commands and configuration commands.
 */
typedef enum _AOE_COMMAND {
    AOE_CMD_ISSUE_ATA_COMMAND   = 0x00,     /**< Issue ATA command */
    AOE_CMD_QUERY_CONFIG_INFO   = 0x01,     /**< Query configuration information */
    AOE_CMD_MAC_MASK_LIST       = 0x02,     /**< MAC mask list */
    AOE_CMD_RESERVE_RELEASE     = 0x03,     /**< Reserve/Release */
} AOE_COMMAND;

/**
 * @brief AoE Error Codes
 *
 * These are AoE-specific error codes returned in the Error field of
 * the AoE header. They are distinct from ATA error codes.
 */
typedef enum _AOE_ERROR {
    AOE_ERR_NONE                = 0x00,     /**< No error */
    AOE_ERR_BAD_COMMAND         = 0x01,     /**< Unrecognized command code */
    AOE_ERR_BAD_ARGUMENT        = 0x02,     /**< Bad argument parameter */
    AOE_ERR_DEVICE_UNAVAILABLE  = 0x03,     /**< Device unavailable */
    AOE_ERR_CONFIG_STRING       = 0x04,     /**< Config string present */
    AOE_ERR_UNSUPPORTED_VERSION = 0x05,     /**< Unsupported version */
    AOE_ERR_TARGET_BUSY         = 0x06,     /**< Target is reserved by another host */
} AOE_ERROR;

/**
 * @brief AoE Header Flags
 */
#define AOE_FLAG_RESPONSE           0x08    /**< Response (set by target) */
#define AOE_FLAG_ERROR              0x04    /**< Error flag */
#define AOE_FLAG_WRITE              0x01    /**< Write command (clear = read) */
#define AOE_FLAG_ASYNC              0x02    /**< Async notification */
#define AOE_FLAG_EXTENDED           0x40    /**< Extended command (LBA48) */

/**
 * @brief AoE Configuration Commands
 */
#define AOE_CONFIG_READ             0x00    /**< Read configuration */
#define AOE_CONFIG_TEST             0x01    /**< Test configuration */
#define AOE_CONFIG_TEST_PREFIX      0x02    /**< Test with prefix */
#define AOE_CONFIG_SET              0x03    /**< Set configuration */
#define AOE_CONFIG_FORCE_SET        0x04    /**< Force set configuration */

/**
 * @brief AoE Major/Minor Device Addressing
 *
 * AoE uses a two-level addressing scheme:
 * - Major Address: 16-bit shelf/controller ID (0-65535)
 * - Minor Address: 8-bit slot/device ID (0-255)
 *
 * This allows up to 16,777,216 (65536 × 256) unique devices on a single
 * Layer 2 Ethernet network. The broadcast address 0xFFFF/0xFF is used
 * for device discovery.
 */
#define AOE_MAJOR_BROADCAST         0xFFFF  /**< Broadcast major address */
#define AOE_MINOR_BROADCAST         0xFF    /**< Broadcast minor address */
#define AOE_MAJOR_MIN               0x0000  /**< Minimum major address */
#define AOE_MAJOR_MAX               0xFFFE  /**< Maximum major address */
#define AOE_MINOR_MIN               0x00    /**< Minimum minor address */
#define AOE_MINOR_MAX               0xFE    /**< Maximum minor address */

/**
 * @brief AoE Maximum Transfer Sizes
 *
 * AoE supports both standard Ethernet frames (1500 byte MTU) and
 * jumbo frames (up to 9000 bytes). Jumbo frames significantly improve
 * performance by reducing protocol overhead.
 */
#define AOE_MAX_SECTORS_STANDARD    2       /**< Max sectors (standard MTU 1500) */
#define AOE_MAX_SECTORS_JUMBO       16      /**< Max sectors (jumbo frame 9000) */
#define AOE_MTU_STANDARD            1500    /**< Standard Ethernet MTU */
#define AOE_MTU_JUMBO               9000    /**< Jumbo frame MTU */

/**
 * @brief AoE Header Structure
 *
 * This is the main AoE protocol header that appears after the Ethernet
 * header. All multi-byte fields are in network byte order (big-endian).
 */
#pragma pack(push, 1)
typedef struct _AOE_HEADER {
    UINT8   Ver_Flags;          /**< Version (4 bits) | Flags (4 bits) */
    UINT8   Error;              /**< Error code (AOE_ERROR) */
    UINT16  Major;              /**< Major device address (big-endian) */
    UINT8   Minor;              /**< Minor device address */
    UINT8   Command;            /**< Command code (AOE_COMMAND) */
    UINT32  Tag;                /**< Command tag for request/response matching */
} AOE_HEADER;

/**
 * @brief AoE ATA Command Structure
 *
 * This structure follows the AoE header for AOE_CMD_ISSUE_ATA_COMMAND.
 * It encapsulates a complete ATA command for transmission over Ethernet.
 */
typedef struct _AOE_ATA_COMMAND {
    UINT8   AFlags;             /**< ATA flags (Extended, Write, Async) */
    UINT8   ErrFeature;         /**< Error (read) / Feature (write) */
    UINT8   SectorCount;        /**< Sector count register */
    UINT8   CmdStatus;          /**< ATA Command (write) / Status (read) */
    UINT8   Lba0;               /**< LBA bits 0-7 */
    UINT8   Lba1;               /**< LBA bits 8-15 */
    UINT8   Lba2;               /**< LBA bits 16-23 */
    UINT8   Lba3;               /**< LBA bits 24-31 (LBA28) / HOB (LBA48) */
    UINT8   Lba4;               /**< LBA bits 32-39 (LBA48 only) */
    UINT8   Lba5;               /**< LBA bits 40-47 (LBA48 only) */
    UINT8   Reserved[2];        /**< Reserved */
    // Data follows for write commands
} AOE_ATA_COMMAND;

/**
 * @brief AoE Configuration String Structure
 *
 * Used for configuration commands to query and set device parameters.
 */
typedef struct _AOE_CONFIG_COMMAND {
    UINT16  BufferCount;        /**< Number of buffers (big-endian) */
    UINT16  FirmwareVersion;    /**< Firmware version (big-endian) */
    UINT8   SectorCount;        /**< Sectors per packet */
    UINT8   AoeCCmdVersion;     /**< AoE Config Command version */
    UINT16  ConfigStringLen;    /**< Config string length (big-endian) */
    // Config string follows
} AOE_CONFIG_COMMAND;

/**
 * @brief AoE MAC Mask List Structure
 *
 * Used to control which MAC addresses can access the device.
 * Supports multicast group management for multi-host access control.
 */
typedef struct _AOE_MAC_MASK {
    UINT8   Reserved;           /**< Reserved (must be 0) */
    UINT8   MCmd;               /**< Mask command (read/edit) */
    UINT8   MError;             /**< Mask error */
    UINT8   DirCount;           /**< Number of directive entries */
    // Directive entries follow (each 8 bytes: MAC + mask)
} AOE_MAC_MASK;

/**
 * @brief AoE MAC Mask Directive
 *
 * Individual MAC address mask entry for access control.
 */
typedef struct _AOE_MAC_DIRECTIVE {
    UINT8   Reserved;           /**< Reserved (must be 0) */
    UINT8   MCommand;           /**< Command (none/read/edit) */
    UINT8   MacAddr[6];         /**< MAC address */
    // MAC mask follows (6 bytes)
} AOE_MAC_DIRECTIVE;

/**
 * @brief AoE Reserve/Release Structure
 *
 * Used for multi-host coordination to prevent simultaneous access.
 * Supports SCSI-style reserve/release semantics over Ethernet.
 */
typedef struct _AOE_RESERVE_RELEASE {
    UINT8   RCmd;               /**< Reserve command (set/force/release) */
    UINT8   NMacs;              /**< Number of MAC addresses */
    // MAC addresses follow
} AOE_RESERVE_RELEASE;

#pragma pack(pop)

/**
 * @brief AoE Device Information
 *
 * Extended device information for AoE targets.
 */
typedef struct _AOE_DEVICE_INFO {
    UINT16  Major;              /**< Major device address */
    UINT8   Minor;              /**< Minor device address */
    UINT8   MacAddr[6];         /**< Target MAC address */
    UINT8   bJumboFrames;       /**< Jumbo frames supported */
    UINT16  MaxSectors;         /**< Maximum sectors per request */
    UINT16  MTU;                /**< Maximum transmission unit */
    UINT32  BufferCount;        /**< Number of buffers on target */
    UINT16  FirmwareVersion;    /**< Target firmware version */
    CHAR8   ConfigString[256];  /**< Configuration string */
} AOE_DEVICE_INFO;

/**
 * @brief IIOAoEController interface GUID
 * {F1E2D3C4-B5A6-7980-8F9E-0D1C2B3A4950}
 */
DEFINE_GUID(IID_IIOAoEController,
    0xF1E2D3C4, 0xB5A6, 0x7980, 0x8F, 0x9E, 0x0D, 0x1C, 0x2B, 0x3A, 0x49, 0x50);

/**
 * @brief ATA Identify Device Response (simplified - 256 words)
 */
typedef struct _ATA_IDENTIFY_DATA {
    UINT16  GeneralConfig;                  /**< Word 0: General configuration */
    UINT16  NumCylinders;                   /**< Word 1: Obsolete */
    UINT16  SpecificConfig;                 /**< Word 2: Specific configuration */
    UINT16  NumHeads;                       /**< Word 3: Obsolete */
    UINT16  Retired1[2];                    /**< Words 4-5: Retired */
    UINT16  NumSectorsPerTrack;             /**< Word 6: Obsolete */
    UINT16  VendorUnique1[3];               /**< Words 7-9: Vendor unique */
    CHAR8   SerialNumber[20];               /**< Words 10-19: Serial number */
    UINT16  Retired2[2];                    /**< Words 20-21: Retired */
    UINT16  Obsolete1;                      /**< Word 22: Obsolete */
    CHAR8   FirmwareRevision[8];            /**< Words 23-26: Firmware revision */
    CHAR8   ModelNumber[40];                /**< Words 27-46: Model number */
    UINT16  MaxMultipleSectors;             /**< Word 47: Max sectors per interrupt */
    UINT16  Reserved1;                      /**< Word 48: Reserved */
    UINT16  Capabilities1;                  /**< Word 49: Capabilities */
    UINT16  Capabilities2;                  /**< Word 50: Capabilities */
    UINT16  PioMode;                        /**< Word 51: PIO mode (obsolete) */
    UINT16  Retired3;                       /**< Word 52: Retired */
    UINT16  FieldValidity;                  /**< Word 53: Field validity */
    UINT16  CurrentCylinders;               /**< Word 54: Current cylinders (obsolete) */
    UINT16  CurrentHeads;                   /**< Word 55: Current heads (obsolete) */
    UINT16  CurrentSectors;                 /**< Word 56: Current sectors (obsolete) */
    UINT16  CurrentCapacityLow;             /**< Word 57: Current capacity low */
    UINT16  CurrentCapacityHigh;            /**< Word 58: Current capacity high */
    UINT16  MultipleSectorSetting;          /**< Word 59: Multiple sector setting */
    UINT32  TotalAddressableSectors;        /**< Words 60-61: Total addressable sectors (28-bit) */
    UINT16  Retired4;                       /**< Word 62: Retired */
    UINT16  MultiWordDmaMode;               /**< Word 63: Multiword DMA modes */
    UINT16  PioModesSupported;              /**< Word 64: PIO modes supported */
    UINT16  MinMultiWordDmaTransferTime;    /**< Word 65: Min multiword DMA time */
    UINT16  RecommendedMultiWordDmaTime;    /**< Word 66: Recommended DMA time */
    UINT16  MinPioTransferNoFlow;           /**< Word 67: Min PIO time without flow control */
    UINT16  MinPioTransferWithFlow;         /**< Word 68: Min PIO time with flow control */
    UINT16  Reserved2[6];                   /**< Words 69-74: Reserved */
    UINT16  QueueDepth;                     /**< Word 75: Queue depth */
    UINT16  SerialAtaCapabilities;          /**< Word 76: SATA capabilities */
    UINT16  SerialAtaReserved;              /**< Word 77: SATA reserved */
    UINT16  SerialAtaFeaturesSupported;     /**< Word 78: SATA features supported */
    UINT16  SerialAtaFeaturesEnabled;       /**< Word 79: SATA features enabled */
    UINT16  MajorVersion;                   /**< Word 80: Major version number */
    UINT16  MinorVersion;                   /**< Word 81: Minor version number */
    UINT16  CommandSet1;                    /**< Word 82: Command set supported */
    UINT16  CommandSet2;                    /**< Word 83: Command sets supported */
    UINT16  CommandSetExtension;            /**< Word 84: Command set/feature extension */
    UINT16  CommandSetEnabled1;             /**< Word 85: Command set enabled */
    UINT16  CommandSetEnabled2;             /**< Word 86: Command set enabled */
    UINT16  CommandSetDefault;              /**< Word 87: Command set default */
    UINT16  UltraDmaMode;                   /**< Word 88: Ultra DMA modes */
    UINT16  EraseTime;                      /**< Word 89: Security erase time */
    UINT16  EnhancedEraseTime;              /**< Word 90: Enhanced security erase time */
    UINT16  CurrentAPMValue;                /**< Word 91: Current APM value */
    UINT16  MasterPasswordRevision;         /**< Word 92: Master password revision */
    UINT16  HardwareResetResult;            /**< Word 93: Hardware reset result */
    UINT16  AcousticValue;                  /**< Word 94: Acoustic management */
    UINT16  StreamMinRequestSize;           /**< Word 95: Stream minimum request size */
    UINT16  StreamTransferTimeDMA;          /**< Word 96: Stream transfer time DMA */
    UINT16  StreamAccessLatency;            /**< Word 97: Stream access latency */
    UINT32  StreamPerformanceGranularity;   /**< Words 98-99: Stream performance */
    UINT64  TotalAddressableSectors48;      /**< Words 100-103: Total sectors (48-bit) */
    UINT16  StreamTransferTimePIO;          /**< Word 104: Stream transfer time PIO */
    UINT16  Reserved3;                      /**< Word 105: Reserved */
    UINT16  PhysicalSectorSize;             /**< Word 106: Physical sector size */
    UINT16  InterSeekDelay;                 /**< Word 107: Inter-seek delay */
    UINT16  WorldWideName[4];               /**< Words 108-111: World wide name */
    UINT16  Reserved4[4];                   /**< Words 112-115: Reserved */
    UINT16  Reserved5;                      /**< Word 116: Reserved */
    UINT32  LogicalSectorSize;              /**< Words 117-118: Logical sector size */
    UINT16  CommandSet3;                    /**< Word 119: Command set supported */
    UINT16  CommandSetEnabled3;             /**< Word 120: Command set enabled */
    UINT16  Reserved6[6];                   /**< Words 121-126: Reserved */
    UINT16  RemovableMediaStatus;           /**< Word 127: Removable media status */
    UINT16  SecurityStatus;                 /**< Word 128: Security status */
    UINT16  VendorSpecific[31];             /**< Words 129-159: Vendor specific */
    UINT16  CfaPowerMode;                   /**< Word 160: CFA power mode */
    UINT16  Reserved7[15];                  /**< Words 161-175: Reserved */
    UINT16  MediaSerialNumber[30];          /**< Words 176-205: Media serial number */
    UINT16  Reserved8[49];                  /**< Words 206-254: Reserved */
    UINT16  IntegrityWord;                  /**< Word 255: Integrity word */
} ATA_IDENTIFY_DATA;

/**
 * @brief ATA Device Information
 */
typedef struct _ATA_DEVICE_INFO {
    ATA_DEVICE_TYPE DeviceType;             /**< Device type */
    ATA_PROTOCOL    Protocol;               /**< Protocol version */
    BOOLEAN         bAtapi;                 /**< ATAPI device */
    BOOLEAN         bMaster;                /**< Master (0) or Slave (1) */

    // Identification
    CHAR8           Model[41];              /**< Model string (null-terminated) */
    CHAR8           SerialNumber[21];       /**< Serial number (null-terminated) */
    CHAR8           FirmwareRevision[9];    /**< Firmware revision (null-terminated) */

    // Capacity
    UINT64          TotalSectors;           /**< Total sectors (28/48-bit LBA) */
    UINT64          TotalCapacity;          /**< Total capacity in bytes */
    UINT32          SectorSize;             /**< Sector size in bytes (usually 512) */
    UINT32          PhysicalSectorSize;     /**< Physical sector size */

    // Capabilities
    BOOLEAN         bLBA;                   /**< LBA mode supported */
    BOOLEAN         bLBA48;                 /**< 48-bit LBA supported */
    BOOLEAN         bDMA;                   /**< DMA supported */
    BOOLEAN         bUDMA;                  /**< Ultra DMA supported */
    BOOLEAN         bSMART;                 /**< S.M.A.R.T. supported */
    BOOLEAN         bWriteCache;            /**< Write cache present */
    BOOLEAN         bReadAhead;             /**< Read look-ahead supported */
    BOOLEAN         bRemovable;             /**< Removable media */
    BOOLEAN         bSecuritySupported;     /**< Security feature supported */
    BOOLEAN         bSecurityEnabled;       /**< Security enabled */
    BOOLEAN         bSecurityLocked;        /**< Security locked */

    // Transfer Modes
    ATA_PIO_MODE    MaxPioMode;             /**< Maximum PIO mode */
    ATA_MWDMA_MODE  MaxMwDmaMode;           /**< Maximum multiword DMA mode */
    ATA_UDMA_MODE   MaxUdmaMode;            /**< Maximum Ultra DMA mode */
    ATA_PIO_MODE    CurrentPioMode;         /**< Current PIO mode */
    ATA_MWDMA_MODE  CurrentMwDmaMode;       /**< Current multiword DMA mode */
    ATA_UDMA_MODE   CurrentUdmaMode;        /**< Current Ultra DMA mode */
    UINT32          MultiSectorCount;       /**< Multiple sector count */

    // Geometry (CHS - obsolete but still supported)
    UINT32          Cylinders;              /**< Number of cylinders */
    UINT32          Heads;                  /**< Number of heads */
    UINT32          SectorsPerTrack;        /**< Sectors per track */
} ATA_DEVICE_INFO;

/**
 * @brief ATA Controller Information
 */
typedef struct _ATA_CONTROLLER_INFO {
    ATA_PROTOCOL    Protocol;               /**< Maximum protocol supported */
    CHAR8           ControllerName[64];     /**< Controller name */
    CHAR8           VendorName[40];         /**< Vendor name */

    UINT16          VendorID;               /**< PCI Vendor ID */
    UINT16          DeviceID;               /**< PCI Device ID */
    UINT32          NumChannels;            /**< Number of ATA channels */
    UINT32          NumDevicesPerChannel;   /**< Devices per channel (usually 2) */

    BOOLEAN         bDMASupported;          /**< DMA support */
    BOOLEAN         bUDMASupported;         /**< Ultra DMA support */
    BOOLEAN         bCableDetect;           /**< 80-wire cable detection */
    BOOLEAN         b80WireCable;           /**< 80-wire cable present */

    ATA_PIO_MODE    MaxPioMode;             /**< Maximum PIO mode */
    ATA_MWDMA_MODE  MaxMwDmaMode;           /**< Maximum MWDMA mode */
    ATA_UDMA_MODE   MaxUdmaMode;            /**< Maximum UDMA mode */

    UINT32          MaxTransferSize;        /**< Maximum transfer size (bytes) */
} ATA_CONTROLLER_INFO;

/**
 * @brief ATA Command Structure
 */
typedef struct _ATA_COMMAND {
    UINT8       Command;                    /**< Command code */
    UINT8       Features;                   /**< Features register */
    UINT8       SectorCount;                /**< Sector count */
    UINT8       SectorNumber;               /**< Sector number / LBA low */
    UINT8       CylinderLow;                /**< Cylinder low / LBA mid */
    UINT8       CylinderHigh;               /**< Cylinder high / LBA high */
    UINT8       DeviceHead;                 /**< Device/head register */

    // For 48-bit LBA commands
    UINT8       FeaturesExt;                /**< Features ext (HOB) */
    UINT8       SectorCountExt;             /**< Sector count ext (HOB) */
    UINT8       LbaLowExt;                  /**< LBA low ext (HOB) */
    UINT8       LbaMidExt;                  /**< LBA mid ext (HOB) */
    UINT8       LbaHighExt;                 /**< LBA high ext (HOB) */

    VOID       *pDataBuffer;                /**< Data buffer */
    UINT32      DataLength;                 /**< Data length in bytes */
    BOOLEAN     bDataIn;                    /**< TRUE for read, FALSE for write */
    UINT32      TimeoutMs;                  /**< Timeout in milliseconds */

    UINT8       Status;                     /**< Command status */
    UINT8       Error;                      /**< Error register value */
} ATA_COMMAND;

/**
 * @brief Known ATA/IDE Controller Vendors
 */
#define ATA_VENDOR_INTEL            0x8086  /**< Intel */
#define ATA_VENDOR_VIA              0x1106  /**< VIA Technologies */
#define ATA_VENDOR_AMD              0x1022  /**< AMD */
#define ATA_VENDOR_ATI              0x1002  /**< ATI/AMD */
#define ATA_VENDOR_SIS              0x1039  /**< SiS */
#define ATA_VENDOR_ALI              0x10B9  /**< ALi (Acer Labs) */
#define ATA_VENDOR_NVIDIA           0x10DE  /**< NVIDIA */
#define ATA_VENDOR_PROMISE          0x105A  /**< Promise Technology */
#define ATA_VENDOR_HIGHPOINT        0x1103  /**< HighPoint Technologies */
#define ATA_VENDOR_CYRIX            0x1078  /**< Cyrix/National Semi */
#define ATA_VENDOR_SERVERWORKS      0x1166  /**< ServerWorks/Broadcom */
#define ATA_VENDOR_CMD              0x1095  /**< CMD Technology */
#define ATA_VENDOR_OPTI             0x1045  /**< OPTi */
#define ATA_VENDOR_ITE              0x1283  /**< ITE Tech */
#define ATA_VENDOR_JMICRON          0x197B  /**< JMicron */

/**
 * @brief IIOATAController - ATA Controller interface
 *
 * This interface represents an ATA/IDE host adapter and provides methods
 * for device enumeration, command submission, and bus management.
 */
#undef INTERFACE
#define INTERFACE IIOATAController

DECLARE_INTERFACE_(IIOATAController, IIOService)
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

    // IIOATAController methods

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
        ATA_CONTROLLER_INFO *pControllerInfo
        ) PURE;

    /**
     * @brief Get device count
     *
     * Returns the number of discovered ATA devices.
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
     * Retrieves the device interface for a specific channel and device.
     *
     * @param uChannel          Channel number (0-based)
     * @param uDevice           Device number (0=Master, 1=Slave)
     * @param ppDevice          Receives device interface
     *
     * @retval IO_SUCCESS       Device interface retrieved
     * @retval IO_NO_DEVICE     Device not found
     */
    STDMETHOD_(IO_RETURN, GetDevice)(THIS_
        UINT32 uChannel,
        UINT32 uDevice,
        IIOATADevice **ppDevice
        ) PURE;

    /**
     * @brief Reset ATA channel
     *
     * Performs a software reset on the specified ATA channel.
     *
     * @param uChannel          Channel number
     *
     * @retval IO_SUCCESS       Reset successful
     * @retval IO_ERROR         Reset failed
     */
    STDMETHOD_(IO_RETURN, ResetChannel)(THIS_
        UINT32 uChannel
        ) PURE;

    /**
     * @brief Reset ATA device
     *
     * Performs a device reset operation.
     *
     * @param uChannel          Channel number
     * @param uDevice           Device number (0=Master, 1=Slave)
     *
     * @retval IO_SUCCESS       Reset successful
     * @retval IO_ERROR         Reset failed
     */
    STDMETHOD_(IO_RETURN, ResetDevice)(THIS_
        UINT32 uChannel,
        UINT32 uDevice
        ) PURE;

    /**
     * @brief Scan bus for devices
     *
     * Scans all ATA channels and enumerates devices.
     *
     * @retval IO_SUCCESS       Scan complete
     */
    STDMETHOD_(IO_RETURN, ScanBus)(THIS) PURE;

    /**
     * @brief Submit ATA command
     *
     * Submits a raw ATA command to a device.
     *
     * @param uChannel          Channel number
     * @param uDevice           Device number (0=Master, 1=Slave)
     * @param pCommand          ATA command structure
     *
     * @retval IO_SUCCESS       Command completed successfully
     * @retval IO_ERROR         Command failed
     */
    STDMETHOD_(IO_RETURN, SubmitCommand)(THIS_
        UINT32 uChannel,
        UINT32 uDevice,
        ATA_COMMAND *pCommand
        ) PURE;

    /**
     * @brief Set transfer mode
     *
     * Sets the transfer mode for a device.
     *
     * @param uChannel          Channel number
     * @param uDevice           Device number
     * @param TransferType      Transfer type (PIO/MWDMA/UDMA)
     * @param uMode             Mode number
     *
     * @retval IO_SUCCESS       Transfer mode set
     * @retval IO_UNSUPPORTED   Mode not supported
     */
    STDMETHOD_(IO_RETURN, SetTransferMode)(THIS_
        UINT32 uChannel,
        UINT32 uDevice,
        ATA_TRANSFER_TYPE TransferType,
        UINT32 uMode
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOATADevice - ATA Device interface
 *
 * This interface represents an ATA device and provides methods
 * for standard ATA operations.
 */
#undef INTERFACE
#define INTERFACE IIOATADevice

DECLARE_INTERFACE_(IIOATADevice, IIOService)
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
        ATA_DEVICE_INFO *pDeviceInfo
        ) PURE;

    /**
     * @brief Read sectors
     *
     * Reads sectors from the device.
     *
     * @param uLBA              Starting LBA
     * @param uSectors          Number of sectors to read
     * @param pBuffer           Buffer to receive data
     * @param cbBuffer          Buffer size in bytes
     *
     * @retval IO_SUCCESS       Read successful
     * @retval IO_ERROR         Read failed
     */
    STDMETHOD_(IO_RETURN, ReadSectors)(THIS_
        UINT64 uLBA,
        UINT32 uSectors,
        VOID *pBuffer,
        UINTN cbBuffer
        ) PURE;

    /**
     * @brief Write sectors
     *
     * Writes sectors to the device.
     *
     * @param uLBA              Starting LBA
     * @param uSectors          Number of sectors to write
     * @param pBuffer           Buffer containing data
     * @param cbBuffer          Buffer size in bytes
     *
     * @retval IO_SUCCESS       Write successful
     * @retval IO_ERROR         Write failed
     */
    STDMETHOD_(IO_RETURN, WriteSectors)(THIS_
        UINT64 uLBA,
        UINT32 uSectors,
        CONST VOID *pBuffer,
        UINTN cbBuffer
        ) PURE;

    /**
     * @brief Execute ATA command
     *
     * Executes an ATA command on this device.
     *
     * @param pCommand          ATA command structure
     *
     * @retval IO_SUCCESS       Command successful
     * @retval IO_ERROR         Command failed
     */
    STDMETHOD_(IO_RETURN, ExecuteCommand)(THIS_
        ATA_COMMAND *pCommand
        ) PURE;

    /**
     * @brief Identify device
     *
     * Executes IDENTIFY DEVICE command and returns data.
     *
     * @param pIdentifyData     Receives identify data
     *
     * @retval IO_SUCCESS       Identify successful
     * @retval IO_ERROR         Identify failed
     */
    STDMETHOD_(IO_RETURN, IdentifyDevice)(THIS_
        ATA_IDENTIFY_DATA *pIdentifyData
        ) PURE;

    /**
     * @brief Flush cache
     *
     * Flushes the device's write cache.
     *
     * @retval IO_SUCCESS       Cache flushed
     * @retval IO_ERROR         Flush failed
     */
    STDMETHOD_(IO_RETURN, FlushCache)(THIS) PURE;

    /**
     * @brief Set transfer mode
     *
     * Sets the device transfer mode.
     *
     * @param TransferType      Transfer type
     * @param uMode             Mode number
     *
     * @retval IO_SUCCESS       Transfer mode set
     * @retval IO_UNSUPPORTED   Mode not supported
     */
    STDMETHOD_(IO_RETURN, SetTransferMode)(THIS_
        ATA_TRANSFER_TYPE TransferType,
        UINT32 uMode
        ) PURE;

    /**
     * @brief Execute ATAPI packet command
     *
     * Executes an ATAPI packet command (for CD/DVD/Zip drives).
     *
     * @param pPacket           ATAPI packet (12 or 16 bytes)
     * @param uPacketLength     Packet length
     * @param pBuffer           Data buffer
     * @param cbBuffer          Buffer size
     * @param bDataIn           TRUE for read, FALSE for write
     *
     * @retval IO_SUCCESS       Command successful
     * @retval IO_ERROR         Command failed
     * @retval IO_UNSUPPORTED   Not an ATAPI device
     */
    STDMETHOD_(IO_RETURN, ExecutePacket)(THIS_
        CONST VOID *pPacket,
        UINT32 uPacketLength,
        VOID *pBuffer,
        UINT32 cbBuffer,
        BOOLEAN bDataIn
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOAoEController - AoE Initiator Controller interface
 *
 * This interface represents an AoE initiator (client) and provides methods
 * for discovering AoE targets, establishing connections, and performing
 * block I/O operations over Layer 2 Ethernet.
 */
#undef INTERFACE
#define INTERFACE IIOAoEController

DECLARE_INTERFACE_(IIOAoEController, IIOService)
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

    // IIOAoEController methods

    /**
     * @brief Discover AoE targets on network
     *
     * Sends AoE discovery broadcasts and enumerates all responding targets.
     * Uses the broadcast address (major=0xFFFF, minor=0xFF) to discover
     * all available AoE devices on the Layer 2 network.
     *
     * @param uTimeoutMs        Discovery timeout in milliseconds
     *
     * @retval IO_SUCCESS       Discovery completed successfully
     * @retval IO_TIMEOUT       Discovery timed out
     * @retval IO_ERROR         Discovery failed
     */
    STDMETHOD_(IO_RETURN, DiscoverTargets)(THIS_
        UINT32 uTimeoutMs
        ) PURE;

    /**
     * @brief Get discovered target count
     *
     * Returns the number of AoE targets discovered on the network.
     *
     * @param puCount           Receives target count
     *
     * @retval IO_SUCCESS       Count retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetTargetCount)(THIS_
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Get target information
     *
     * Retrieves detailed information about a discovered AoE target.
     *
     * @param uIndex            Target index (0-based)
     * @param pTargetInfo       Receives target information
     *
     * @retval IO_SUCCESS       Information retrieved
     * @retval IO_BAD_ARGUMENT  Invalid index
     */
    STDMETHOD_(IO_RETURN, GetTargetInfo)(THIS_
        UINT32 uIndex,
        AOE_DEVICE_INFO *pTargetInfo
        ) PURE;

    /**
     * @brief Connect to AoE target
     *
     * Establishes a connection to a specific AoE target by major/minor address.
     * Queries the target configuration and prepares for I/O operations.
     *
     * @param uMajor            Major device address
     * @param uMinor            Minor device address
     * @param ppDevice          Receives ATA device interface for target
     *
     * @retval IO_SUCCESS       Connection established
     * @retval IO_NO_DEVICE     Target not found
     * @retval IO_BUSY          Target reserved by another host
     */
    STDMETHOD_(IO_RETURN, ConnectTarget)(THIS_
        UINT16 uMajor,
        UINT8 uMinor,
        IIOATADevice **ppDevice
        ) PURE;

    /**
     * @brief Disconnect from AoE target
     *
     * Closes the connection to an AoE target and releases resources.
     *
     * @param uMajor            Major device address
     * @param uMinor            Minor device address
     *
     * @retval IO_SUCCESS       Disconnected successfully
     */
    STDMETHOD_(IO_RETURN, DisconnectTarget)(THIS_
        UINT16 uMajor,
        UINT8 uMinor
        ) PURE;

    /**
     * @brief Query target configuration
     *
     * Retrieves configuration information from an AoE target including
     * buffer count, firmware version, and configuration string.
     *
     * @param uMajor            Major device address
     * @param uMinor            Minor device address
     * @param pConfig           Receives configuration data
     *
     * @retval IO_SUCCESS       Configuration retrieved
     * @retval IO_NO_DEVICE     Target not found
     */
    STDMETHOD_(IO_RETURN, QueryConfig)(THIS_
        UINT16 uMajor,
        UINT8 uMinor,
        AOE_DEVICE_INFO *pConfig
        ) PURE;

    /**
     * @brief Reserve AoE target
     *
     * Reserves an AoE target for exclusive access by this initiator.
     * Prevents other hosts from accessing the target until released.
     *
     * @param uMajor            Major device address
     * @param uMinor            Minor device address
     * @param bForce            Force reservation (override existing)
     *
     * @retval IO_SUCCESS       Target reserved
     * @retval IO_BUSY          Target already reserved
     * @retval IO_NO_DEVICE     Target not found
     */
    STDMETHOD_(IO_RETURN, ReserveTarget)(THIS_
        UINT16 uMajor,
        UINT8 uMinor,
        BOOLEAN bForce
        ) PURE;

    /**
     * @brief Release AoE target
     *
     * Releases a previously reserved target, allowing other hosts to access it.
     *
     * @param uMajor            Major device address
     * @param uMinor            Minor device address
     *
     * @retval IO_SUCCESS       Target released
     */
    STDMETHOD_(IO_RETURN, ReleaseTarget)(THIS_
        UINT16 uMajor,
        UINT8 uMinor
        ) PURE;

    /**
     * @brief Set jumbo frame support
     *
     * Enables or disables jumbo frame support for improved performance.
     * Jumbo frames (up to 9000 bytes) significantly reduce overhead by
     * allowing more sectors per packet.
     *
     * @param bEnable           TRUE to enable jumbo frames
     * @param uMTU              MTU size (1500 or 9000)
     *
     * @retval IO_SUCCESS       Jumbo frame setting updated
     * @retval IO_UNSUPPORTED   Jumbo frames not supported
     */
    STDMETHOD_(IO_RETURN, SetJumboFrames)(THIS_
        BOOLEAN bEnable,
        UINT16 uMTU
        ) PURE;

    /**
     * @brief Submit AoE ATA command
     *
     * Submits a raw ATA command encapsulated in AoE protocol.
     * This is a low-level interface for direct ATA command access.
     *
     * @param uMajor            Major device address
     * @param uMinor            Minor device address
     * @param pATACommand       ATA command to execute
     * @param pData             Data buffer (for read/write)
     * @param cbData            Data buffer size
     * @param bWrite            TRUE for write, FALSE for read
     * @param puTag             Receives command tag
     *
     * @retval IO_SUCCESS       Command submitted
     * @retval IO_ERROR         Submission failed
     */
    STDMETHOD_(IO_RETURN, SubmitAoECommand)(THIS_
        UINT16 uMajor,
        UINT8 uMinor,
        CONST ATA_COMMAND *pATACommand,
        VOID *pData,
        UINT32 cbData,
        BOOLEAN bWrite,
        UINT32 *puTag
        ) PURE;

    /**
     * @brief Get network statistics
     *
     * Retrieves network performance statistics for AoE operations.
     *
     * @param puPacketsSent     Receives packets sent count
     * @param puPacketsReceived Receives packets received count
     * @param puBytesTransferred Receives total bytes transferred
     * @param puErrors          Receives error count
     *
     * @retval IO_SUCCESS       Statistics retrieved
     */
    STDMETHOD_(IO_RETURN, GetStatistics)(THIS_
        UINT64 *puPacketsSent,
        UINT64 *puPacketsReceived,
        UINT64 *puBytesTransferred,
        UINT32 *puErrors
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOATAController methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOATAController_GetControllerInfo(p,a)     (p)->lpVtbl->GetControllerInfo(p,a)
#define IIOATAController_GetDeviceCount(p,a)        (p)->lpVtbl->GetDeviceCount(p,a)
#define IIOATAController_GetDevice(p,a,b,c)         (p)->lpVtbl->GetDevice(p,a,b,c)
#define IIOATAController_ResetChannel(p,a)          (p)->lpVtbl->ResetChannel(p,a)
#define IIOATAController_ResetDevice(p,a,b)         (p)->lpVtbl->ResetDevice(p,a,b)
#define IIOATAController_ScanBus(p)                 (p)->lpVtbl->ScanBus(p)
#define IIOATAController_SubmitCommand(p,a,b,c)     (p)->lpVtbl->SubmitCommand(p,a,b,c)
#define IIOATAController_SetTransferMode(p,a,b,c,d) (p)->lpVtbl->SetTransferMode(p,a,b,c,d)

#define IIOATADevice_GetDeviceInfo(p,a)             (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOATADevice_ReadSectors(p,a,b,c,d)         (p)->lpVtbl->ReadSectors(p,a,b,c,d)
#define IIOATADevice_WriteSectors(p,a,b,c,d)        (p)->lpVtbl->WriteSectors(p,a,b,c,d)
#define IIOATADevice_ExecuteCommand(p,a)            (p)->lpVtbl->ExecuteCommand(p,a)
#define IIOATADevice_IdentifyDevice(p,a)            (p)->lpVtbl->IdentifyDevice(p,a)
#define IIOATADevice_FlushCache(p)                  (p)->lpVtbl->FlushCache(p)
#define IIOATADevice_SetTransferMode(p,a,b)         (p)->lpVtbl->SetTransferMode(p,a,b)
#define IIOATADevice_ExecutePacket(p,a,b,c,d,e)     (p)->lpVtbl->ExecutePacket(p,a,b,c,d,e)

#define IIOAoEController_DiscoverTargets(p,a)       (p)->lpVtbl->DiscoverTargets(p,a)
#define IIOAoEController_GetTargetCount(p,a)        (p)->lpVtbl->GetTargetCount(p,a)
#define IIOAoEController_GetTargetInfo(p,a,b)       (p)->lpVtbl->GetTargetInfo(p,a,b)
#define IIOAoEController_ConnectTarget(p,a,b,c)     (p)->lpVtbl->ConnectTarget(p,a,b,c)
#define IIOAoEController_DisconnectTarget(p,a,b)    (p)->lpVtbl->DisconnectTarget(p,a,b)
#define IIOAoEController_QueryConfig(p,a,b,c)       (p)->lpVtbl->QueryConfig(p,a,b,c)
#define IIOAoEController_ReserveTarget(p,a,b,c)     (p)->lpVtbl->ReserveTarget(p,a,b,c)
#define IIOAoEController_ReleaseTarget(p,a,b)       (p)->lpVtbl->ReleaseTarget(p,a,b)
#define IIOAoEController_SetJumboFrames(p,a,b)      (p)->lpVtbl->SetJumboFrames(p,a,b)
#define IIOAoEController_SubmitAoECommand(p,a,b,c,d,e,f,g) (p)->lpVtbl->SubmitAoECommand(p,a,b,c,d,e,f,g)
#define IIOAoEController_GetStatistics(p,a,b,c,d)   (p)->lpVtbl->GetStatistics(p,a,b,c,d)

#endif

/**
 * @brief Initialize ATA/IDE family driver
 *
 * Initializes the ATA/IDE family driver and registers controller classes.
 *
 * @retval IO_SUCCESS   Initialization successful
 * @retval IO_ERROR     Initialization failed
 */
IO_RETURN
ATAInitialize(
    VOID
    );

/**
 * @brief Shutdown ATA/IDE family driver
 *
 * Shuts down the ATA family driver and releases resources.
 *
 * @retval IO_SUCCESS   Shutdown successful
 */
IO_RETURN
ATAShutdown(
    VOID
    );

/**
 * @brief Create ATA controller instance
 *
 * Creates an ATA controller interface for a PCI device.
 *
 * @param pPCIDevice        PCI device interface
 * @param ppController      Receives controller interface
 *
 * @retval IO_SUCCESS       Controller created successfully
 * @retval IO_NO_MEMORY     Insufficient memory
 * @retval IO_NO_DEVICE     Not an ATA controller
 */
IO_RETURN
ATAControllerCreate(
    IIOService *pPCIDevice,
    IIOATAController **ppController
    );

/**
 * @brief Create AoE initiator controller instance
 *
 * Creates an AoE initiator controller interface for a network device.
 * This allows discovery and connection to AoE targets on the network.
 *
 * @param pNetworkDevice    Network device interface (Ethernet)
 * @param ppController      Receives AoE controller interface
 *
 * @retval IO_SUCCESS       Controller created successfully
 * @retval IO_NO_MEMORY     Insufficient memory
 * @retval IO_NO_DEVICE     Not a suitable network device
 * @retval IO_UNSUPPORTED   Network device doesn't support raw Ethernet
 */
IO_RETURN
AoEControllerCreate(
    IIOService *pNetworkDevice,
    IIOAoEController **ppController
    );

/**
 * @brief Helper: Convert AoE major/minor to string
 *
 * Converts AoE device address to a human-readable string format.
 *
 * @param uMajor            Major address
 * @param uMinor            Minor address
 * @param pszBuffer         Output buffer
 * @param cbBuffer          Buffer size
 *
 * @return Number of characters written
 */
UINT32
AoEAddressToString(
    UINT16 uMajor,
    UINT8 uMinor,
    CHAR8 *pszBuffer,
    UINTN cbBuffer
    );

/**
 * @brief Helper: Build AoE header
 *
 * Constructs an AoE protocol header for transmission.
 *
 * @param pHeader           Output header structure
 * @param uCommand          AoE command code
 * @param uMajor            Major device address
 * @param uMinor            Minor device address
 * @param uTag              Command tag
 * @param uFlags            Header flags
 */
VOID
AoEBuildHeader(
    AOE_HEADER *pHeader,
    AOE_COMMAND uCommand,
    UINT16 uMajor,
    UINT8 uMinor,
    UINT32 uTag,
    UINT8 uFlags
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_ATA_H */
