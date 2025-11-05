/**
 * @file qbus.h
 * @brief Q-bus Family Interface - DEC PDP-11 and VAX Expansion Bus
 *
 * This header defines the Q-bus family interface for Digital Equipment
 * Corporation's expansion bus architecture used in PDP-11 and VAX systems.
 *
 * Q-bus is a 16-bit multiplexed address/data bus with:
 * - 16-bit, 18-bit, or 22-bit addressing (64KB, 256KB, or 4MB)
 * - Control Status Register (CSR) architecture
 * - 4 interrupt priority levels (BR4, BR5, BR6, BR7)
 * - DMA support with NPR (Non-Processor Request)
 * - Block mode transfers
 * - Vectored interrupts (64-1024 vector range)
 *
 * Supported systems:
 * - PDP-11/23, PDP-11/23+, PDP-11/53, PDP-11/73, PDP-11/83, PDP-11/93
 * - MicroVAX II, MicroVAX III, MicroVAX 3300, MicroVAX 3400, MicroVAX 3500
 * - VAXstation 2000, VAXstation 3100, VAXstation 3200, VAXstation 3500
 * - MicroPDP-11, Professional 325, Professional 350, Professional 380
 * - VAXserver 3100, VAXserver 3300, VAXserver 3400
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_QBUS_H
#define IOKIT_QBUS_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Interface GUIDs
//=============================================================================

/**
 * @brief IIOQBusBus interface GUID
 * {2D4A6F8B-1C3E-5A7B-9D0F-2E4A6C8D0E2F}
 */
DEFINE_GUID(IID_IIOQBusBus,
    0x2D4A6F8B, 0x1C3E, 0x5A7B, 0x9D, 0x0F, 0x2E, 0x4A, 0x6C, 0x8D, 0x0E, 0x2F);

/**
 * @brief IIOQBusDevice interface GUID
 * {3E5B7F9C-2D4F-6B8C-0E1F-3F5B7D9E1F3E}
 */
DEFINE_GUID(IID_IIOQBusDevice,
    0x3E5B7F9C, 0x2D4F, 0x6B8C, 0x0E, 0x1F, 0x3F, 0x5B, 0x7D, 0x9E, 0x1F, 0x3E);

//=============================================================================
// Q-bus Constants
//=============================================================================

/**
 * @brief Q-bus addressing modes
 */
#define QBUS_ADDR_16BIT         16      /**< 16-bit addressing (64KB) */
#define QBUS_ADDR_18BIT         18      /**< 18-bit addressing (256KB) */
#define QBUS_ADDR_22BIT         22      /**< 22-bit addressing (4MB) */

/**
 * @brief Q-bus address space limits
 */
#define QBUS_ADDR_MAX_16BIT     0x10000     /**< 64KB address space */
#define QBUS_ADDR_MAX_18BIT     0x40000     /**< 256KB address space */
#define QBUS_ADDR_MAX_22BIT     0x400000    /**< 4MB address space */

/**
 * @brief Q-bus I/O page (last 8KB of address space)
 */
#define QBUS_IOPAGE_SIZE        0x2000      /**< 8KB I/O page */
#define QBUS_IOPAGE_BASE_16     0xE000      /**< I/O page base (16-bit) */
#define QBUS_IOPAGE_BASE_18     0x3E000     /**< I/O page base (18-bit) */
#define QBUS_IOPAGE_BASE_22     0x3FE000    /**< I/O page base (22-bit) */

/**
 * @brief Q-bus interrupt priority levels
 */
#define QBUS_BR4                4       /**< Priority level 4 (lowest) */
#define QBUS_BR5                5       /**< Priority level 5 */
#define QBUS_BR6                6       /**< Priority level 6 */
#define QBUS_BR7                7       /**< Priority level 7 (highest) */

/**
 * @brief Q-bus interrupt vectors
 */
#define QBUS_VECTOR_MIN         0x40    /**< Minimum vector (64) */
#define QBUS_VECTOR_MAX         0x3FF   /**< Maximum vector (1023) */

/**
 * @brief Q-bus transfer modes
 */
#define QBUS_XFER_SINGLE        0       /**< Single word transfer */
#define QBUS_XFER_BLOCK         1       /**< Block mode transfer */
#define QBUS_XFER_DMA           2       /**< DMA transfer */

/**
 * @brief Q-bus data transfer rates
 */
#define QBUS_BANDWIDTH_CLASSIC  3600000     /**< 3.6 MB/s (classic Q-bus) */
#define QBUS_BANDWIDTH_FAST     13300000    /**< 13.3 MB/s (fast Q22) */

/**
 * @brief Common CSR addresses (in I/O page)
 */
#define QBUS_CSR_DL11_RCSR      0x7560  /**< DL11 receiver CSR */
#define QBUS_CSR_DL11_RBUF      0x7562  /**< DL11 receiver buffer */
#define QBUS_CSR_DL11_XCSR      0x7564  /**< DL11 transmitter CSR */
#define QBUS_CSR_DL11_XBUF      0x7566  /**< DL11 transmitter buffer */

#define QBUS_CSR_DZ11_CSR       0x7600  /**< DZ11 CSR */
#define QBUS_CSR_DZ11_RBUF      0x7602  /**< DZ11 receiver buffer */
#define QBUS_CSR_DZ11_TCR       0x7604  /**< DZ11 transmit control */
#define QBUS_CSR_DZ11_MSR       0x7606  /**< DZ11 modem status */

#define QBUS_CSR_RK11_RKDS      0x7400  /**< RK11 drive status */
#define QBUS_CSR_RK11_RKER      0x7402  /**< RK11 error register */
#define QBUS_CSR_RK11_RKCS      0x7404  /**< RK11 control status */
#define QBUS_CSR_RK11_RKWC      0x7406  /**< RK11 word count */
#define QBUS_CSR_RK11_RKBA      0x7410  /**< RK11 bus address */

#define QBUS_CSR_RL11_RLCS      0x7774  /**< RL11 control status */
#define QBUS_CSR_RL11_RLBA      0x7776  /**< RL11 bus address */

#define QBUS_CSR_RX11_RXCS      0x7770  /**< RX11 control status */
#define QBUS_CSR_RX11_RXDB      0x7772  /**< RX11 data buffer */

#define QBUS_CSR_TM11_MTS       0x7520  /**< TM11 status */
#define QBUS_CSR_TM11_MTC       0x7522  /**< TM11 command */
#define QBUS_CSR_TM11_MTBRC     0x7524  /**< TM11 byte record count */

#define QBUS_CSR_LP11_LPCS      0x7514  /**< LP11 control status */
#define QBUS_CSR_LP11_LPDB      0x7516  /**< LP11 data buffer */

#define QBUS_CSR_PC11_PRS       0x7550  /**< PC11 reader status */
#define QBUS_CSR_PC11_PRB       0x7552  /**< PC11 reader buffer */
#define QBUS_CSR_PC11_PPS       0x7554  /**< PC11 punch status */
#define QBUS_CSR_PC11_PPB       0x7556  /**< PC11 punch buffer */

//=============================================================================
// Q-bus Enumerations
//=============================================================================

/**
 * @brief Q-bus addressing mode
 */
typedef enum _QBUS_ADDR_MODE {
    QBUS_MODE_16BIT         = 0,    /**< 16-bit (64KB) addressing */
    QBUS_MODE_18BIT         = 1,    /**< 18-bit (256KB) addressing */
    QBUS_MODE_22BIT         = 2,    /**< 22-bit (4MB) addressing */
} QBUS_ADDR_MODE;

/**
 * @brief Q-bus transfer type
 */
typedef enum _QBUS_TRANSFER_TYPE {
    QBUS_XFER_TYPE_DATI     = 0,    /**< Read (data in) */
    QBUS_XFER_TYPE_DATO     = 1,    /**< Write (data out) */
    QBUS_XFER_TYPE_DATOB    = 2,    /**< Byte write */
    QBUS_XFER_TYPE_DATIP    = 3,    /**< Read-modify-write */
} QBUS_TRANSFER_TYPE;

/**
 * @brief Q-bus interrupt priority
 */
typedef enum _QBUS_PRIORITY {
    QBUS_PRIORITY_BR4       = 4,    /**< Priority level 4 (lowest) */
    QBUS_PRIORITY_BR5       = 5,    /**< Priority level 5 */
    QBUS_PRIORITY_BR6       = 6,    /**< Priority level 6 */
    QBUS_PRIORITY_BR7       = 7,    /**< Priority level 7 (highest) */
} QBUS_PRIORITY;

/**
 * @brief Q-bus device class
 */
typedef enum _QBUS_DEVICE_CLASS {
    QBUS_CLASS_UNKNOWN      = 0,    /**< Unknown device */
    QBUS_CLASS_SERIAL       = 1,    /**< Serial communications */
    QBUS_CLASS_PARALLEL     = 2,    /**< Parallel interface */
    QBUS_CLASS_DISK         = 3,    /**< Disk controller */
    QBUS_CLASS_TAPE         = 4,    /**< Tape controller */
    QBUS_CLASS_NETWORK      = 5,    /**< Network adapter */
    QBUS_CLASS_GRAPHICS     = 6,    /**< Graphics/display */
    QBUS_CLASS_MEMORY       = 7,    /**< Memory expansion */
    QBUS_CLASS_PROCESSOR    = 8,    /**< CPU/FPU */
    QBUS_CLASS_MULTIPORT    = 9,    /**< Multi-port serial */
    QBUS_CLASS_REALTIME     = 10,   /**< Real-time clock */
    QBUS_CLASS_LABORATORY   = 11,   /**< Lab I/O (ADC/DAC) */
} QBUS_DEVICE_CLASS;

/**
 * @brief Q-bus DMA mode
 */
typedef enum _QBUS_DMA_MODE {
    QBUS_DMA_SINGLE         = 0,    /**< Single word DMA */
    QBUS_DMA_BLOCK          = 1,    /**< Block mode DMA */
    QBUS_DMA_DEMAND         = 2,    /**< Demand mode DMA */
} QBUS_DMA_MODE;

//=============================================================================
// Q-bus Structures
//=============================================================================

/**
 * @brief Q-bus CSR (Control Status Register) location
 */
typedef struct _QBUS_CSR_INFO {
    UINT32      uBaseAddress;       /**< CSR base address in I/O page */
    UINT32      uSize;              /**< CSR space size in bytes */
    UINT16      uVectorBase;        /**< Interrupt vector base */
    UINT8       uVectorCount;       /**< Number of vectors */
    QBUS_PRIORITY Priority;         /**< Interrupt priority level */
} QBUS_CSR_INFO;

/**
 * @brief Q-bus interrupt configuration
 */
typedef struct _QBUS_INTERRUPT {
    UINT16      uVector;            /**< Interrupt vector */
    QBUS_PRIORITY Priority;         /**< Priority level (BR4-BR7) */
    BOOLEAN     bShared;            /**< Can be shared */
    VOID        (*pfnHandler)(VOID *pContext);  /**< Interrupt handler */
    VOID        *pContext;          /**< Handler context */
} QBUS_INTERRUPT;

/**
 * @brief Q-bus DMA configuration
 */
typedef struct _QBUS_DMA {
    UINT32      uAddress;           /**< DMA buffer address */
    UINT32      uLength;            /**< Transfer length in bytes */
    QBUS_DMA_MODE Mode;             /**< DMA mode */
    BOOLEAN     bWrite;             /**< TRUE for write, FALSE for read */
    UINT8       uBurstSize;         /**< Words per burst (0 = no limit) */
} QBUS_DMA;

/**
 * @brief Q-bus device information
 */
typedef struct _QBUS_DEVICE_INFO {
    CHAR8           szName[64];         /**< Device name */
    CHAR8           szVendor[64];       /**< Vendor name */
    CHAR8           szModel[64];        /**< Model number */
    CHAR8           szRevision[16];     /**< Revision/version */

    QBUS_DEVICE_CLASS Class;            /**< Device class */
    QBUS_ADDR_MODE  AddressMode;        /**< Addressing mode */

    QBUS_CSR_INFO   CSR;                /**< CSR information */

    UINT32          uMemoryBase;        /**< Memory base address (if any) */
    UINT32          uMemorySize;        /**< Memory size in bytes */

    BOOLEAN         bDMACapable;        /**< Supports DMA */
    BOOLEAN         bBlockMode;         /**< Supports block transfers */
    BOOLEAN         bBusMaster;         /**< Can be bus master */

    UINT16          uDeviceID;          /**< Device ID */
    UINT16          uSubsystemID;       /**< Subsystem ID */
} QBUS_DEVICE_INFO;

/**
 * @brief Q-bus configuration
 */
typedef struct _QBUS_CONFIG {
    QBUS_ADDR_MODE  AddressMode;        /**< Addressing mode */
    UINT32          uClockRate;         /**< Bus clock in Hz */
    UINT32          uBandwidth;         /**< Bandwidth in bytes/sec */
    BOOLEAN         bFastMode;          /**< Fast Q22 mode */
    BOOLEAN         bParityEnabled;     /**< Parity checking enabled */
    UINT8           uArbitrationLevel;  /**< Bus arbitration level */
} QBUS_CONFIG;

//=============================================================================
// Q-bus Device Database
//=============================================================================

/**
 * @brief Known Q-bus device database entry
 */
typedef struct _QBUS_CARD_DB_ENTRY {
    UINT16              uDeviceID;      /**< Device ID */
    CONST CHAR8         *pszVendor;     /**< Vendor name */
    CONST CHAR8         *pszName;       /**< Device name */
    CONST CHAR8         *pszDescription;/**< Description */
    QBUS_DEVICE_CLASS   Class;          /**< Device class */
    UINT32              uCSRBase;       /**< Default CSR base */
    UINT16              uVector;        /**< Default vector */
    QBUS_PRIORITY       Priority;       /**< Interrupt priority */
} QBUS_CARD_DB_ENTRY;

//=============================================================================
// Forward Declarations
//=============================================================================

typedef struct IIOQBusBus IIOQBusBus;
typedef struct IIOQBusDevice IIOQBusDevice;

//=============================================================================
// IIOQBusBus Interface
//=============================================================================

/**
 * @brief Q-bus bus controller interface
 */
struct IIOQBusBus {
    IIOService  Base;

    /**
     * @brief Get bus configuration
     */
    IO_RETURN (*GetConfig)(
        IIOQBusBus      *this,
        QBUS_CONFIG     *pConfig
    );

    /**
     * @brief Set bus configuration
     */
    IO_RETURN (*SetConfig)(
        IIOQBusBus      *this,
        CONST QBUS_CONFIG *pConfig
    );

    /**
     * @brief Detect installed devices
     */
    IO_RETURN (*DetectDevices)(
        IIOQBusBus      *this,
        UINT32          *puDeviceCount
    );

    /**
     * @brief Enumerate devices
     */
    IO_RETURN (*EnumerateDevices)(
        IIOQBusBus      *this,
        IIOQBusDevice   ***pppDevices,
        UINT32          *puCount
    );

    /**
     * @brief Read from Q-bus address
     */
    IO_RETURN (*ReadBus)(
        IIOQBusBus      *this,
        UINT32          uAddress,
        UINT16          *puValue
    );

    /**
     * @brief Write to Q-bus address
     */
    IO_RETURN (*WriteBus)(
        IIOQBusBus      *this,
        UINT32          uAddress,
        UINT16          uValue
    );

    /**
     * @brief Read byte from Q-bus address
     */
    IO_RETURN (*ReadByte)(
        IIOQBusBus      *this,
        UINT32          uAddress,
        UINT8           *puValue
    );

    /**
     * @brief Write byte to Q-bus address
     */
    IO_RETURN (*WriteByte)(
        IIOQBusBus      *this,
        UINT32          uAddress,
        UINT8           uValue
    );

    /**
     * @brief Read CSR register
     */
    IO_RETURN (*ReadCSR)(
        IIOQBusBus      *this,
        UINT32          uCSRAddress,
        UINT16          *puValue
    );

    /**
     * @brief Write CSR register
     */
    IO_RETURN (*WriteCSR)(
        IIOQBusBus      *this,
        UINT32          uCSRAddress,
        UINT16          uValue
    );

    /**
     * @brief Allocate interrupt vector
     */
    IO_RETURN (*AllocateVector)(
        IIOQBusBus      *this,
        QBUS_PRIORITY   Priority,
        UINT16          *puVector
    );

    /**
     * @brief Free interrupt vector
     */
    IO_RETURN (*FreeVector)(
        IIOQBusBus      *this,
        UINT16          uVector
    );

    /**
     * @brief Install interrupt handler
     */
    IO_RETURN (*InstallInterrupt)(
        IIOQBusBus      *this,
        CONST QBUS_INTERRUPT *pInterrupt
    );

    /**
     * @brief Remove interrupt handler
     */
    IO_RETURN (*RemoveInterrupt)(
        IIOQBusBus      *this,
        UINT16          uVector
    );

    /**
     * @brief Setup DMA transfer
     */
    IO_RETURN (*SetupDMA)(
        IIOQBusBus      *this,
        CONST QBUS_DMA  *pDMA
    );

    /**
     * @brief Start DMA transfer
     */
    IO_RETURN (*StartDMA)(
        IIOQBusBus      *this
    );

    /**
     * @brief Stop DMA transfer
     */
    IO_RETURN (*StopDMA)(
        IIOQBusBus      *this
    );

    /**
     * @brief Get DMA status
     */
    IO_RETURN (*GetDMAStatus)(
        IIOQBusBus      *this,
        UINT32          *puBytesTransferred,
        BOOLEAN         *pbComplete
    );

    /**
     * @brief Request bus mastership
     */
    IO_RETURN (*RequestBus)(
        IIOQBusBus      *this,
        UINT32          uTimeout
    );

    /**
     * @brief Release bus mastership
     */
    IO_RETURN (*ReleaseBus)(
        IIOQBusBus      *this
    );

    /**
     * @brief Block transfer (optimized multi-word)
     */
    IO_RETURN (*BlockTransfer)(
        IIOQBusBus      *this,
        UINT32          uAddress,
        UINT16          *pBuffer,
        UINT32          uWordCount,
        BOOLEAN         bWrite
    );

    /**
     * @brief Set addressing mode
     */
    IO_RETURN (*SetAddressMode)(
        IIOQBusBus      *this,
        QBUS_ADDR_MODE  Mode
    );

    /**
     * @brief Enable/disable parity checking
     */
    IO_RETURN (*SetParityMode)(
        IIOQBusBus      *this,
        BOOLEAN         bEnable
    );

    /**
     * @brief Get bus error status
     */
    IO_RETURN (*GetBusError)(
        IIOQBusBus      *this,
        UINT32          *puErrorAddress,
        UINT32          *puErrorFlags
    );

    /**
     * @brief Clear bus error
     */
    IO_RETURN (*ClearBusError)(
        IIOQBusBus      *this
    );
};

//=============================================================================
// IIOQBusDevice Interface
//=============================================================================

/**
 * @brief Q-bus device interface
 */
struct IIOQBusDevice {
    IIOService  Base;

    /**
     * @brief Get device information
     */
    IO_RETURN (*GetDeviceInfo)(
        IIOQBusDevice       *this,
        QBUS_DEVICE_INFO    *pInfo
    );

    /**
     * @brief Get CSR base address
     */
    IO_RETURN (*GetCSRBase)(
        IIOQBusDevice       *this,
        UINT32              *puBaseAddress
    );

    /**
     * @brief Read CSR register
     */
    IO_RETURN (*ReadCSR)(
        IIOQBusDevice       *this,
        UINT32              uOffset,
        UINT16              *puValue
    );

    /**
     * @brief Write CSR register
     */
    IO_RETURN (*WriteCSR)(
        IIOQBusDevice       *this,
        UINT32              uOffset,
        UINT16              uValue
    );

    /**
     * @brief Read device memory
     */
    IO_RETURN (*ReadMemory)(
        IIOQBusDevice       *this,
        UINT32              uOffset,
        VOID                *pBuffer,
        UINT32              uLength
    );

    /**
     * @brief Write device memory
     */
    IO_RETURN (*WriteMemory)(
        IIOQBusDevice       *this,
        UINT32              uOffset,
        CONST VOID          *pBuffer,
        UINT32              uLength
    );

    /**
     * @brief Enable device interrupts
     */
    IO_RETURN (*EnableInterrupts)(
        IIOQBusDevice       *this,
        VOID (*pfnHandler)(VOID *pContext),
        VOID                *pContext
    );

    /**
     * @brief Disable device interrupts
     */
    IO_RETURN (*DisableInterrupts)(
        IIOQBusDevice       *this
    );

    /**
     * @brief Setup device DMA
     */
    IO_RETURN (*SetupDMA)(
        IIOQBusDevice       *this,
        UINT32              uAddress,
        UINT32              uLength,
        BOOLEAN             bWrite
    );

    /**
     * @brief Start device DMA
     */
    IO_RETURN (*StartDMA)(
        IIOQBusDevice       *this
    );

    /**
     * @brief Stop device DMA
     */
    IO_RETURN (*StopDMA)(
        IIOQBusDevice       *this
    );

    /**
     * @brief Reset device
     */
    IO_RETURN (*Reset)(
        IIOQBusDevice       *this
    );

    /**
     * @brief Get interrupt vector
     */
    IO_RETURN (*GetVector)(
        IIOQBusDevice       *this,
        UINT16              *puVector
    );

    /**
     * @brief Set interrupt vector
     */
    IO_RETURN (*SetVector)(
        IIOQBusDevice       *this,
        UINT16              uVector
    );

    /**
     * @brief Get interrupt priority
     */
    IO_RETURN (*GetPriority)(
        IIOQBusDevice       *this,
        QBUS_PRIORITY       *pPriority
    );

    /**
     * @brief Set interrupt priority
     */
    IO_RETURN (*SetPriority)(
        IIOQBusDevice       *this,
        QBUS_PRIORITY       Priority
    );

    /**
     * @brief Enable device
     */
    IO_RETURN (*Enable)(
        IIOQBusDevice       *this
    );

    /**
     * @brief Disable device
     */
    IO_RETURN (*Disable)(
        IIOQBusDevice       *this
    );

    /**
     * @brief Perform self-test
     */
    IO_RETURN (*SelfTest)(
        IIOQBusDevice       *this,
        UINT32              *puResult
    );
};

//=============================================================================
// Helper Macros
//=============================================================================

/**
 * @brief Calculate I/O page address based on addressing mode
 */
#define QBUS_IOPAGE_BASE(mode) \
    ((mode) == QBUS_MODE_22BIT ? QBUS_IOPAGE_BASE_22 : \
     (mode) == QBUS_MODE_18BIT ? QBUS_IOPAGE_BASE_18 : QBUS_IOPAGE_BASE_16)

/**
 * @brief Check if address is in I/O page
 */
#define QBUS_IS_IOPAGE(addr, mode) \
    ((addr) >= QBUS_IOPAGE_BASE(mode) && \
     (addr) < (QBUS_IOPAGE_BASE(mode) + QBUS_IOPAGE_SIZE))

/**
 * @brief Make CSR address from I/O page offset
 */
#define QBUS_MAKE_CSR_ADDR(offset, mode) \
    (QBUS_IOPAGE_BASE(mode) + (offset))

/**
 * @brief Check if vector is valid
 */
#define QBUS_VECTOR_IS_VALID(vec) \
    ((vec) >= QBUS_VECTOR_MIN && (vec) <= QBUS_VECTOR_MAX && ((vec) & 0x3) == 0)

/**
 * @brief Check if priority level is valid
 */
#define QBUS_PRIORITY_IS_VALID(pri) \
    ((pri) >= QBUS_BR4 && (pri) <= QBUS_BR7)

//=============================================================================
// Function Declarations
//=============================================================================

/**
 * @brief Initialize Q-bus subsystem
 */
IO_RETURN IOQBusInitialize(VOID);

/**
 * @brief Shutdown Q-bus subsystem
 */
IO_RETURN IOQBusShutdown(VOID);

/**
 * @brief Get Q-bus bus instance
 */
IO_RETURN IOQBusGetBus(IIOQBusBus **ppBus);

/**
 * @brief Detect if Q-bus is present in system
 */
IO_RETURN IOQBusDetect(BOOLEAN *pbPresent);

/**
 * @brief Get device information from database
 */
IO_RETURN IOQBusGetDeviceInfo(
    UINT16                      uDeviceID,
    CONST QBUS_CARD_DB_ENTRY    **ppEntry
);

/**
 * @brief Create Q-bus device instance
 */
IO_RETURN IOQBusDeviceCreate(
    CONST CHAR8         *pszName,
    UINT32              uCSRBase,
    UINT16              uVector,
    QBUS_PRIORITY       Priority,
    IIOQBusDevice       **ppDevice
);

/**
 * @brief Probe Q-bus CSR space for device
 */
IO_RETURN IOQBusProbeCSR(
    UINT32              uCSRBase,
    BOOLEAN             *pbPresent,
    UINT16              *puDeviceID
);

/**
 * @brief Auto-configure Q-bus devices
 */
IO_RETURN IOQBusAutoConfigure(
    IIOQBusDevice       ***pppDevices,
    UINT32              *puCount
);

/**
 * @brief Map device name to device class
 */
QBUS_DEVICE_CLASS IOQBusGetDeviceClass(
    CONST CHAR8         *pszName
);

/**
 * @brief Convert CSR address to I/O page offset
 */
UINT32 IOQBusCSRToOffset(
    UINT32              uCSRAddress,
    QBUS_ADDR_MODE      Mode
);

/**
 * @brief Convert I/O page offset to CSR address
 */
UINT32 IOQBusOffsetToCSR(
    UINT32              uOffset,
    QBUS_ADDR_MODE      Mode
);

#ifdef __cplusplus
}
#endif

#endif // IOKIT_QBUS_H
