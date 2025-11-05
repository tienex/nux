/**
 * @file nubus.h
 * @brief NuBus Family Interface - Apple Macintosh Expansion Bus
 *
 * This header defines the NuBus family interface for Apple's expansion
 * bus architecture used in Macintosh II series computers (1987-1995).
 *
 * NuBus is a 32-bit synchronous bus with:
 * - 10 MB/s transfer rate
 * - Auto-configuration via Declaration ROM
 * - 6 standard slots (numbered $9-$E in hexadecimal)
 * - sResource directory structure for device information
 * - Block transfer mode for improved performance
 *
 * Supported systems:
 * - Macintosh II, IIx, IIcx, IIci, IIsi, IIfx
 * - Macintosh Quadra 700, 900, 950
 * - Macintosh Centris 610, 650
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_NUBUS_H
#define IOKIT_NUBUS_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Interface GUIDs
//=============================================================================

/**
 * @brief IIONuBusBus interface GUID
 * {A0B1C2D3-E4F5-6A7B-8C9D-0E1F2A3B4C5D}
 */
DEFINE_GUID(IID_IIONuBusBus,
    0xA0B1C2D3, 0xE4F5, 0x6A7B, 0x8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D);

/**
 * @brief IIONuBusDevice interface GUID
 * {B1C2D3E4-F5A6-7B8C-9D0E-1F2A3B4C5D6E}
 */
DEFINE_GUID(IID_IIONuBusDevice,
    0xB1C2D3E4, 0xF5A6, 0x7B8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E);

//=============================================================================
// NuBus Constants
//=============================================================================

/**
 * @brief NuBus slot numbers
 */
#define NUBUS_SLOT_MIN      0x9     /**< First NuBus slot ($9) */
#define NUBUS_SLOT_MAX      0xE     /**< Last NuBus slot ($E) */
#define NUBUS_SLOT_COUNT    6       /**< Number of standard slots */

/**
 * @brief NuBus address space
 */
#define NUBUS_SLOT_SIZE     0x01000000  /**< 16 MB per slot */
#define NUBUS_BASE_ADDR     0xF9000000  /**< Base address for slot $9 */

/**
 * @brief Declaration ROM magic values
 */
#define NUBUS_DROM_TEST_PATTERN     0x5A932BC7  /**< Test pattern in Declaration ROM */
#define NUBUS_DROM_BYTE_LANE_MASK   0x0F        /**< Byte lane mask */

/**
 * @brief sResource types
 */
#define NUBUS_SRESOURCE_BOARD       0x01    /**< Board sResource */
#define NUBUS_SRESOURCE_NAME        0x02    /**< Name sResource */
#define NUBUS_SRESOURCE_ICON        0x03    /**< Icon sResource */
#define NUBUS_SRESOURCE_DRIVER      0x04    /**< Driver sResource */
#define NUBUS_SRESOURCE_LOAD        0x05    /**< Load rec sResource */
#define NUBUS_SRESOURCE_EXEC        0x06    /**< Exec block sResource */
#define NUBUS_SRESOURCE_OS          0x07    /**< OS override sResource */
#define NUBUS_SRESOURCE_VENDOR      0x08    /**< Vendor info sResource */
#define NUBUS_SRESOURCE_FLAGS       0x09    /**< Flags sResource */
#define NUBUS_SRESOURCE_HW_DEV_ID   0x0A    /**< Hardware device ID sResource */

/**
 * @brief Card categories
 */
typedef enum _NUBUS_CATEGORY {
    NUBUS_CAT_DISPLAY       = 0x0001,   /**< Display card */
    NUBUS_CAT_NETWORK       = 0x0004,   /**< Network card */
    NUBUS_CAT_COMMUNICATION = 0x0006,   /**< Communications card */
    NUBUS_CAT_MEMORY        = 0x0008,   /**< Memory expansion */
    NUBUS_CAT_PROCESSOR     = 0x000A,   /**< CPU accelerator */
    NUBUS_CAT_MASS_STORAGE  = 0x000C,   /**< Disk controller */
} NUBUS_CATEGORY;

/**
 * @brief Card types (subcategories)
 */
typedef enum _NUBUS_TYPE {
    // Display types
    NUBUS_TYPE_VIDEO        = 0x0001,   /**< Video display */
    NUBUS_TYPE_VIDEO_3D     = 0x0002,   /**< 3D accelerator */

    // Network types
    NUBUS_TYPE_ETHERNET     = 0x0001,   /**< Ethernet adapter */
    NUBUS_TYPE_TOKEN_RING   = 0x0002,   /**< Token Ring adapter */

    // Communications types
    NUBUS_TYPE_SERIAL       = 0x0001,   /**< Serial card */
    NUBUS_TYPE_PARALLEL     = 0x0002,   /**< Parallel card */

    // Mass storage types
    NUBUS_TYPE_SCSI         = 0x0001,   /**< SCSI controller */
    NUBUS_TYPE_IDE          = 0x0002,   /**< IDE controller */
} NUBUS_TYPE;

//=============================================================================
// NuBus Structures
//=============================================================================

/**
 * @brief Declaration ROM format entry
 */
typedef struct _NUBUS_DROM_ENTRY {
    UINT8   uByteOffset;        /**< Byte offset (0-3) */
    UINT32  uData;              /**< 32-bit data value */
} NUBUS_DROM_ENTRY;

/**
 * @brief sResource directory entry
 */
typedef struct _NUBUS_SRESOURCE {
    UINT8   uType;              /**< sResource type */
    UINT32  uOffset;            /**< Offset to resource data */
    UINT32  uLength;            /**< Length of resource data */
    VOID    *pData;             /**< Pointer to resource data */
} NUBUS_SRESOURCE;

/**
 * @brief NuBus board information
 */
typedef struct _NUBUS_BOARD_INFO {
    UINT32              uTestPattern;       /**< Test pattern (0x5A932BC7) */
    UINT8               uByteLanes;         /**< Byte lane configuration */
    UINT32              uFormatBlock;       /**< Format block offset */
    UINT16              uCategory;          /**< Board category */
    UINT16              uType;              /**< Board type */
    UINT16              uDriver;            /**< Driver sw resource ID */
    UINT16              uDriverHw;          /**< Driver hw resource ID */
    CHAR8               szName[256];        /**< Board name */
    CHAR8               szVendor[256];      /**< Vendor name */
    CHAR8               szRevision[32];     /**< Revision string */
    CHAR8               szPartNumber[64];   /**< Part number */
    UINT32              uBoardID;           /**< Board ID */
    UINT32              uFlags;             /**< Board flags */
    NUBUS_SRESOURCE     *pResources;        /**< Array of sResources */
    UINT32              uResourceCount;     /**< Number of sResources */
} NUBUS_BOARD_INFO;

/**
 * @brief NuBus slot information
 */
typedef struct _NUBUS_SLOT_INFO {
    UINT8               uSlot;              /**< Slot number ($9-$E) */
    UINT32              uBaseAddress;       /**< Base address in memory */
    BOOLEAN             bCardPresent;       /**< TRUE if card installed */
    BOOLEAN             bEnabled;           /**< TRUE if slot enabled */
    NUBUS_BOARD_INFO    BoardInfo;          /**< Board information */
} NUBUS_SLOT_INFO;

/**
 * @brief Transfer mode
 */
typedef enum _NUBUS_TRANSFER_MODE {
    NUBUS_TRANSFER_STANDARD     = 0,    /**< Standard transfer (10 MB/s) */
    NUBUS_TRANSFER_BLOCK        = 1,    /**< Block transfer mode */
} NUBUS_TRANSFER_MODE;

//=============================================================================
// NuBus Device Database
//=============================================================================

/**
 * @brief Known NuBus card database entry
 */
typedef struct _NUBUS_CARD_DB_ENTRY {
    UINT32          uBoardID;           /**< Board ID */
    UINT16          uCategory;          /**< Category */
    UINT16          uType;              /**< Type */
    CONST CHAR8     *pszVendor;         /**< Vendor name */
    CONST CHAR8     *pszName;           /**< Card name */
    CONST CHAR8     *pszDescription;    /**< Description */
} NUBUS_CARD_DB_ENTRY;

//=============================================================================
// IIONuBusBus Interface
//=============================================================================

/**
 * @brief NuBus bus interface
 */
typedef struct IIONuBusBus {
    IIOService  Base;

    /**
     * @brief Detect installed cards
     */
    IO_RETURN (*DetectCards)(
        struct IIONuBusBus  *this,
        UINT32              *puCardCount
    );

    /**
     * @brief Get slot information
     */
    IO_RETURN (*GetSlotInfo)(
        struct IIONuBusBus  *this,
        UINT8               uSlot,
        NUBUS_SLOT_INFO     *pSlotInfo
    );

    /**
     * @brief Enable/disable slot
     */
    IO_RETURN (*EnableSlot)(
        struct IIONuBusBus  *this,
        UINT8               uSlot,
        BOOLEAN             bEnable
    );

    /**
     * @brief Read Declaration ROM
     */
    IO_RETURN (*ReadDeclarationROM)(
        struct IIONuBusBus  *this,
        UINT8               uSlot,
        UINT32              uOffset,
        VOID                *pBuffer,
        UINT32              uLength
    );

    /**
     * @brief Parse sResource directory
     */
    IO_RETURN (*ParseSResourceDir)(
        struct IIONuBusBus  *this,
        UINT8               uSlot,
        NUBUS_BOARD_INFO    *pBoardInfo
    );

    /**
     * @brief Get sResource data
     */
    IO_RETURN (*GetSResource)(
        struct IIONuBusBus  *this,
        UINT8               uSlot,
        UINT8               uResourceType,
        VOID                *pBuffer,
        UINT32              *puLength
    );

    /**
     * @brief Enumerate all cards
     */
    IO_RETURN (*EnumerateCards)(
        struct IIONuBusBus  *this,
        IIONuBusDevice      ***pppDevices,
        UINT32              *puCount
    );
} IIONuBusBus;

//=============================================================================
// IIONuBusDevice Interface
//=============================================================================

/**
 * @brief NuBus card device interface
 */
typedef struct IIONuBusDevice {
    IIOService  Base;

    /**
     * @brief Get slot number
     */
    IO_RETURN (*GetSlot)(
        struct IIONuBusDevice   *this,
        UINT8                   *puSlot
    );

    /**
     * @brief Get board information
     */
    IO_RETURN (*GetBoardInfo)(
        struct IIONuBusDevice   *this,
        NUBUS_BOARD_INFO        *pBoardInfo
    );

    /**
     * @brief Get base address
     */
    IO_RETURN (*GetBaseAddress)(
        struct IIONuBusDevice   *this,
        UINT32                  *puBaseAddress
    );

    /**
     * @brief Read card memory
     */
    IO_RETURN (*ReadMemory)(
        struct IIONuBusDevice   *this,
        UINT32                  uOffset,
        VOID                    *pBuffer,
        UINT32                  uLength
    );

    /**
     * @brief Write card memory
     */
    IO_RETURN (*WriteMemory)(
        struct IIONuBusDevice   *this,
        UINT32                  uOffset,
        CONST VOID              *pBuffer,
        UINT32                  uLength
    );

    /**
     * @brief Set transfer mode
     */
    IO_RETURN (*SetTransferMode)(
        struct IIONuBusDevice   *this,
        NUBUS_TRANSFER_MODE     eMode
    );

    /**
     * @brief Get sResource
     */
    IO_RETURN (*GetResource)(
        struct IIONuBusDevice   *this,
        UINT8                   uResourceType,
        VOID                    *pBuffer,
        UINT32                  *puLength
    );

    /**
     * @brief Enable card
     */
    IO_RETURN (*Enable)(
        struct IIONuBusDevice   *this
    );

    /**
     * @brief Disable card
     */
    IO_RETURN (*Disable)(
        struct IIONuBusDevice   *this
    );
} IIONuBusDevice;

//=============================================================================
// Helper Macros
//=============================================================================

/**
 * @brief Calculate NuBus slot base address
 */
#define NUBUS_SLOT_TO_ADDRESS(slot) \
    (NUBUS_BASE_ADDR + ((slot) - NUBUS_SLOT_MIN) * NUBUS_SLOT_SIZE)

/**
 * @brief Check if slot number is valid
 */
#define NUBUS_SLOT_IS_VALID(slot) \
    ((slot) >= NUBUS_SLOT_MIN && (slot) <= NUBUS_SLOT_MAX)

/**
 * @brief Make board ID from category and type
 */
#define NUBUS_MAKE_BOARD_ID(category, type) \
    ((UINT32)(((category) << 16) | (type)))

//=============================================================================
// Function Declarations
//=============================================================================

/**
 * @brief Initialize NuBus subsystem
 */
IO_RETURN IONuBusInitialize(VOID);

/**
 * @brief Get NuBus bus instance
 */
IO_RETURN IONuBusGetBus(IIONuBusBus **ppBus);

/**
 * @brief Detect if NuBus is present in system
 */
IO_RETURN IONuBusDetect(BOOLEAN *pbPresent);

/**
 * @brief Get card database entry
 */
IO_RETURN IONuBusGetCardInfo(
    UINT32                      uBoardID,
    CONST NUBUS_CARD_DB_ENTRY   **ppEntry
);

#ifdef __cplusplus
}
#endif

#endif // IOKIT_NUBUS_H
