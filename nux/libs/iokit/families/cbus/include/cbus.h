/**
 * @file cbus.h
 * @brief C-bus (PC-9801 Bus) Family Interface - NEC PC-98 Series Expansion
 *
 * This header defines the C-bus family interface for NEC's PC-9801 series
 * expansion bus architecture, the dominant personal computer platform in Japan
 * during the 1980s and 1990s.
 *
 * C-bus variants:
 * - C-bus 8-bit: Original 8-bit bus, 5/8 MHz, used in early PC-9801 models
 * - C-bus 16-bit: 16-bit extension, 8/10 MHz, added with PC-9801VM/VX
 * - PC-H98: High-speed 32-bit variant, up to 66 MHz, PCI-compatible
 * - Local bus: CPU-synchronized variant for high-speed peripherals
 *
 * Bus characteristics:
 * - I/O port addressing: 0x0000-0xFFFF (conflicts with standard ISA)
 * - Memory addressing: 0xA0000-0xFFFFF (640KB-1MB range)
 * - Interrupts: INT0-INT6 (7 levels), edge-triggered
 * - DMA: 4 channels (0-3), similar to Intel 8237A
 * - Card slots: Typically 3-6 slots depending on model
 *
 * Notable features:
 * - Non-ISA compatible (proprietary NEC design)
 * - Japanese text video capabilities (GRCG, EGC graphic accelerators)
 * - Sound support (PC-9801-26/86 sound boards)
 * - SCSI, networking, and graphics expansion cards
 *
 * Supported systems:
 * - PC-9801 series (1982-1997): VM, VX, UV, UX, RA, RX, etc.
 * - PC-9821 series (1992-2000): Windows-capable models
 * - PC-H98 series (1995-2000): High-end workstations
 * - FC-9801 series: Industrial/embedded variants
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_CBUS_H
#define IOKIT_CBUS_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Interface GUIDs
//=============================================================================

/**
 * @brief IIOCBusBus interface GUID
 * {C98019EC-9801-4C62-7573-4641757348A8}
 */
DEFINE_GUID(IID_IIOCBusBus,
    0xC98019EC, 0x9801, 0x4C62, 0x75, 0x73, 0x46, 0x41, 0x75, 0x73, 0x48, 0xA8);

/**
 * @brief IIOCBusDevice interface GUID
 * {C98019ED-9801-4C62-7573-4465764348A8}
 */
DEFINE_GUID(IID_IIOCBusDevice,
    0xC98019ED, 0x9801, 0x4C62, 0x75, 0x73, 0x44, 0x65, 0x76, 0x43, 0x48, 0xA8);

//=============================================================================
// C-bus Constants
//=============================================================================

/**
 * @brief C-bus types
 */
typedef enum _CBUS_TYPE {
    CBUS_TYPE_UNKNOWN       = 0,    /**< Unknown bus type */
    CBUS_TYPE_8BIT          = 1,    /**< 8-bit C-bus (original) */
    CBUS_TYPE_16BIT         = 2,    /**< 16-bit C-bus (VM/VX+) */
    CBUS_TYPE_PCH98         = 3,    /**< PC-H98 high-speed bus */
    CBUS_TYPE_LOCAL         = 4,    /**< Local bus variant */
} CBUS_TYPE;

/**
 * @brief C-bus slot configuration
 */
#define CBUS_SLOT_MIN           0       /**< First slot */
#define CBUS_SLOT_MAX           5       /**< Last slot (typically) */
#define CBUS_SLOT_COUNT_TYPICAL 3       /**< Common slot count */
#define CBUS_SLOT_COUNT_MAX     6       /**< Maximum slots in high-end models */

/**
 * @brief C-bus timing
 */
#define CBUS_CLOCK_8BIT_SLOW    5000000     /**< 5 MHz (early models) */
#define CBUS_CLOCK_8BIT_FAST    8000000     /**< 8 MHz */
#define CBUS_CLOCK_16BIT_SLOW   8000000     /**< 8 MHz */
#define CBUS_CLOCK_16BIT_FAST   10000000    /**< 10 MHz */
#define CBUS_CLOCK_PCH98_MIN    33000000    /**< 33 MHz */
#define CBUS_CLOCK_PCH98_MAX    66000000    /**< 66 MHz */

/**
 * @brief C-bus I/O port ranges
 *
 * Note: C-bus uses full 16-bit I/O space, conflicting with standard ISA.
 * Common address ranges:
 */
#define CBUS_IO_BASE            0x0000  /**< I/O space start */
#define CBUS_IO_END             0xFFFF  /**< I/O space end */

// Standard PC-98 I/O port assignments
#define CBUS_IO_FDC_BASE        0x0090  /**< Floppy controller */
#define CBUS_IO_FDC_STATUS      0x0094  /**< FDC status */
#define CBUS_IO_FDC_DATA        0x0092  /**< FDC data */

#define CBUS_IO_HDD_BASE        0x0640  /**< Hard disk controller (SASI/IDE) */
#define CBUS_IO_HDD_DATA        0x0640  /**< HDD data */
#define CBUS_IO_HDD_ERROR       0x0642  /**< HDD error */
#define CBUS_IO_HDD_STATUS      0x0644  /**< HDD status */

#define CBUS_IO_GDC_BASE        0x0060  /**< Graphics Display Controller */
#define CBUS_IO_GDC_COMMAND     0x0062  /**< GDC command */
#define CBUS_IO_GDC_PARAM       0x0060  /**< GDC parameter */

#define CBUS_IO_CRTC_BASE       0x0070  /**< CRT controller */
#define CBUS_IO_GRCG_BASE       0x007C  /**< GRCG (Graphic Charger) */
#define CBUS_IO_EGC_BASE        0x04A0  /**< EGC (Enhanced Graphic Charger) */

#define CBUS_IO_SOUND_BASE      0x0188  /**< Sound board base (PC-9801-26) */
#define CBUS_IO_SOUND86_BASE    0x0A460 /**< Sound board (PC-9801-86) */

#define CBUS_IO_SERIAL_BASE     0x0030  /**< Serial port base */
#define CBUS_IO_MOUSE_BASE      0x007FD0 /**< Mouse controller */

#define CBUS_IO_PIC_MASTER      0x0000  /**< Master PIC */
#define CBUS_IO_PIC_SLAVE       0x0008  /**< Slave PIC */

#define CBUS_IO_DMA_BASE        0x0001  /**< DMA controller */
#define CBUS_IO_DMAPG_BASE      0x0021  /**< DMA page registers */

#define CBUS_IO_TIMER_BASE      0x0071  /**< System timer (8253) */
#define CBUS_IO_RTC_BASE        0x0020  /**< Real-time clock */

/**
 * @brief C-bus memory ranges
 */
#define CBUS_MEM_CONVENTIONAL   0x00000000  /**< Conventional RAM (0-640KB) */
#define CBUS_MEM_CONV_SIZE      0x000A0000  /**< 640KB */

#define CBUS_MEM_TEXT_BASE      0x000A0000  /**< Text VRAM */
#define CBUS_MEM_TEXT_SIZE      0x00002000  /**< 8KB text VRAM */

#define CBUS_MEM_GRAPHIC_BASE   0x000A8000  /**< Graphic VRAM */
#define CBUS_MEM_GRAPHIC_SIZE   0x00018000  /**< 96KB graphic VRAM (standard) */

#define CBUS_MEM_EXTGRAPH_BASE  0x000E0000  /**< Extended graphic VRAM */
#define CBUS_MEM_EXTGRAPH_SIZE  0x00008000  /**< 32KB extended graphic */

#define CBUS_MEM_BIOS_BASE      0x000E8000  /**< System BIOS ROM */
#define CBUS_MEM_BIOS_SIZE      0x00018000  /**< 96KB BIOS */

#define CBUS_MEM_WINDOW_BASE    0x000CC000  /**< Window RAM (bank-switched) */
#define CBUS_MEM_WINDOW_SIZE    0x00004000  /**< 16KB window */

#define CBUS_MEM_EXPANSION_BASE 0x00100000  /**< Expansion RAM (above 1MB) */

/**
 * @brief C-bus interrupt levels (INT0-INT6)
 *
 * PC-98 uses a different interrupt controller than IBM PC.
 * Interrupts are edge-triggered and use INT0-INT6 naming.
 */
typedef enum _CBUS_IRQ_LEVEL {
    CBUS_INT0               = 0,    /**< INT0 - Highest priority */
    CBUS_INT1               = 1,    /**< INT1 */
    CBUS_INT2               = 2,    /**< INT2 */
    CBUS_INT3               = 3,    /**< INT3 */
    CBUS_INT4               = 4,    /**< INT4 */
    CBUS_INT5               = 5,    /**< INT5 */
    CBUS_INT6               = 6,    /**< INT6 - Lowest priority */
    CBUS_INT_COUNT          = 7,    /**< Total interrupt count */
    CBUS_INT_NONE           = 0xFF, /**< No interrupt */
} CBUS_IRQ_LEVEL;

/**
 * @brief Standard interrupt assignments
 */
#define CBUS_INT_TIMER          CBUS_INT0   /**< System timer (8253) */
#define CBUS_INT_KEYBOARD       CBUS_INT1   /**< Keyboard controller */
#define CBUS_INT_CRTC           CBUS_INT2   /**< CRT controller */
#define CBUS_INT_EXPANSION      CBUS_INT3   /**< Expansion slots (most cards) */
#define CBUS_INT_SERIAL         CBUS_INT4   /**< Serial port (RS-232C) */
#define CBUS_INT_FDD            CBUS_INT5   /**< Floppy disk */
#define CBUS_INT_HDD            CBUS_INT6   /**< Hard disk */

/**
 * @brief C-bus DMA channels
 *
 * PC-98 uses Intel 8237A-compatible DMA controller with 4 channels.
 */
typedef enum _CBUS_DMA_CHANNEL {
    CBUS_DMA0               = 0,    /**< DMA channel 0 */
    CBUS_DMA1               = 1,    /**< DMA channel 1 */
    CBUS_DMA2               = 2,    /**< DMA channel 2 */
    CBUS_DMA3               = 3,    /**< DMA channel 3 */
    CBUS_DMA_COUNT          = 4,    /**< Total DMA channels */
    CBUS_DMA_NONE           = 0xFF, /**< No DMA */
} CBUS_DMA_CHANNEL;

/**
 * @brief Standard DMA assignments
 */
#define CBUS_DMA_FLOPPY         CBUS_DMA2   /**< Floppy disk (typical) */
#define CBUS_DMA_SOUND          CBUS_DMA3   /**< Sound board (PC-9801-86) */

/**
 * @brief DMA transfer modes
 */
typedef enum _CBUS_DMA_MODE {
    CBUS_DMA_MODE_DEMAND    = 0x00, /**< Demand mode */
    CBUS_DMA_MODE_SINGLE    = 0x40, /**< Single transfer */
    CBUS_DMA_MODE_BLOCK     = 0x80, /**< Block transfer */
    CBUS_DMA_MODE_CASCADE   = 0xC0, /**< Cascade mode */
} CBUS_DMA_MODE;

/**
 * @brief DMA transfer direction
 */
typedef enum _CBUS_DMA_DIRECTION {
    CBUS_DMA_READ           = 0,    /**< Device to memory */
    CBUS_DMA_WRITE          = 1,    /**< Memory to device */
    CBUS_DMA_VERIFY         = 2,    /**< Verify mode */
} CBUS_DMA_DIRECTION;

/**
 * @brief Card categories
 */
typedef enum _CBUS_CARD_CATEGORY {
    CBUS_CAT_UNKNOWN        = 0,    /**< Unknown category */
    CBUS_CAT_GRAPHICS       = 1,    /**< Graphics/display cards */
    CBUS_CAT_SOUND          = 2,    /**< Sound cards */
    CBUS_CAT_NETWORK        = 3,    /**< Network adapters */
    CBUS_CAT_STORAGE        = 4,    /**< Storage controllers */
    CBUS_CAT_INTERFACE      = 5,    /**< I/O interfaces (serial, parallel) */
    CBUS_CAT_MEMORY         = 6,    /**< Memory expansion */
    CBUS_CAT_MODEM          = 7,    /**< Modems */
    CBUS_CAT_SCSI           = 8,    /**< SCSI controllers */
    CBUS_CAT_MULTIMEDIA     = 9,    /**< Multimedia (video capture, etc.) */
    CBUS_CAT_ACCELERATOR    = 10,   /**< CPU/graphics accelerators */
} CBUS_CARD_CATEGORY;

//=============================================================================
// C-bus Structures
//=============================================================================

/**
 * @brief C-bus I/O port range
 */
typedef struct _CBUS_IO_RANGE {
    UINT16      wBase;              /**< Base I/O address */
    UINT16      wLength;            /**< Length in ports */
    UINT16      wAlignment;         /**< Address alignment requirement */
    BOOLEAN     bDecode16Bit;       /**< 16-bit decode (vs 10-bit) */
} CBUS_IO_RANGE;

/**
 * @brief C-bus memory range
 */
typedef struct _CBUS_MEMORY_RANGE {
    UINT32      dwBase;             /**< Base physical address */
    UINT32      dwLength;           /**< Length in bytes */
    UINT32      dwAlignment;        /**< Address alignment */
    BOOLEAN     bWriteable;         /**< Memory is writeable */
    BOOLEAN     bCacheable;         /**< Memory is cacheable */
    BOOLEAN     bPrefetchable;      /**< Memory is prefetchable */
} CBUS_MEMORY_RANGE;

/**
 * @brief C-bus interrupt resource
 */
typedef struct _CBUS_IRQ {
    CBUS_IRQ_LEVEL  Level;          /**< Interrupt level (INT0-INT6) */
    BOOLEAN         bShared;        /**< Can be shared */
    BOOLEAN         bEdgeTriggered; /**< Edge-triggered (always TRUE for C-bus) */
} CBUS_IRQ;

/**
 * @brief C-bus DMA resource
 */
typedef struct _CBUS_DMA {
    CBUS_DMA_CHANNEL    Channel;    /**< DMA channel (0-3) */
    CBUS_DMA_MODE       Mode;       /**< Transfer mode */
    CBUS_DMA_DIRECTION  Direction;  /**< Transfer direction */
    BOOLEAN             bBusMaster; /**< Bus master capable */
    UINT16              wMaxTransfer; /**< Maximum transfer size */
} CBUS_DMA;

/**
 * @brief C-bus card information
 */
typedef struct _CBUS_CARD_INFO {
    CBUS_TYPE           BusType;        /**< Bus type this card supports */
    UINT8               uSlot;          /**< Slot number (0-5) */
    CBUS_CARD_CATEGORY  Category;       /**< Card category */

    CHAR8               szVendor[64];   /**< Vendor name */
    CHAR8               szModel[64];    /**< Model name */
    CHAR8               szDescription[128]; /**< Description */
    CHAR8               szPartNumber[32]; /**< Part number */

    UINT16              wVendorID;      /**< Vendor ID (if available) */
    UINT16              wDeviceID;      /**< Device ID (if available) */
    UINT16              wRevision;      /**< Hardware revision */

    // Resources
    CBUS_IO_RANGE       IOPorts[8];     /**< I/O port ranges */
    UINT32              uIOCount;       /**< Number of I/O ranges */

    CBUS_MEMORY_RANGE   Memory[4];      /**< Memory ranges */
    UINT32              uMemoryCount;   /**< Number of memory ranges */

    CBUS_IRQ            IRQs[2];        /**< Interrupt resources */
    UINT32              uIRQCount;      /**< Number of IRQs */

    CBUS_DMA            DMAs[2];        /**< DMA resources */
    UINT32              uDMACount;      /**< Number of DMA channels */

    BOOLEAN             bPresent;       /**< Card is present */
    BOOLEAN             bEnabled;       /**< Card is enabled */
    BOOLEAN             bCompatible16Bit; /**< 16-bit compatible */
    BOOLEAN             bPCH98Compatible; /**< PC-H98 compatible */
} CBUS_CARD_INFO;

/**
 * @brief PC-H98 bus information
 */
typedef struct _PCH98_BUS_INFO {
    UINT32      dwClockSpeed;           /**< Bus clock in Hz */
    UINT8       uBusWidth;              /**< Bus width (16/32 bits) */
    BOOLEAN     bBurstMode;             /**< Burst mode supported */
    BOOLEAN     bPCIBridge;             /**< PCI bridge present */
    UINT8       uSlotCount;             /**< Number of slots */
} PCH98_BUS_INFO;

/**
 * @brief C-bus device database entry
 */
typedef struct _CBUS_DEVICE_DB_ENTRY {
    UINT16              wVendorID;      /**< Vendor ID */
    UINT16              wDeviceID;      /**< Device ID */
    CBUS_CARD_CATEGORY  Category;       /**< Category */
    CONST CHAR8         *pszVendor;     /**< Vendor name */
    CONST CHAR8         *pszModel;      /**< Model name */
    CONST CHAR8         *pszDescription; /**< Description */
    UINT16              wIOBase;        /**< Default I/O base */
    CBUS_IRQ_LEVEL      DefaultIRQ;     /**< Default interrupt */
    CBUS_DMA_CHANNEL    DefaultDMA;     /**< Default DMA channel */
} CBUS_DEVICE_DB_ENTRY;

//=============================================================================
// PC-98 Graphics Structures
//=============================================================================

/**
 * @brief GDC (Graphics Display Controller) modes
 */
typedef enum _CBUS_GDC_MODE {
    GDC_MODE_TEXT       = 0,    /**< Text mode */
    GDC_MODE_GRAPHICS   = 1,    /**< Graphics mode */
    GDC_MODE_MIXED      = 2,    /**< Mixed text/graphics */
} CBUS_GDC_MODE;

/**
 * @brief Graphics resolution modes
 */
typedef enum _CBUS_GRAPHICS_MODE {
    CBUS_GFX_640x200    = 0,    /**< 640x200 (8 colors) */
    CBUS_GFX_640x400    = 1,    /**< 640x400 (8/16 colors) */
    CBUS_GFX_640x480    = 2,    /**< 640x480 (16 colors) */
    CBUS_GFX_800x600    = 3,    /**< 800x600 (16 colors, high-res) */
    CBUS_GFX_1024x768   = 4,    /**< 1024x768 (256 colors, PC-H98) */
} CBUS_GRAPHICS_MODE;

/**
 * @brief GRCG (Graphic Charger) operations
 */
typedef enum _CBUS_GRCG_MODE {
    GRCG_OFF            = 0,    /**< GRCG disabled */
    GRCG_RMW            = 1,    /**< Read-Modify-Write */
    GRCG_TDWR           = 2,    /**< TDW (Tile Data Write) */
    GRCG_TDW            = 3,    /**< TDW mode */
} CBUS_GRCG_MODE;

//=============================================================================
// Forward Declarations
//=============================================================================

typedef struct IIOCBusBus IIOCBusBus;
typedef struct IIOCBusDevice IIOCBusDevice;

//=============================================================================
// IIOCBusBus Interface
//=============================================================================

/**
 * @brief C-bus controller interface
 */
struct IIOCBusBus {
    IIOService  Base;

    /**
     * @brief Get bus type and capabilities
     */
    IO_RETURN (*GetBusInfo)(
        IIOCBusBus      *this,
        CBUS_TYPE       *pBusType,
        UINT32          *pdwClockSpeed
    );

    /**
     * @brief Detect installed cards
     */
    IO_RETURN (*DetectCards)(
        IIOCBusBus      *this,
        UINT32          *puCardCount
    );

    /**
     * @brief Get slot information
     */
    IO_RETURN (*GetSlotInfo)(
        IIOCBusBus      *this,
        UINT8           uSlot,
        CBUS_CARD_INFO  *pCardInfo
    );

    /**
     * @brief Enable/disable slot
     */
    IO_RETURN (*EnableSlot)(
        IIOCBusBus      *this,
        UINT8           uSlot,
        BOOLEAN         bEnable
    );

    /**
     * @brief Allocate I/O port range
     */
    IO_RETURN (*AllocateIO)(
        IIOCBusBus          *this,
        CONST CBUS_IO_RANGE *pRange
    );

    /**
     * @brief Free I/O port range
     */
    IO_RETURN (*FreeIO)(
        IIOCBusBus          *this,
        CONST CBUS_IO_RANGE *pRange
    );

    /**
     * @brief Allocate memory range
     */
    IO_RETURN (*AllocateMemory)(
        IIOCBusBus              *this,
        CONST CBUS_MEMORY_RANGE *pRange
    );

    /**
     * @brief Free memory range
     */
    IO_RETURN (*FreeMemory)(
        IIOCBusBus              *this,
        CONST CBUS_MEMORY_RANGE *pRange
    );

    /**
     * @brief Allocate interrupt
     */
    IO_RETURN (*AllocateIRQ)(
        IIOCBusBus      *this,
        CONST CBUS_IRQ  *pIRQ
    );

    /**
     * @brief Free interrupt
     */
    IO_RETURN (*FreeIRQ)(
        IIOCBusBus      *this,
        CONST CBUS_IRQ  *pIRQ
    );

    /**
     * @brief Allocate DMA channel
     */
    IO_RETURN (*AllocateDMA)(
        IIOCBusBus      *this,
        CONST CBUS_DMA  *pDMA
    );

    /**
     * @brief Free DMA channel
     */
    IO_RETURN (*FreeDMA)(
        IIOCBusBus      *this,
        CONST CBUS_DMA  *pDMA
    );

    /**
     * @brief Configure interrupt controller
     */
    IO_RETURN (*ConfigurePIC)(
        IIOCBusBus      *this,
        CBUS_IRQ_LEVEL  Level,
        BOOLEAN         bEnable
    );

    /**
     * @brief Configure DMA controller
     */
    IO_RETURN (*ConfigureDMA)(
        IIOCBusBus          *this,
        CBUS_DMA_CHANNEL    Channel,
        CONST CBUS_DMA      *pDMA
    );

    /**
     * @brief Enumerate all cards
     */
    IO_RETURN (*EnumerateCards)(
        IIOCBusBus      *this,
        IIOCBusDevice   ***pppDevices,
        UINT32          *puCount
    );

    /**
     * @brief Get PC-H98 extended information
     */
    IO_RETURN (*GetPCH98Info)(
        IIOCBusBus      *this,
        PCH98_BUS_INFO  *pInfo
    );
};

//=============================================================================
// IIOCBusDevice Interface
//=============================================================================

/**
 * @brief C-bus card device interface
 */
struct IIOCBusDevice {
    IIOService  Base;

    /**
     * @brief Get slot number
     */
    IO_RETURN (*GetSlot)(
        IIOCBusDevice   *this,
        UINT8           *puSlot
    );

    /**
     * @brief Get card information
     */
    IO_RETURN (*GetCardInfo)(
        IIOCBusDevice   *this,
        CBUS_CARD_INFO  *pInfo
    );

    /**
     * @brief Read from I/O port
     */
    IO_RETURN (*ReadPort)(
        IIOCBusDevice   *this,
        UINT16          wPort,
        UINT8           uSize,
        UINT32          *pdwValue
    );

    /**
     * @brief Write to I/O port
     */
    IO_RETURN (*WritePort)(
        IIOCBusDevice   *this,
        UINT16          wPort,
        UINT8           uSize,
        UINT32          dwValue
    );

    /**
     * @brief Read card memory
     */
    IO_RETURN (*ReadMemory)(
        IIOCBusDevice   *this,
        UINT32          dwOffset,
        VOID            *pBuffer,
        UINT32          dwLength
    );

    /**
     * @brief Write card memory
     */
    IO_RETURN (*WriteMemory)(
        IIOCBusDevice   *this,
        UINT32          dwOffset,
        CONST VOID      *pBuffer,
        UINT32          dwLength
    );

    /**
     * @brief Enable interrupt
     */
    IO_RETURN (*EnableInterrupt)(
        IIOCBusDevice   *this,
        CBUS_IRQ_LEVEL  Level,
        VOID            (*pfnHandler)(VOID *pContext),
        VOID            *pContext
    );

    /**
     * @brief Disable interrupt
     */
    IO_RETURN (*DisableInterrupt)(
        IIOCBusDevice   *this,
        CBUS_IRQ_LEVEL  Level
    );

    /**
     * @brief Setup DMA transfer
     */
    IO_RETURN (*SetupDMATransfer)(
        IIOCBusDevice       *this,
        CBUS_DMA_CHANNEL    Channel,
        VOID                *pBuffer,
        UINT32              dwLength,
        CBUS_DMA_DIRECTION  Direction
    );

    /**
     * @brief Start DMA transfer
     */
    IO_RETURN (*StartDMA)(
        IIOCBusDevice       *this,
        CBUS_DMA_CHANNEL    Channel
    );

    /**
     * @brief Stop DMA transfer
     */
    IO_RETURN (*StopDMA)(
        IIOCBusDevice       *this,
        CBUS_DMA_CHANNEL    Channel
    );

    /**
     * @brief Enable card
     */
    IO_RETURN (*Enable)(
        IIOCBusDevice   *this
    );

    /**
     * @brief Disable card
     */
    IO_RETURN (*Disable)(
        IIOCBusDevice   *this
    );

    /**
     * @brief Reset card
     */
    IO_RETURN (*Reset)(
        IIOCBusDevice   *this
    );
};

//=============================================================================
// Helper Macros
//=============================================================================

/**
 * @brief Check if slot number is valid
 */
#define CBUS_SLOT_IS_VALID(slot) \
    ((slot) >= CBUS_SLOT_MIN && (slot) <= CBUS_SLOT_MAX)

/**
 * @brief Check if interrupt level is valid
 */
#define CBUS_IRQ_IS_VALID(irq) \
    ((irq) >= CBUS_INT0 && (irq) < CBUS_INT_COUNT)

/**
 * @brief Check if DMA channel is valid
 */
#define CBUS_DMA_IS_VALID(dma) \
    ((dma) >= CBUS_DMA0 && (dma) < CBUS_DMA_COUNT)

/**
 * @brief Make vendor/device ID
 */
#define CBUS_MAKE_ID(vendor, device) \
    ((UINT32)(((vendor) << 16) | (device)))

//=============================================================================
// Function Declarations
//=============================================================================

/**
 * @brief Initialize C-bus subsystem
 */
IO_RETURN IOCBusInitialize(VOID);

/**
 * @brief Shutdown C-bus subsystem
 */
IO_RETURN IOCBusShutdown(VOID);

/**
 * @brief Get C-bus instance
 */
IO_RETURN IOCBusGetBus(IIOCBusBus **ppBus);

/**
 * @brief Detect if C-bus is present in system
 */
IO_RETURN IOCBusDetect(BOOLEAN *pbPresent, CBUS_TYPE *pBusType);

/**
 * @brief Get card database entry
 */
IO_RETURN IOCBusGetCardInfo(
    UINT16                          wVendorID,
    UINT16                          wDeviceID,
    CONST CBUS_DEVICE_DB_ENTRY      **ppEntry
);

/**
 * @brief Probe I/O port for card presence
 */
IO_RETURN IOCBusProbeIOPort(
    UINT16      wPort,
    BOOLEAN     *pbPresent
);

/**
 * @brief Get default resources for known card
 */
IO_RETURN IOCBusGetDefaultResources(
    CONST CHAR8         *pszModel,
    CBUS_IO_RANGE       *pIORange,
    CBUS_IRQ_LEVEL      *pIRQ,
    CBUS_DMA_CHANNEL    *pDMA
);

#ifdef __cplusplus
}
#endif

#endif // IOKIT_CBUS_H
