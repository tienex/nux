/**
 * @file unibus.h
 * @brief UNIBUS Family Interface - DEC PDP-11 System Bus
 *
 * This header defines the UNIBUS family interface for Digital Equipment
 * Corporation's UNIBUS architecture used in PDP-11 systems (1970-1997).
 *
 * UNIBUS characteristics:
 * - 18-bit addressing (256 KB address space)
 * - 16-bit data bus
 * - Asynchronous bus operation
 * - Memory-mapped I/O (top 8 KB for I/O devices)
 * - Interrupt vector system (256 vectors, each 2 words)
 * - DMA via NPR/NPG (Non-Processor Request/Grant)
 * - Priority interrupt levels (BR4-BR7)
 * - 40-56 signal lines on bus
 * - Maximum transfer rate: 1.5 MB/s
 *
 * Supported systems:
 * - PDP-11/20, 11/15, 11/10
 * - PDP-11/40, 11/35, 11/05
 * - PDP-11/45, 11/50, 11/55
 * - PDP-11/70, 11/44, 11/60
 * - PDP-11/34, 11/04, 11/24
 * - VAX-11/780, 11/750 (UNIBUS adapters)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_UNIBUS_H
#define IOKIT_UNIBUS_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Interface GUIDs
//=============================================================================

/**
 * @brief IIOUNIBusBus interface GUID
 * {D1E2C3B4-A5F6-7A8B-9C0D-1E2F3A4B5C6D}
 */
DEFINE_GUID(IID_IIOUNIBusBus,
    0xD1E2C3B4, 0xA5F6, 0x7A8B, 0x9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D);

/**
 * @brief IIOUNIBusDevice interface GUID
 * {E2F3D4C5-B6A7-8B9C-0D1E-2F3A4B5C6D7E}
 */
DEFINE_GUID(IID_IIOUNIBusDevice,
    0xE2F3D4C5, 0xB6A7, 0x8B9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E);

//=============================================================================
// UNIBUS Constants
//=============================================================================

/**
 * @brief UNIBUS address space
 */
#define UNIBUS_ADDR_BITS        18          /**< 18-bit addressing */
#define UNIBUS_ADDR_SPACE       0x40000     /**< 256 KB total (262,144 bytes) */
#define UNIBUS_ADDR_MASK        0x3FFFF     /**< Address mask */

/**
 * @brief UNIBUS I/O page (top 8 KB)
 */
#define UNIBUS_IOPAGE_BASE      0x160000    /**< I/O page base address */
#define UNIBUS_IOPAGE_SIZE      0x2000      /**< 8 KB I/O page */
#define UNIBUS_IOPAGE_END       0x17FFFF    /**< End of I/O page */

/**
 * @brief Common device address ranges
 */
#define UNIBUS_IO_BASE          0x760000    /**< I/O device base (8-bit mode) */
#define UNIBUS_IO_SIZE          0x2000      /**< I/O device space size */

/**
 * @brief UNIBUS data width
 */
#define UNIBUS_DATA_BITS        16          /**< 16-bit data bus */
#define UNIBUS_DATA_MASK        0xFFFF      /**< Data mask */

/**
 * @brief Interrupt vectors
 */
#define UNIBUS_VECTOR_MIN       0           /**< Minimum vector number */
#define UNIBUS_VECTOR_MAX       255         /**< Maximum vector number */
#define UNIBUS_VECTOR_SIZE      4           /**< Each vector is 2 words (4 bytes) */
#define UNIBUS_VECTOR_BASE      0x000000    /**< Vector table at address 0 */

/**
 * @brief Common interrupt vectors
 */
#define UNIBUS_VEC_TIMEOUT      0x004       /**< Bus timeout */
#define UNIBUS_VEC_ILLEGALINST  0x010       /**< Illegal instruction */
#define UNIBUS_VEC_BPT          0x014       /**< Breakpoint */
#define UNIBUS_VEC_IOT          0x020       /**< IOT instruction */
#define UNIBUS_VEC_POWERFAIL    0x024       /**< Power fail */
#define UNIBUS_VEC_EMT          0x030       /**< EMT instruction */
#define UNIBUS_VEC_TRAP         0x034       /**< TRAP instruction */
#define UNIBUS_VEC_PIRQ         0x240       /**< Programmable interrupt */
#define UNIBUS_VEC_FP11         0x244       /**< Floating point */

/**
 * @brief Bus request levels (priority interrupts)
 */
#define UNIBUS_BR4              4           /**< Bus Request level 4 */
#define UNIBUS_BR5              5           /**< Bus Request level 5 */
#define UNIBUS_BR6              6           /**< Bus Request level 6 */
#define UNIBUS_BR7              7           /**< Bus Request level 7 (highest) */

/**
 * @brief DMA modes
 */
#define UNIBUS_DMA_NPR          0x01        /**< Non-Processor Request */
#define UNIBUS_DMA_BLOCK        0x02        /**< Block mode transfer */

/**
 * @brief Transfer rates
 */
#define UNIBUS_TRANSFER_RATE    1500000     /**< 1.5 MB/s maximum */

//=============================================================================
// Device Types and Categories
//=============================================================================

/**
 * @brief UNIBUS device categories
 */
typedef enum _UNIBUS_CATEGORY {
    UNIBUS_CAT_UNKNOWN          = 0x00,     /**< Unknown device */
    UNIBUS_CAT_DISK             = 0x01,     /**< Disk controller */
    UNIBUS_CAT_TAPE             = 0x02,     /**< Magnetic tape */
    UNIBUS_CAT_TERMINAL         = 0x03,     /**< Serial terminal */
    UNIBUS_CAT_PRINTER          = 0x04,     /**< Line printer */
    UNIBUS_CAT_PAPERTAPE        = 0x05,     /**< Paper tape reader/punch */
    UNIBUS_CAT_CARD             = 0x06,     /**< Card reader */
    UNIBUS_CAT_NETWORK          = 0x07,     /**< Network interface */
    UNIBUS_CAT_CLOCK            = 0x08,     /**< Real-time clock */
    UNIBUS_CAT_MEMORY           = 0x09,     /**< Memory controller */
    UNIBUS_CAT_INTERFACE        = 0x0A,     /**< General interface */
    UNIBUS_CAT_DISPLAY          = 0x0B,     /**< Graphics display */
    UNIBUS_CAT_MODEM            = 0x0C,     /**< Modem control */
    UNIBUS_CAT_SYNC_SERIAL      = 0x0D,     /**< Synchronous serial */
} UNIBUS_CATEGORY;

/**
 * @brief Device transfer modes
 */
typedef enum _UNIBUS_TRANSFER_MODE {
    UNIBUS_MODE_PROGRAMMED_IO   = 0,        /**< Programmed I/O */
    UNIBUS_MODE_DMA             = 1,        /**< DMA transfer */
    UNIBUS_MODE_BLOCK_DMA       = 2,        /**< Block DMA */
} UNIBUS_TRANSFER_MODE;

//=============================================================================
// UNIBUS Structures
//=============================================================================

/**
 * @brief UNIBUS device address assignment
 */
typedef struct _UNIBUS_DEVICE_ADDR {
    UINT32      uCSRBase;               /**< Control/Status register base */
    UINT32      uCSRSize;               /**< CSR space size in bytes */
    UINT16      uVector;                /**< Interrupt vector address */
    UINT8       uBRLevel;               /**< Bus Request level (4-7) */
    BOOLEAN     bFloating;              /**< TRUE if floating address */
} UNIBUS_DEVICE_ADDR;

/**
 * @brief UNIBUS interrupt vector entry
 */
typedef struct _UNIBUS_VECTOR_ENTRY {
    UINT16      uPC;                    /**< Program Counter */
    UINT16      uPSW;                   /**< Processor Status Word */
} UNIBUS_VECTOR_ENTRY;

/**
 * @brief UNIBUS DMA transfer descriptor
 */
typedef struct _UNIBUS_DMA_DESC {
    UINT32      uMemoryAddr;            /**< Memory address (18-bit) */
    UINT16      uWordCount;             /**< Word count (negative) */
    UINT8       uMode;                  /**< Transfer mode */
    BOOLEAN     bWrite;                 /**< TRUE for memory write */
    BOOLEAN     bByteMode;              /**< TRUE for byte transfer */
} UNIBUS_DMA_DESC;

/**
 * @brief UNIBUS device information
 */
typedef struct _UNIBUS_DEVICE_INFO {
    CHAR8                   szName[64];         /**< Device name */
    CHAR8                   szModel[32];        /**< Model number */
    UNIBUS_CATEGORY         Category;           /**< Device category */
    UNIBUS_DEVICE_ADDR      Address;            /**< Address assignment */
    BOOLEAN                 bDMACapable;        /**< DMA support */
    BOOLEAN                 bBlockMode;         /**< Block transfer support */
    UINT32                  uMaxTransferSize;   /**< Maximum transfer (bytes) */
    UINT16                  uDeviceID;          /**< Device type ID */
    CHAR8                   szDescription[128]; /**< Description */
} UNIBUS_DEVICE_INFO;

/**
 * @brief UNIBUS configuration register (typical layout)
 */
typedef struct _UNIBUS_CSR {
    UINT16      uStatus;                /**< Status register */
    UINT16      uControl;               /**< Control register */
    UINT16      uData;                  /**< Data register */
    UINT16      uAddress;               /**< Address register */
} UNIBUS_CSR;

/**
 * @brief Common CSR bit definitions
 */
#define UNIBUS_CSR_ERR          0x8000      /**< Error flag */
#define UNIBUS_CSR_DONE         0x0080      /**< Operation complete */
#define UNIBUS_CSR_IE           0x0040      /**< Interrupt enable */
#define UNIBUS_CSR_READY        0x0080      /**< Device ready */
#define UNIBUS_CSR_GO           0x0001      /**< Start operation */

/**
 * @brief Bus timing parameters
 */
typedef struct _UNIBUS_TIMING {
    UINT32      uCycleTime;             /**< Bus cycle time (ns) */
    UINT32      uDataSetup;             /**< Data setup time (ns) */
    UINT32      uDataHold;              /**< Data hold time (ns) */
    UINT32      uArbitrationTime;       /**< Arbitration time (ns) */
} UNIBUS_TIMING;

//=============================================================================
// UNIBUS Device Database
//=============================================================================

/**
 * @brief Known UNIBUS device database entry
 */
typedef struct _UNIBUS_DEVICE_DB_ENTRY {
    UINT16              uDeviceID;      /**< Device type ID */
    CONST CHAR8         *pszName;       /**< Device name */
    CONST CHAR8         *pszModel;      /**< Model number */
    UNIBUS_CATEGORY     Category;       /**< Category */
    UINT32              uCSRBase;       /**< Standard CSR address */
    UINT32              uCSRSize;       /**< CSR size */
    UINT16              uVector;        /**< Standard vector */
    UINT8               uBRLevel;       /**< Bus request level */
    CONST CHAR8         *pszDescription;/**< Description */
} UNIBUS_DEVICE_DB_ENTRY;

//=============================================================================
// Common Device CSR Addresses
//=============================================================================

// Disk controllers
#define UNIBUS_RK11_CSR         0x777400    /**< RK11/RK05 disk */
#define UNIBUS_RK11_VEC         0x220       /**< RK11 vector */
#define UNIBUS_RL11_CSR         0x774400    /**< RL11/RL01/RL02 */
#define UNIBUS_RL11_VEC         0x160       /**< RL11 vector */
#define UNIBUS_RP11_CSR         0x776700    /**< RP11/RP02/RP03 */
#define UNIBUS_RP11_VEC         0x254       /**< RP11 vector */
#define UNIBUS_RH11_CSR         0x776700    /**< RH11 Massbus */
#define UNIBUS_RH11_VEC         0x254       /**< RH11 vector */
#define UNIBUS_RK611_CSR        0x777440    /**< RK611/RK06/RK07 */
#define UNIBUS_RK611_VEC        0x220       /**< RK611 vector */

// Tape controllers
#define UNIBUS_TM11_CSR         0x772520    /**< TM11/TU10 magtape */
#define UNIBUS_TM11_VEC         0x224       /**< TM11 vector */
#define UNIBUS_TC11_CSR         0x777340    /**< TC11/TU56 DECtape */
#define UNIBUS_TC11_VEC         0x214       /**< TC11 vector */
#define UNIBUS_TS11_CSR         0x772520    /**< TS11 tape */
#define UNIBUS_TS11_VEC         0x224       /**< TS11 vector */
#define UNIBUS_TU58_CSR         0x776500    /**< TU58 tape cartridge */
#define UNIBUS_TU58_VEC         0x300       /**< TU58 vector */

// Serial interfaces
#define UNIBUS_DL11_CSR         0x775610    /**< DL11 serial line */
#define UNIBUS_DL11_VEC         0x300       /**< DL11 vector */
#define UNIBUS_KL11_CSR         0x777560    /**< KL11 console */
#define UNIBUS_KL11_VEC         0x060       /**< KL11 vector */
#define UNIBUS_DZ11_CSR         0x760100    /**< DZ11 8-line mux */
#define UNIBUS_DZ11_VEC         0x300       /**< DZ11 vector */
#define UNIBUS_DH11_CSR         0x760020    /**< DH11 16-line mux */
#define UNIBUS_DH11_VEC         0x300       /**< DH11 vector */
#define UNIBUS_DJ11_CSR         0x760140    /**< DJ11 16-line mux */
#define UNIBUS_DJ11_VEC         0x310       /**< DJ11 vector */
#define UNIBUS_DU11_CSR         0x760130    /**< DU11 sync serial */
#define UNIBUS_DU11_VEC         0x320       /**< DU11 vector */
#define UNIBUS_DUP11_CSR        0x775400    /**< DUP11 sync serial */
#define UNIBUS_DUP11_VEC        0x330       /**< DUP11 vector */

// Printer/reader devices
#define UNIBUS_LP11_CSR         0x777514    /**< LP11 line printer */
#define UNIBUS_LP11_VEC         0x200       /**< LP11 vector */
#define UNIBUS_PC11_CSR         0x777550    /**< PC11 paper tape */
#define UNIBUS_PC11_VEC_READER  0x070       /**< PC11 reader vector */
#define UNIBUS_PC11_VEC_PUNCH   0x074       /**< PC11 punch vector */
#define UNIBUS_CR11_CSR         0x777160    /**< CR11 card reader */
#define UNIBUS_CR11_VEC         0x230       /**< CR11 vector */

// Network interfaces
#define UNIBUS_DMC11_CSR        0x760070    /**< DMC11 DDCMP link */
#define UNIBUS_DMC11_VEC        0x340       /**< DMC11 vector */
#define UNIBUS_DUV11_CSR        0x760340    /**< DUV11 sync serial */
#define UNIBUS_DUV11_VEC        0x350       /**< DUV11 vector */
#define UNIBUS_DEUNA_CSR        0x774510    /**< DEUNA Ethernet */
#define UNIBUS_DEUNA_VEC        0x120       /**< DEUNA vector */
#define UNIBUS_DMR11_CSR        0x760100    /**< DMR11 interprocessor */
#define UNIBUS_DMR11_VEC        0x360       /**< DMR11 vector */

// Clocks and timers
#define UNIBUS_KW11L_CSR        0x777546    /**< KW11-L line clock */
#define UNIBUS_KW11L_VEC        0x100       /**< KW11-L vector */
#define UNIBUS_KW11P_CSR        0x772540    /**< KW11-P prog clock */
#define UNIBUS_KW11P_VEC        0x104       /**< KW11-P vector */

// Memory management
#define UNIBUS_KT11B_CSR        0x772300    /**< KT11-B MMU */
#define UNIBUS_KT11C_CSR        0x777600    /**< KT11-C MMU */

// General interfaces
#define UNIBUS_DR11C_CSR        0x767770    /**< DR11-C interface */
#define UNIBUS_DR11C_VEC        0x370       /**< DR11-C vector */
#define UNIBUS_DR11B_CSR        0x772410    /**< DR11-B interface */
#define UNIBUS_DR11B_VEC        0x410       /**< DR11-B vector */
#define UNIBUS_DN11_CSR         0x775200    /**< DN11 auto-dialer */
#define UNIBUS_DN11_VEC         0x420       /**< DN11 vector */
#define UNIBUS_DM11_CSR         0x760150    /**< DM11 modem control */
#define UNIBUS_DM11_VEC         0x430       /**< DM11 vector */

// Display devices
#define UNIBUS_VT11_CSR         0x772000    /**< VT11 graphics display */
#define UNIBUS_VT11_VEC         0x440       /**< VT11 vector */
#define UNIBUS_GT40_CSR         0x772000    /**< GT40 graphics term */
#define UNIBUS_GT40_VEC         0x440       /**< GT40 vector */

//=============================================================================
// IIOUNIBusBus Interface
//=============================================================================

/**
 * @brief UNIBUS bus interface
 */
typedef struct IIOUNIBusBus {
    IIOService  Base;

    /**
     * @brief Detect installed devices
     */
    IO_RETURN (*DetectDevices)(
        struct IIOUNIBusBus *this,
        UINT32              *puDeviceCount
    );

    /**
     * @brief Enumerate all devices
     */
    IO_RETURN (*EnumerateDevices)(
        struct IIOUNIBusBus     *this,
        IIOUNIBusDevice         ***pppDevices,
        UINT32                  *puCount
    );

    /**
     * @brief Get device by CSR address
     */
    IO_RETURN (*GetDeviceByCSR)(
        struct IIOUNIBusBus     *this,
        UINT32                  uCSRAddr,
        IIOUNIBusDevice         **ppDevice
    );

    /**
     * @brief Get device by vector
     */
    IO_RETURN (*GetDeviceByVector)(
        struct IIOUNIBusBus     *this,
        UINT16                  uVector,
        IIOUNIBusDevice         **ppDevice
    );

    /**
     * @brief Read from UNIBUS address
     */
    IO_RETURN (*ReadWord)(
        struct IIOUNIBusBus *this,
        UINT32              uAddress,
        UINT16              *puValue
    );

    /**
     * @brief Write to UNIBUS address
     */
    IO_RETURN (*WriteWord)(
        struct IIOUNIBusBus *this,
        UINT32              uAddress,
        UINT16              uValue
    );

    /**
     * @brief Read byte from UNIBUS address
     */
    IO_RETURN (*ReadByte)(
        struct IIOUNIBusBus *this,
        UINT32              uAddress,
        UINT8               *puValue
    );

    /**
     * @brief Write byte to UNIBUS address
     */
    IO_RETURN (*WriteByte)(
        struct IIOUNIBusBus *this,
        UINT32              uAddress,
        UINT8               uValue
    );

    /**
     * @brief Setup interrupt vector
     */
    IO_RETURN (*SetupVector)(
        struct IIOUNIBusBus     *this,
        UINT16                  uVector,
        UNIBUS_VECTOR_ENTRY     *pEntry
    );

    /**
     * @brief Get interrupt vector
     */
    IO_RETURN (*GetVector)(
        struct IIOUNIBusBus     *this,
        UINT16                  uVector,
        UNIBUS_VECTOR_ENTRY     *pEntry
    );

    /**
     * @brief Enable interrupt level
     */
    IO_RETURN (*EnableInterrupt)(
        struct IIOUNIBusBus *this,
        UINT8               uBRLevel,
        BOOLEAN             bEnable
    );

    /**
     * @brief Setup DMA transfer
     */
    IO_RETURN (*SetupDMA)(
        struct IIOUNIBusBus     *this,
        UNIBUS_DMA_DESC         *pDesc,
        UINT32                  *puHandle
    );

    /**
     * @brief Start DMA transfer
     */
    IO_RETURN (*StartDMA)(
        struct IIOUNIBusBus *this,
        UINT32              uHandle
    );

    /**
     * @brief Abort DMA transfer
     */
    IO_RETURN (*AbortDMA)(
        struct IIOUNIBusBus *this,
        UINT32              uHandle
    );

    /**
     * @brief Get DMA status
     */
    IO_RETURN (*GetDMAStatus)(
        struct IIOUNIBusBus *this,
        UINT32              uHandle,
        UINT16              *puWordsTransferred
    );

    /**
     * @brief Request bus mastership (NPR)
     */
    IO_RETURN (*RequestBus)(
        struct IIOUNIBusBus *this,
        BOOLEAN             bBlock
    );

    /**
     * @brief Release bus mastership
     */
    IO_RETURN (*ReleaseBus)(
        struct IIOUNIBusBus *this
    );

    /**
     * @brief Assert INIT signal (bus reset)
     */
    IO_RETURN (*Reset)(
        struct IIOUNIBusBus *this
    );

    /**
     * @brief Get bus timing parameters
     */
    IO_RETURN (*GetTiming)(
        struct IIOUNIBusBus *this,
        UNIBUS_TIMING       *pTiming
    );
} IIOUNIBusBus;

//=============================================================================
// IIOUNIBusDevice Interface
//=============================================================================

/**
 * @brief UNIBUS device interface
 */
typedef struct IIOUNIBusDevice {
    IIOService  Base;

    /**
     * @brief Get device information
     */
    IO_RETURN (*GetDeviceInfo)(
        struct IIOUNIBusDevice  *this,
        UNIBUS_DEVICE_INFO      *pInfo
    );

    /**
     * @brief Get CSR base address
     */
    IO_RETURN (*GetCSRAddress)(
        struct IIOUNIBusDevice  *this,
        UINT32                  *puAddress
    );

    /**
     * @brief Get interrupt vector
     */
    IO_RETURN (*GetInterruptVector)(
        struct IIOUNIBusDevice  *this,
        UINT16                  *puVector
    );

    /**
     * @brief Read device register
     */
    IO_RETURN (*ReadRegister)(
        struct IIOUNIBusDevice  *this,
        UINT32                  uOffset,
        UINT16                  *puValue
    );

    /**
     * @brief Write device register
     */
    IO_RETURN (*WriteRegister)(
        struct IIOUNIBusDevice  *this,
        UINT32                  uOffset,
        UINT16                  uValue
    );

    /**
     * @brief Read device register (byte)
     */
    IO_RETURN (*ReadRegisterByte)(
        struct IIOUNIBusDevice  *this,
        UINT32                  uOffset,
        UINT8                   *puValue
    );

    /**
     * @brief Write device register (byte)
     */
    IO_RETURN (*WriteRegisterByte)(
        struct IIOUNIBusDevice  *this,
        UINT32                  uOffset,
        UINT8                   uValue
    );

    /**
     * @brief Enable device interrupt
     */
    IO_RETURN (*EnableInterrupt)(
        struct IIOUNIBusDevice  *this,
        VOID                    (*pfnHandler)(VOID *pContext),
        VOID                    *pContext
    );

    /**
     * @brief Disable device interrupt
     */
    IO_RETURN (*DisableInterrupt)(
        struct IIOUNIBusDevice  *this
    );

    /**
     * @brief Setup DMA transfer
     */
    IO_RETURN (*SetupDMATransfer)(
        struct IIOUNIBusDevice  *this,
        UINT32                  uMemoryAddr,
        UINT16                  uWordCount,
        BOOLEAN                 bWrite
    );

    /**
     * @brief Start DMA operation
     */
    IO_RETURN (*StartDMA)(
        struct IIOUNIBusDevice  *this
    );

    /**
     * @brief Abort DMA operation
     */
    IO_RETURN (*AbortDMA)(
        struct IIOUNIBusDevice  *this
    );

    /**
     * @brief Reset device
     */
    IO_RETURN (*Reset)(
        struct IIOUNIBusDevice  *this
    );

    /**
     * @brief Set transfer mode
     */
    IO_RETURN (*SetTransferMode)(
        struct IIOUNIBusDevice      *this,
        UNIBUS_TRANSFER_MODE        eMode
    );

    /**
     * @brief Get device status
     */
    IO_RETURN (*GetStatus)(
        struct IIOUNIBusDevice  *this,
        UINT16                  *puStatus
    );
} IIOUNIBusDevice;

//=============================================================================
// Helper Macros
//=============================================================================

/**
 * @brief Check if address is in I/O page
 */
#define UNIBUS_IS_IOPAGE(addr) \
    (((addr) >= UNIBUS_IOPAGE_BASE) && ((addr) <= UNIBUS_IOPAGE_END))

/**
 * @brief Align address to word boundary
 */
#define UNIBUS_ALIGN_WORD(addr) \
    ((addr) & ~0x01)

/**
 * @brief Check if address is word-aligned
 */
#define UNIBUS_IS_WORD_ALIGNED(addr) \
    (((addr) & 0x01) == 0)

/**
 * @brief Get byte offset within word
 */
#define UNIBUS_BYTE_OFFSET(addr) \
    ((addr) & 0x01)

/**
 * @brief Make vector entry
 */
#define UNIBUS_MAKE_VECTOR(pc, psw) \
    { .uPC = (pc), .uPSW = (psw) }

/**
 * @brief Check if BR level is valid
 */
#define UNIBUS_IS_VALID_BR(level) \
    (((level) >= UNIBUS_BR4) && ((level) <= UNIBUS_BR7))

//=============================================================================
// Function Declarations
//=============================================================================

/**
 * @brief Initialize UNIBUS subsystem
 */
IO_RETURN IOUNIBusInitialize(VOID);

/**
 * @brief Get UNIBUS bus instance
 */
IO_RETURN IOUNIBusGetBus(IIOUNIBusBus **ppBus);

/**
 * @brief Detect if UNIBUS is present in system
 */
IO_RETURN IOUNIBusDetect(BOOLEAN *pbPresent);

/**
 * @brief Get device database entry
 */
IO_RETURN IOUNIBusGetDeviceInfo(
    UINT16                          uDeviceID,
    CONST UNIBUS_DEVICE_DB_ENTRY    **ppEntry
);

/**
 * @brief Get device database entry by CSR address
 */
IO_RETURN IOUNIBusGetDeviceByCSR(
    UINT32                          uCSRAddr,
    CONST UNIBUS_DEVICE_DB_ENTRY    **ppEntry
);

/**
 * @brief Get device database entry by vector
 */
IO_RETURN IOUNIBusGetDeviceByVector(
    UINT16                          uVector,
    CONST UNIBUS_DEVICE_DB_ENTRY    **ppEntry
);

/**
 * @brief Convert 18-bit UNIBUS address to physical address
 */
UINT32 IOUNIBusToPhysical(UINT32 uUnibusAddr);

/**
 * @brief Convert physical address to UNIBUS address
 */
UINT32 IOPhysicalToUNIBus(UINT32 uPhysicalAddr);

#ifdef __cplusplus
}
#endif

#endif // IOKIT_UNIBUS_H
