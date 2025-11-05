/**
 * @file zorro.h
 * @brief Zorro Bus Family Interface - Commodore Amiga Expansion Bus
 *
 * This header defines the Zorro family interface for Commodore Amiga's expansion
 * bus architecture used in Amiga 2000/3000/4000 systems (1987-1994).
 *
 * Supported buses:
 * - Zorro II: 16 MB address space, 24-bit addressing, 8.33 MHz, up to 3.3 MB/s
 * - Zorro III: 4 GB address space, 32-bit addressing, 25 MHz, up to 150 MB/s burst
 *
 * Key features:
 * - AutoConfig protocol for automatic card detection and configuration
 * - ConfigDev structure in expansion.library
 * - Manufacturer and product ID database
 * - Board types: Memory expansion, I/O cards, DMA devices
 * - Interrupt support: INT2 (low priority), INT6 (high priority)
 * - Burst transfer mode for Zorro III
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_ZORRO_H
#define IOKIT_ZORRO_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Interface GUIDs
//=============================================================================

/**
 * @brief IIOZorroBus interface GUID
 * {C0DE1200-A500-4000-9000-000000000000}
 */
DEFINE_GUID(IID_IIOZorroBus,
    0xC0DE1200, 0xA500, 0x4000, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

/**
 * @brief IIOZorroDevice interface GUID
 * {C0DE1201-A500-4000-9000-000000000001}
 */
DEFINE_GUID(IID_IIOZorroDevice,
    0xC0DE1201, 0xA500, 0x4000, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01);

//=============================================================================
// Zorro Bus Constants
//=============================================================================

/**
 * @brief Zorro bus types
 */
typedef enum _ZORRO_BUS_TYPE {
    ZORRO_BUS_TYPE_UNKNOWN      = 0,
    ZORRO_BUS_TYPE_II           = 2,    /**< Zorro II: 16 MB, 24-bit */
    ZORRO_BUS_TYPE_III          = 3,    /**< Zorro III: 4 GB, 32-bit */
} ZORRO_BUS_TYPE;

/**
 * @brief Zorro II constants
 */
#define ZORRO_II_ADDRESS_SPACE      0x01000000  /**< 16 MB address space */
#define ZORRO_II_BASE_ADDR          0x00200000  /**< Base address: 2 MB */
#define ZORRO_II_END_ADDR           0x00A00000  /**< End address: 10 MB */
#define ZORRO_II_MAX_SIZE           0x00800000  /**< Max card size: 8 MB */
#define ZORRO_II_CLOCK_MHZ          8           /**< 8.33 MHz bus clock */
#define ZORRO_II_MAX_BANDWIDTH      3300000     /**< 3.3 MB/s max */

/**
 * @brief Zorro III constants
 */
#define ZORRO_III_ADDRESS_SPACE     0x100000000ULL  /**< 4 GB address space */
#define ZORRO_III_BASE_ADDR         0x40000000  /**< Base address: 1 GB */
#define ZORRO_III_END_ADDR          0x80000000  /**< End address: 2 GB */
#define ZORRO_III_MAX_SIZE          0x40000000  /**< Max card size: 1 GB */
#define ZORRO_III_CLOCK_MHZ         25          /**< 25 MHz bus clock */
#define ZORRO_III_BURST_BANDWIDTH   150000000   /**< 150 MB/s burst */

/**
 * @brief AutoConfig ROM addresses
 */
#define ZORRO_AUTOCONFIG_BASE       0x00E80000  /**< AutoConfig space */
#define ZORRO_AUTOCONFIG_SIZE       0x00010000  /**< 64 KB per slot */
#define ZORRO_MAX_SLOTS             100         /**< Maximum expansion slots */

/**
 * @brief AutoConfig ROM offsets
 */
#define ZORRO_ROM_TYPE_SIZE         0x00    /**< Type and size nibbles */
#define ZORRO_ROM_PRODUCT           0x04    /**< Product number */
#define ZORRO_ROM_FLAGS             0x08    /**< Configuration flags */
#define ZORRO_ROM_RESERVED          0x0C    /**< Reserved */
#define ZORRO_ROM_MANUFACTURER_HI   0x10    /**< Manufacturer ID high */
#define ZORRO_ROM_MANUFACTURER_LO   0x14    /**< Manufacturer ID low */
#define ZORRO_ROM_SERIAL_1          0x18    /**< Serial number byte 1 */
#define ZORRO_ROM_SERIAL_2          0x1C    /**< Serial number byte 2 */
#define ZORRO_ROM_SERIAL_3          0x20    /**< Serial number byte 3 */
#define ZORRO_ROM_SERIAL_4          0x24    /**< Serial number byte 4 */
#define ZORRO_ROM_INIT_DIAG_VEC_HI  0x28    /**< Init/diag vector high */
#define ZORRO_ROM_INIT_DIAG_VEC_LO  0x2C    /**< Init/diag vector low */

/**
 * @brief Type/Size register bits (offset 0x00)
 */
#define ZORRO_TYPE_MASK             0xC0    /**< Type mask */
#define ZORRO_TYPE_ZORROII          0xC0    /**< Zorro II card */
#define ZORRO_TYPE_ZORROIII         0x80    /**< Zorro III card */
#define ZORRO_SIZE_MASK             0x07    /**< Size mask */

/**
 * @brief Configuration flags (offset 0x08)
 */
#define ZORRO_FLAG_CAN_SHUTUP       0x40    /**< Can be shut up */
#define ZORRO_FLAG_NEXT_BOARD       0x20    /**< Next board exists */
#define ZORRO_FLAG_ROM_VECTOR       0x10    /**< ROM vector valid */
#define ZORRO_FLAG_MEMLIST          0x08    /**< Add to memory list */

/**
 * @brief Board types
 */
typedef enum _ZORRO_BOARD_TYPE {
    ZORRO_BOARD_TYPE_UNKNOWN    = 0,    /**< Unknown/invalid */
    ZORRO_BOARD_TYPE_MEMORY     = 1,    /**< Memory expansion */
    ZORRO_BOARD_TYPE_IO         = 2,    /**< I/O device */
    ZORRO_BOARD_TYPE_DMA        = 3,    /**< DMA-capable device */
    ZORRO_BOARD_TYPE_BRIDGE     = 4,    /**< Bus bridge */
} ZORRO_BOARD_TYPE;

/**
 * @brief Board size codes (in 64 KB units for Zorro II, 1 MB for Zorro III)
 */
typedef enum _ZORRO_SIZE_CODE {
    ZORRO_SIZE_8MB              = 0,    /**< 8 MB */
    ZORRO_SIZE_64KB             = 1,    /**< 64 KB */
    ZORRO_SIZE_128KB            = 2,    /**< 128 KB */
    ZORRO_SIZE_256KB            = 3,    /**< 256 KB */
    ZORRO_SIZE_512KB            = 4,    /**< 512 KB */
    ZORRO_SIZE_1MB              = 5,    /**< 1 MB */
    ZORRO_SIZE_2MB              = 6,    /**< 2 MB */
    ZORRO_SIZE_4MB              = 7,    /**< 4 MB */
} ZORRO_SIZE_CODE;

/**
 * @brief Interrupt levels
 */
typedef enum _ZORRO_IRQ_LEVEL {
    ZORRO_IRQ_NONE              = 0,    /**< No interrupt */
    ZORRO_IRQ_INT2              = 2,    /**< INT2 (low priority) */
    ZORRO_IRQ_INT6              = 6,    /**< INT6 (high priority) */
} ZORRO_IRQ_LEVEL;

//=============================================================================
// Manufacturer IDs (16-bit)
//=============================================================================

#define ZORRO_MFG_COMMODORE         0x0202  /**< Commodore Business Machines */
#define ZORRO_MFG_COMMODORE_ALT     0x0201  /**< Commodore (alternate) */
#define ZORRO_MFG_GVP               0x0891  /**< Great Valley Products */
#define ZORRO_MFG_PHASE5            0x2140  /**< Phase5 Digital Products */
#define ZORRO_MFG_VILLAGE_TRONIC    0x2167  /**< Village Tronic */
#define ZORRO_MFG_PROGRESSIVE_PERIPH 0x2169 /**< Progressive Peripherals */
#define ZORRO_MFG_APOLLO            0x2200  /**< Apollo Computer */
#define ZORRO_MFG_MACROSYSTEMS      0x18C6  /**< MacroSystem */
#define ZORRO_MFG_BVISION           0x2131  /**< BVision */
#define ZORRO_MFG_HYDRA             0x2121  /**< Hydra Systems */
#define ZORRO_MFG_READYSOFT         0x2129  /**< ReadySoft */
#define ZORRO_MFG_IMTRONICS         0x2145  /**< Imtronics */
#define ZORRO_MFG_CARDCO            0x03EC  /**< Cardco */
#define ZORRO_MFG_DKB               0x07DB  /**< DKB (Dave Kinzer Buffalo) */
#define ZORRO_MFG_HELFRICH          0x0861  /**< Helfrich */
#define ZORRO_MFG_MICROBOTICS       0x0A75  /**< MicroBotics */
#define ZORRO_MFG_EXPANSION_SYS     0x07E6  /**< Expansion Systems */
#define ZORRO_MFG_MTEC              0x1388  /**< M-TEC */
#define ZORRO_MFG_BSC               0x07E5  /**< BSC Alfadata */
#define ZORRO_MFG_HACKER            0x07DB  /**< Hacker */
#define ZORRO_MFG_JOCHHEIM          0x0A82  /**< Jochheim */
#define ZORRO_MFG_ACT               0x0828  /**< ACT */
#define ZORRO_MFG_XETEC             0x07E9  /**< Xetec */
#define ZORRO_MFG_GFX_BASE          0x08F8  /**< GFX-Base */
#define ZORRO_MFG_ROCTEC            0x2260  /**< Roctec */
#define ZORRO_MFG_BUDDHA            0x1212  /**< Buddha Flash */
#define ZORRO_MFG_INDIVIDUAL        0x2134  /**< Individual Computers */
#define ZORRO_MFG_ELBOX             0x2206  /**< Elbox Computer */

//=============================================================================
// Product IDs for Known Cards
//=============================================================================

// Commodore products
#define ZORRO_PROD_CBM_A2088        0x01    /**< A2088 XT bridgeboard */
#define ZORRO_PROD_CBM_A2286        0x02    /**< A2286 AT bridgeboard */
#define ZORRO_PROD_CBM_A2090A       0x0A    /**< A2090a HD controller */
#define ZORRO_PROD_CBM_A2052        0x0C    /**< A2052 RAM expansion */
#define ZORRO_PROD_CBM_A590         0x20    /**< A590 HD controller */
#define ZORRO_PROD_CBM_A2091        0x54    /**< A2091 SCSI controller */
#define ZORRO_PROD_CBM_A2091_2      0x03    /**< A2091 (alternate ID) */
#define ZORRO_PROD_CBM_A2232        0x45    /**< A2232 multiport serial */
#define ZORRO_PROD_CBM_A2386        0x67    /**< A2386-SX AT bridgeboard */

// GVP products
#define ZORRO_PROD_GVP_IMPACT_SERIES_I  0x08    /**< Impact Series I */
#define ZORRO_PROD_GVP_IMPACT_SERIES_II 0x09    /**< Impact Series II */
#define ZORRO_PROD_GVP_IMPACT_3001_IDE  0x0A    /**< Impact 3001 IDE */
#define ZORRO_PROD_GVP_IMPACT_3001_SCSI 0x0B    /**< Impact 3001 SCSI */
#define ZORRO_PROD_GVP_A530_TURBO       0x0C    /**< A530 Turbo */
#define ZORRO_PROD_GVP_COMBO_030_R4     0x0D    /**< Combo 030 R4 */
#define ZORRO_PROD_GVP_PHONEPAL         0x10    /**< PhonePak */
#define ZORRO_PROD_GVP_IOEXTENDER       0x20    /**< I/O Extender */
#define ZORRO_PROD_GVP_GFORCE_030       0x30    /**< G-Force 030 */
#define ZORRO_PROD_GVP_GFORCE_040       0x40    /**< G-Force 040 */
#define ZORRO_PROD_GVP_A1291            0xFE    /**< A1291 SCSI */

// Phase5 products
#define ZORRO_PROD_PHASE5_BLIZZARD_1230 0x01   /**< Blizzard 1230-IV */
#define ZORRO_PROD_PHASE5_BLIZZARD_1260 0x02   /**< Blizzard 1260 */
#define ZORRO_PROD_PHASE5_BLIZZARD_2060 0x03   /**< Blizzard 2060 */
#define ZORRO_PROD_PHASE5_CYBERSTORM    0x06   /**< CyberStorm 060 */
#define ZORRO_PROD_PHASE5_CYBERVISION   0x0C   /**< CyberVision 64 */
#define ZORRO_PROD_PHASE5_CYBERVISION3D 0x43   /**< CyberVision 64/3D */
#define ZORRO_PROD_PHASE5_FASTLANE      0x0B   /**< FastLane Z3 RAM */

// Village Tronic products
#define ZORRO_PROD_VT_PICASSO_II        0x01   /**< Picasso II */
#define ZORRO_PROD_VT_PICASSO_II_PLUS   0x02   /**< Picasso II+ */
#define ZORRO_PROD_VT_PICASSO_IV_Z2     0x15   /**< Picasso IV (Zorro II) */
#define ZORRO_PROD_VT_PICASSO_IV_Z3     0x16   /**< Picasso IV (Zorro III) */
#define ZORRO_PROD_VT_ARIADNE           0x01   /**< Ariadne Ethernet */
#define ZORRO_PROD_VT_ARIADNE_II        0xC9   /**< Ariadne II Ethernet */

// Progressive Peripherals & Software products
#define ZORRO_PROD_PPS_MERCURY          0x01   /**< Mercury */
#define ZORRO_PROD_PPS_A3000_RAM8       0x11   /**< A3000 8MB RAM */
#define ZORRO_PROD_PPS_A3000_RAM2       0x12   /**< A3000 2MB RAM */
#define ZORRO_PROD_PPS_ZEUS_040         0x13   /**< Zeus 68040 */

// MacroSystem products
#define ZORRO_PROD_MACROSYSTEM_WARP     0x13   /**< Warp Engine */
#define ZORRO_PROD_MACROSYSTEM_RETINA   0x01   /**< Retina Z2 */
#define ZORRO_PROD_MACROSYSTEM_RETINA_Z3 0x02  /**< Retina Z3 */
#define ZORRO_PROD_MACROSYSTEM_VLAB     0x10   /**< VLab */
#define ZORRO_PROD_MACROSYSTEM_FALCON   0x04   /**< Falcon 040 */

// Individual Computers products
#define ZORRO_PROD_INDIVIDUAL_BUDDHA    0x00   /**< Buddha IDE */
#define ZORRO_PROD_INDIVIDUAL_CATWEASEL 0x01   /**< Catweasel */
#define ZORRO_PROD_INDIVIDUAL_X_SURF    0x17   /**< X-Surf Ethernet */

//=============================================================================
// AutoConfig Structures
//=============================================================================

/**
 * @brief AutoConfig ROM structure
 *
 * The AutoConfig ROM contains board identification and configuration data
 * stored in every second nibble of 16 bytes (32 nibbles total).
 */
typedef struct _ZORRO_AUTOCONFIG_ROM {
    UINT8   uTypeSize;              /**< Type and size nibbles */
    UINT8   uProduct;               /**< Product number */
    UINT8   uFlags;                 /**< Configuration flags */
    UINT8   uReserved;              /**< Reserved */
    UINT16  uManufacturer;          /**< Manufacturer ID */
    UINT32  uSerialNumber;          /**< Serial number */
    UINT16  uInitDiagVector;        /**< Init/diag vector offset */
} ZORRO_AUTOCONFIG_ROM;

/**
 * @brief ConfigDev structure
 *
 * This structure represents a configured Zorro device. Based on the
 * Amiga expansion.library ConfigDev structure.
 */
typedef struct _ZORRO_CONFIGDEV {
    // Device identification
    UINT16  uManufacturer;          /**< Manufacturer ID */
    UINT8   uProduct;               /**< Product number */
    UINT8   uFlags;                 /**< Configuration flags */
    UINT32  uSerialNumber;          /**< Board serial number */

    // Memory configuration
    UINT32  uBaseAddress;           /**< Configured base address */
    UINT32  uBoardSize;             /**< Board size in bytes */
    UINT8   uSlotNumber;            /**< Slot number (0-based) */
    UINT8   uSlotAddress;           /**< Slot address */

    // Board characteristics
    ZORRO_BUS_TYPE  eBusType;       /**< Zorro II or III */
    ZORRO_BOARD_TYPE eType;         /**< Board type */
    UINT8   uInterruptLevel;        /**< IRQ level (0, 2, or 6) */

    // ROM information
    UINT16  uROMVector;             /**< ROM vector offset */
    VOID    *pROMBase;              /**< Pointer to ROM base */
    UINT32  uROMSize;               /**< ROM size in bytes */

    // Flags
    BOOLEAN bCanShutUp;             /**< Can be shut up */
    BOOLEAN bMemoryDevice;          /**< Should be added to memory list */
    BOOLEAN bConfigured;            /**< Has been configured */
    BOOLEAN bDMACapable;            /**< Supports DMA */
    BOOLEAN bBurstMode;             /**< Supports burst (Zorro III) */
} ZORRO_CONFIGDEV;

/**
 * @brief Zorro device information
 */
typedef struct _ZORRO_DEVICE_INFO {
    ZORRO_CONFIGDEV ConfigDev;      /**< ConfigDev structure */

    // Human-readable information
    CHAR8   szManufacturerName[64]; /**< Manufacturer name */
    CHAR8   szProductName[128];     /**< Product name */
    CHAR8   szDescription[256];     /**< Description */

    // Additional information
    UINT32  uRevision;              /**< Board revision */
    CHAR8   szDriverName[64];       /**< Preferred driver name */
} ZORRO_DEVICE_INFO;

//=============================================================================
// DMA Structures
//=============================================================================

/**
 * @brief DMA transfer direction
 */
typedef enum _ZORRO_DMA_DIRECTION {
    ZORRO_DMA_TO_DEVICE     = 0,    /**< Memory to device */
    ZORRO_DMA_FROM_DEVICE   = 1,    /**< Device to memory */
    ZORRO_DMA_BIDIRECTIONAL = 2,    /**< Bidirectional */
} ZORRO_DMA_DIRECTION;

/**
 * @brief DMA transfer mode
 */
typedef enum _ZORRO_DMA_MODE {
    ZORRO_DMA_MODE_SINGLE   = 0,    /**< Single transfer */
    ZORRO_DMA_MODE_BURST    = 1,    /**< Burst transfer (Z3) */
    ZORRO_DMA_MODE_BLOCK    = 2,    /**< Block transfer */
} ZORRO_DMA_MODE;

/**
 * @brief DMA transfer parameters
 */
typedef struct _ZORRO_DMA_PARAMS {
    VOID                    *pBuffer;       /**< Buffer address */
    UINT32                  uLength;        /**< Transfer length */
    ZORRO_DMA_DIRECTION     eDirection;     /**< Transfer direction */
    ZORRO_DMA_MODE          eMode;          /**< Transfer mode */
    BOOLEAN                 bAutoIncrement; /**< Auto-increment address */
    UINT32                  uBurstSize;     /**< Burst size (Z3 only) */
} ZORRO_DMA_PARAMS;

//=============================================================================
// Zorro Card Database Entry
//=============================================================================

/**
 * @brief Known Zorro card database entry
 */
typedef struct _ZORRO_CARD_DB_ENTRY {
    UINT16              uManufacturer;      /**< Manufacturer ID */
    UINT8               uProduct;           /**< Product number */
    CONST CHAR8         *pszManufacturer;   /**< Manufacturer name */
    CONST CHAR8         *pszProduct;        /**< Product name */
    CONST CHAR8         *pszDescription;    /**< Description */
    ZORRO_BOARD_TYPE    eType;              /**< Board type */
    ZORRO_BUS_TYPE      eBusType;           /**< Bus type */
    CONST CHAR8         *pszDriver;         /**< Recommended driver */
} ZORRO_CARD_DB_ENTRY;

//=============================================================================
// Forward Declarations
//=============================================================================

typedef struct IIOZorroBus IIOZorroBus;
typedef struct IIOZorroDevice IIOZorroDevice;

//=============================================================================
// IIOZorroBus Interface
//=============================================================================

/**
 * @brief Zorro bus interface
 *
 * Represents a Zorro bus controller (II or III) and provides methods for
 * device enumeration, AutoConfig, and resource management.
 */
struct IIOZorroBus {
    IIOService Base;

    /**
     * @brief Detect Zorro bus type
     *
     * @param this          Interface pointer
     * @param peBusType     Receives bus type (Zorro II or III)
     *
     * @retval IO_SUCCESS   Bus type detected
     * @retval IO_NO_DEVICE No Zorro bus present
     */
    IO_RETURN (*DetectBus)(
        IIOZorroBus         *this,
        ZORRO_BUS_TYPE      *peBusType
    );

    /**
     * @brief Scan for AutoConfig devices
     *
     * Performs AutoConfig protocol to detect and enumerate expansion cards.
     *
     * @param this          Interface pointer
     * @param puDeviceCount Receives number of devices found
     *
     * @retval IO_SUCCESS   Scan completed
     */
    IO_RETURN (*ScanAutoConfig)(
        IIOZorroBus         *this,
        UINT32              *puDeviceCount
    );

    /**
     * @brief Get device by slot number
     *
     * @param this          Interface pointer
     * @param uSlot         Slot number (0-based)
     * @param ppDevice      Receives device interface
     *
     * @retval IO_SUCCESS   Device retrieved
     * @retval IO_NO_DEVICE No device in slot
     */
    IO_RETURN (*GetDeviceBySlot)(
        IIOZorroBus         *this,
        UINT8               uSlot,
        IIOZorroDevice      **ppDevice
    );

    /**
     * @brief Get device by manufacturer and product
     *
     * @param this          Interface pointer
     * @param uManufacturer Manufacturer ID
     * @param uProduct      Product ID
     * @param ppDevice      Receives device interface
     *
     * @retval IO_SUCCESS   Device found
     * @retval IO_NOT_FOUND Device not present
     */
    IO_RETURN (*FindDevice)(
        IIOZorroBus         *this,
        UINT16              uManufacturer,
        UINT8               uProduct,
        IIOZorroDevice      **ppDevice
    );

    /**
     * @brief Enumerate all devices
     *
     * @param this          Interface pointer
     * @param pppDevices    Receives array of device interfaces
     * @param puCount       On input: max devices; On output: actual count
     *
     * @retval IO_SUCCESS   Enumeration successful
     */
    IO_RETURN (*EnumerateDevices)(
        IIOZorroBus         *this,
        IIOZorroDevice      ***pppDevices,
        UINT32              *puCount
    );

    /**
     * @brief Read AutoConfig ROM
     *
     * @param this          Interface pointer
     * @param uSlot         Slot number
     * @param uOffset       Offset in ROM
     * @param pBuffer       Buffer to receive data
     * @param uLength       Number of bytes to read
     *
     * @retval IO_SUCCESS   Read successful
     */
    IO_RETURN (*ReadAutoConfigROM)(
        IIOZorroBus         *this,
        UINT8               uSlot,
        UINT32              uOffset,
        VOID                *pBuffer,
        UINT32              uLength
    );

    /**
     * @brief Configure device address
     *
     * Called during AutoConfig to assign base address to device.
     *
     * @param this          Interface pointer
     * @param uSlot         Slot number
     * @param uBaseAddr     Base address to assign
     *
     * @retval IO_SUCCESS   Address configured
     */
    IO_RETURN (*ConfigureAddress)(
        IIOZorroBus         *this,
        UINT8               uSlot,
        UINT32              uBaseAddr
    );

    /**
     * @brief Shut up device
     *
     * Tells a device to remove itself from AutoConfig space.
     *
     * @param this          Interface pointer
     * @param uSlot         Slot number
     *
     * @retval IO_SUCCESS   Device shut up
     */
    IO_RETURN (*ShutUpDevice)(
        IIOZorroBus         *this,
        UINT8               uSlot
    );

    /**
     * @brief Allocate DMA channel
     *
     * @param this          Interface pointer
     * @param pDevice       Device requesting DMA
     * @param puChannel     Receives DMA channel number
     *
     * @retval IO_SUCCESS       Channel allocated
     * @retval IO_NO_CHANNELS   No channels available
     */
    IO_RETURN (*AllocateDMA)(
        IIOZorroBus         *this,
        IIOZorroDevice      *pDevice,
        UINT32              *puChannel
    );

    /**
     * @brief Free DMA channel
     *
     * @param this          Interface pointer
     * @param uChannel      Channel number to free
     *
     * @retval IO_SUCCESS   Channel freed
     */
    IO_RETURN (*FreeDMA)(
        IIOZorroBus         *this,
        UINT32              uChannel
    );

    /**
     * @brief Enable burst mode (Zorro III only)
     *
     * @param this          Interface pointer
     * @param pDevice       Device to enable burst mode
     * @param bEnable       TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS       Burst mode configured
     * @retval IO_UNSUPPORTED   Not Zorro III
     */
    IO_RETURN (*EnableBurstMode)(
        IIOZorroBus         *this,
        IIOZorroDevice      *pDevice,
        BOOLEAN             bEnable
    );
};

//=============================================================================
// IIOZorroDevice Interface
//=============================================================================

/**
 * @brief Zorro device interface
 *
 * Represents a Zorro expansion card and provides methods for device
 * I/O, interrupts, and DMA.
 */
struct IIOZorroDevice {
    IIOService Base;

    /**
     * @brief Get device information
     *
     * @param this          Interface pointer
     * @param pInfo         Receives device information
     *
     * @retval IO_SUCCESS   Information retrieved
     */
    IO_RETURN (*GetDeviceInfo)(
        IIOZorroDevice      *this,
        ZORRO_DEVICE_INFO   *pInfo
    );

    /**
     * @brief Get ConfigDev structure
     *
     * @param this          Interface pointer
     * @param pConfigDev    Receives ConfigDev
     *
     * @retval IO_SUCCESS   ConfigDev retrieved
     */
    IO_RETURN (*GetConfigDev)(
        IIOZorroDevice      *this,
        ZORRO_CONFIGDEV     *pConfigDev
    );

    /**
     * @brief Get base address
     *
     * @param this          Interface pointer
     * @param puBaseAddr    Receives base address
     *
     * @retval IO_SUCCESS   Address retrieved
     */
    IO_RETURN (*GetBaseAddress)(
        IIOZorroDevice      *this,
        UINT32              *puBaseAddr
    );

    /**
     * @brief Get board size
     *
     * @param this          Interface pointer
     * @param puSize        Receives size in bytes
     *
     * @retval IO_SUCCESS   Size retrieved
     */
    IO_RETURN (*GetBoardSize)(
        IIOZorroDevice      *this,
        UINT32              *puSize
    );

    /**
     * @brief Map device memory
     *
     * @param this          Interface pointer
     * @param uOffset       Offset from base
     * @param uLength       Length to map
     * @param ppMapped      Receives mapped pointer
     *
     * @retval IO_SUCCESS   Memory mapped
     * @retval IO_NO_MEMORY Mapping failed
     */
    IO_RETURN (*MapMemory)(
        IIOZorroDevice      *this,
        UINT32              uOffset,
        UINT32              uLength,
        VOID                **ppMapped
    );

    /**
     * @brief Unmap device memory
     *
     * @param this          Interface pointer
     * @param pMapped       Mapped pointer
     * @param uLength       Length to unmap
     *
     * @retval IO_SUCCESS   Memory unmapped
     */
    IO_RETURN (*UnmapMemory)(
        IIOZorroDevice      *this,
        VOID                *pMapped,
        UINT32              uLength
    );

    /**
     * @brief Read from device
     *
     * @param this          Interface pointer
     * @param uOffset       Offset from base
     * @param pBuffer       Buffer to receive data
     * @param uLength       Number of bytes to read
     *
     * @retval IO_SUCCESS   Read successful
     */
    IO_RETURN (*ReadMemory)(
        IIOZorroDevice      *this,
        UINT32              uOffset,
        VOID                *pBuffer,
        UINT32              uLength
    );

    /**
     * @brief Write to device
     *
     * @param this          Interface pointer
     * @param uOffset       Offset from base
     * @param pBuffer       Data to write
     * @param uLength       Number of bytes to write
     *
     * @retval IO_SUCCESS   Write successful
     */
    IO_RETURN (*WriteMemory)(
        IIOZorroDevice      *this,
        UINT32              uOffset,
        CONST VOID          *pBuffer,
        UINT32              uLength
    );

    /**
     * @brief Enable device interrupt
     *
     * @param this          Interface pointer
     * @param eLevel        Interrupt level (INT2 or INT6)
     * @param pfnHandler    Interrupt handler function
     * @param pContext      Handler context
     *
     * @retval IO_SUCCESS       Interrupt enabled
     * @retval IO_NO_INTERRUPT  Interrupt not available
     */
    IO_RETURN (*EnableInterrupt)(
        IIOZorroDevice      *this,
        ZORRO_IRQ_LEVEL     eLevel,
        VOID                (*pfnHandler)(VOID *pContext),
        VOID                *pContext
    );

    /**
     * @brief Disable device interrupt
     *
     * @param this          Interface pointer
     * @param eLevel        Interrupt level
     *
     * @retval IO_SUCCESS   Interrupt disabled
     */
    IO_RETURN (*DisableInterrupt)(
        IIOZorroDevice      *this,
        ZORRO_IRQ_LEVEL     eLevel
    );

    /**
     * @brief Setup DMA transfer
     *
     * @param this          Interface pointer
     * @param pParams       DMA parameters
     *
     * @retval IO_SUCCESS       DMA configured
     * @retval IO_UNSUPPORTED   Device not DMA-capable
     */
    IO_RETURN (*SetupDMA)(
        IIOZorroDevice      *this,
        CONST ZORRO_DMA_PARAMS *pParams
    );

    /**
     * @brief Start DMA transfer
     *
     * @param this          Interface pointer
     *
     * @retval IO_SUCCESS   Transfer started
     */
    IO_RETURN (*StartDMA)(
        IIOZorroDevice      *this
    );

    /**
     * @brief Stop DMA transfer
     *
     * @param this          Interface pointer
     *
     * @retval IO_SUCCESS   Transfer stopped
     */
    IO_RETURN (*StopDMA)(
        IIOZorroDevice      *this
    );

    /**
     * @brief Check if DMA is complete
     *
     * @param this          Interface pointer
     * @param pbComplete    Receives completion status
     *
     * @retval IO_SUCCESS   Status retrieved
     */
    IO_RETURN (*IsDMAComplete)(
        IIOZorroDevice      *this,
        BOOLEAN             *pbComplete
    );

    /**
     * @brief Enable/disable device
     *
     * @param this          Interface pointer
     * @param bEnable       TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS   Device state changed
     */
    IO_RETURN (*Enable)(
        IIOZorroDevice      *this,
        BOOLEAN             bEnable
    );
};

//=============================================================================
// Helper Macros
//=============================================================================

/**
 * @brief Decode size code to bytes for Zorro II
 */
#define ZORRO_II_SIZE_TO_BYTES(code) \
    ((code) == 0 ? 0x00800000 : (0x00010000 << ((code) - 1)))

/**
 * @brief Decode size code to bytes for Zorro III
 */
#define ZORRO_III_SIZE_TO_BYTES(code) \
    ((code) == 0 ? 0x08000000 : (0x00100000 << ((code) - 1)))

/**
 * @brief Make device ID from manufacturer and product
 */
#define ZORRO_MAKE_ID(mfg, prod) \
    ((UINT32)(((mfg) << 16) | (prod)))

/**
 * @brief Extract manufacturer from device ID
 */
#define ZORRO_GET_MANUFACTURER(id) \
    ((UINT16)((id) >> 16))

/**
 * @brief Extract product from device ID
 */
#define ZORRO_GET_PRODUCT(id) \
    ((UINT8)((id) & 0xFF))

/**
 * @brief Check if address is in Zorro II space
 */
#define ZORRO_IS_ZORRO_II_ADDR(addr) \
    ((addr) >= ZORRO_II_BASE_ADDR && (addr) < ZORRO_II_END_ADDR)

/**
 * @brief Check if address is in Zorro III space
 */
#define ZORRO_IS_ZORRO_III_ADDR(addr) \
    ((addr) >= ZORRO_III_BASE_ADDR && (addr) < ZORRO_III_END_ADDR)

//=============================================================================
// Public API Functions
//=============================================================================

/**
 * @brief Initialize Zorro subsystem
 *
 * @retval IO_SUCCESS   Subsystem initialized
 */
IO_RETURN ZorroInitialize(VOID);

/**
 * @brief Shutdown Zorro subsystem
 *
 * @retval IO_SUCCESS   Subsystem shut down
 */
IO_RETURN ZorroShutdown(VOID);

/**
 * @brief Get Zorro bus instance
 *
 * @param ppBus         Receives bus interface
 *
 * @retval IO_SUCCESS   Bus retrieved
 * @retval IO_NO_DEVICE No Zorro bus present
 */
IO_RETURN IOZorroGetBus(IIOZorroBus **ppBus);

/**
 * @brief Detect if Zorro bus is present
 *
 * @param pbPresent     Receives presence flag
 *
 * @retval IO_SUCCESS   Detection complete
 */
IO_RETURN IOZorroDetect(BOOLEAN *pbPresent);

/**
 * @brief Get card information from database
 *
 * @param uManufacturer Manufacturer ID
 * @param uProduct      Product ID
 * @param ppEntry       Receives database entry
 *
 * @retval IO_SUCCESS   Card found in database
 * @retval IO_NOT_FOUND Card not in database
 */
IO_RETURN IOZorroGetCardInfo(
    UINT16                      uManufacturer,
    UINT8                       uProduct,
    CONST ZORRO_CARD_DB_ENTRY   **ppEntry
);

/**
 * @brief Parse AutoConfig ROM
 *
 * Reads and decodes AutoConfig ROM data into ConfigDev structure.
 *
 * @param pROMData      Pointer to AutoConfig ROM data
 * @param pConfigDev    Receives parsed ConfigDev
 *
 * @retval IO_SUCCESS       Parse successful
 * @retval IO_BAD_ARGUMENT  Invalid ROM data
 */
IO_RETURN ZorroParseAutoConfigROM(
    CONST VOID          *pROMData,
    ZORRO_CONFIGDEV     *pConfigDev
);

/**
 * @brief Get manufacturer name
 *
 * @param uManufacturer Manufacturer ID
 * @param pszName       Buffer to receive name
 * @param cbSize        Buffer size
 *
 * @retval IO_SUCCESS   Name retrieved
 */
IO_RETURN ZorroGetManufacturerName(
    UINT16              uManufacturer,
    CHAR8               *pszName,
    UINTN               cbSize
);

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_ZORRO_H */
