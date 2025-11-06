/**
 * @file vmebus.h
 * @brief VMEbus Family Interface - VME64/VME64x Expansion Bus
 *
 * This header defines the VMEbus family interface for VME64 and VME64x
 * expansion bus architectures used in industrial, military, and aerospace systems.
 *
 * VMEbus is a robust 32/64-bit computer bus standard with:
 * - VME64: 32-bit data, 32-bit address, 40 MB/s
 * - VME64x: 64-bit data, 64-bit address, 320 MB/s
 * - Multiple address modes: A16, A24, A32, A40, A64
 * - Block transfer modes: BLT, MBLT, 2eVME, 2eSST
 * - 7-level interrupt system (IRQ1-IRQ7)
 * - Geographical addressing support
 * - Up to 21 slots in standard chassis
 * - Hot-swap capability (VME64x)
 * - Fault-tolerant design
 * - Industrial temperature range support
 *
 * Supported standards:
 * - IEEE 1014-1987 (VMEbus)
 * - ANSI/VITA 1-1994 (VME64)
 * - ANSI/VITA 1.1-1997 (VME64 Extensions)
 * - ANSI/VITA 1.3-2008 (VME64x)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_VMEBUS_H
#define IOKIT_VMEBUS_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Interface GUIDs
//=============================================================================

/**
 * @brief IIOVMEBus interface GUID
 * {A1B2C3D4-5E6F-7A8B-9C0D-1E2F3A4B5C6D}
 */
DEFINE_GUID(IID_IIOVMEBus,
    0xA1B2C3D4, 0x5E6F, 0x7A8B, 0x9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D);

/**
 * @brief IIOVMEDevice interface GUID
 * {B2C3D4E5-6F7A-8B9C-0D1E-2F3A4B5C6D7E}
 */
DEFINE_GUID(IID_IIOVMEDevice,
    0xB2C3D4E5, 0x6F7A, 0x8B9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E);

//=============================================================================
// VMEbus Constants
//=============================================================================

/**
 * @brief VME slot configuration
 */
#define VME_SLOT_MIN            1       /**< First VME slot */
#define VME_SLOT_MAX            21      /**< Last VME slot (1-21 = 21 slots max) */
#define VME_MAX_SLOTS           21      /**< Maximum number of slots */
#define VME_SLOT_CONTROLLER     0       /**< Slot 0 = bus controller */

/**
 * @brief VME bus widths
 */
#define VME_DATA_WIDTH_8        8       /**< 8-bit data path */
#define VME_DATA_WIDTH_16       16      /**< 16-bit data path */
#define VME_DATA_WIDTH_32       32      /**< 32-bit data path (VME64) */
#define VME_DATA_WIDTH_64       64      /**< 64-bit data path (VME64x) */

/**
 * @brief VME address modifier codes
 */
#define VME_AM_A16_SUP_DATA     0x2D    /**< A16 supervisor data */
#define VME_AM_A16_SUP_PROG     0x2E    /**< A16 supervisor program */
#define VME_AM_A16_USR_DATA     0x29    /**< A16 user data */
#define VME_AM_A16_USR_PROG     0x2A    /**< A16 user program */

#define VME_AM_A24_SUP_DATA     0x3D    /**< A24 supervisor data */
#define VME_AM_A24_SUP_PROG     0x3E    /**< A24 supervisor program */
#define VME_AM_A24_SUP_BLT      0x3F    /**< A24 supervisor block */
#define VME_AM_A24_SUP_MBLT     0x3C    /**< A24 supervisor MBLT */
#define VME_AM_A24_USR_DATA     0x39    /**< A24 user data */
#define VME_AM_A24_USR_PROG     0x3A    /**< A24 user program */
#define VME_AM_A24_USR_BLT      0x3B    /**< A24 user block */
#define VME_AM_A24_USR_MBLT     0x38    /**< A24 user MBLT */

#define VME_AM_A32_SUP_DATA     0x0D    /**< A32 supervisor data */
#define VME_AM_A32_SUP_PROG     0x0E    /**< A32 supervisor program */
#define VME_AM_A32_SUP_BLT      0x0F    /**< A32 supervisor block */
#define VME_AM_A32_SUP_MBLT     0x0C    /**< A32 supervisor MBLT */
#define VME_AM_A32_SUP_2eVME    0x20    /**< A32 supervisor 2eVME */
#define VME_AM_A32_SUP_2eSST    0x21    /**< A32 supervisor 2eSST */
#define VME_AM_A32_USR_DATA     0x09    /**< A32 user data */
#define VME_AM_A32_USR_PROG     0x0A    /**< A32 user program */
#define VME_AM_A32_USR_BLT      0x0B    /**< A32 user block */
#define VME_AM_A32_USR_MBLT     0x08    /**< A32 user MBLT */
#define VME_AM_A32_USR_2eVME    0x21    /**< A32 user 2eVME */
#define VME_AM_A32_USR_2eSST    0x22    /**< A32 user 2eSST */

#define VME_AM_A40_SUP_DATA     0x11    /**< A40 supervisor data (VME64x) */
#define VME_AM_A40_USR_DATA     0x0A    /**< A40 user data (VME64x) */

#define VME_AM_A64_SUP_DATA     0x01    /**< A64 supervisor data (VME64x) */
#define VME_AM_A64_USR_DATA     0x00    /**< A64 user data (VME64x) */
#define VME_AM_A64_SUP_BLT      0x03    /**< A64 supervisor block */
#define VME_AM_A64_USR_BLT      0x02    /**< A64 user block */
#define VME_AM_A64_SUP_MBLT     0x04    /**< A64 supervisor MBLT */
#define VME_AM_A64_USR_MBLT     0x05    /**< A64 user MBLT */
#define VME_AM_A64_SUP_2eVME    0x20    /**< A64 supervisor 2eVME */
#define VME_AM_A64_USR_2eVME    0x21    /**< A64 user 2eVME */
#define VME_AM_A64_SUP_2eSST    0x21    /**< A64 supervisor 2eSST */
#define VME_AM_A64_USR_2eSST    0x22    /**< A64 user 2eSST */

/**
 * @brief VME CR/CSR address space
 */
#define VME_CSR_BASE            0x00000000  /**< CSR base address */
#define VME_CR_BASE             0x00000000  /**< CR base address */
#define VME_CR_SIZE             0x00080000  /**< 512 KB CR space */
#define VME_CSR_SIZE            0x00001000  /**< 4 KB CSR space */
#define VME_CSR_OFFSET          0x7FC00     /**< CSR offset in CR space */

/**
 * @brief VME CR/CSR register offsets
 */
#define VME_CSR_BAR             0x7FFFF     /**< Base Address Register */
#define VME_CSR_BIT_SET         0x7FFFB     /**< Bit Set Register */
#define VME_CSR_BIT_CLR         0x7FFF7     /**< Bit Clear Register */
#define VME_CSR_CRAM_OWNER      0x7FFF3     /**< CRAM Owner Register */
#define VME_CSR_USR_BIT_SET     0x7FFEF     /**< User Bit Set Register */
#define VME_CSR_USR_BIT_CLR     0x7FFEB     /**< User Bit Clear Register */

/**
 * @brief VME interrupt levels
 */
#define VME_IRQ_MIN             1           /**< Minimum IRQ level */
#define VME_IRQ_MAX             7           /**< Maximum IRQ level */
#define VME_IRQ_COUNT           7           /**< Number of IRQ levels */

/**
 * @brief VME transfer rates
 */
#define VME_XFER_RATE_STANDARD  40000000    /**< 40 MB/s (D32) */
#define VME_XFER_RATE_BLT       80000000    /**< 80 MB/s (D32 BLT) */
#define VME_XFER_RATE_MBLT      160000000   /**< 160 MB/s (D64 MBLT) */
#define VME_XFER_RATE_2eVME     160000000   /**< 160 MB/s (2eVME) */
#define VME_XFER_RATE_2eSST     320000000   /**< 320 MB/s (2eSST) */

/**
 * @brief VME 2eSST transfer rates
 */
#define VME_2eSST_160           0           /**< 160 MB/s */
#define VME_2eSST_267           1           /**< 267 MB/s */
#define VME_2eSST_320           2           /**< 320 MB/s */

/**
 * @brief VME geographical addressing
 */
#define VME_GA_NOT_AVAILABLE    0xFF        /**< GA not available */

/**
 * @brief VME card configuration ROM (CR) ID offsets
 */
#define VME_CR_IEEE_OUI         0x00        /**< IEEE OUI (24 bits) */
#define VME_CR_BOARD_ID         0x03        /**< Board ID (32 bits) */
#define VME_CR_REVISION         0x07        /**< Revision (32 bits) */
#define VME_CR_ASCII_PTR        0x0B        /**< ASCII string pointer */
#define VME_CR_PROGRAM_ID       0x0F        /**< Program ID code */
#define VME_CR_BEG_UCR          0x13        /**< Begin User CR */
#define VME_CR_END_UCR          0x17        /**< End User CR */
#define VME_CR_BEG_CRAM         0x1B        /**< Begin CRAM */
#define VME_CR_END_CRAM         0x1F        /**< End CRAM */
#define VME_CR_BEG_USR_CSR      0x23        /**< Begin User CSR */
#define VME_CR_END_USR_CSR      0x27        /**< End User CSR */

//=============================================================================
// VMEbus Enumerations
//=============================================================================

/**
 * @brief VME bus standards
 */
typedef enum _VME_BUS_STANDARD {
    VME_STANDARD_UNKNOWN        = 0,    /**< Unknown standard */
    VME_STANDARD_VME32          = 1,    /**< VME32 (IEEE 1014-1987) */
    VME_STANDARD_VME64          = 2,    /**< VME64 (ANSI/VITA 1-1994) */
    VME_STANDARD_VME64X         = 3,    /**< VME64x (ANSI/VITA 1.1-1997) */
    VME_STANDARD_VME64X_2008    = 4,    /**< VME64x (ANSI/VITA 1.3-2008) */
} VME_BUS_STANDARD;

/**
 * @brief VME address spaces
 */
typedef enum _VME_ADDRESS_SPACE {
    VME_ADDR_A16                = 16,   /**< 16-bit address (64 KB) */
    VME_ADDR_A24                = 24,   /**< 24-bit address (16 MB) */
    VME_ADDR_A32                = 32,   /**< 32-bit address (4 GB) */
    VME_ADDR_A40                = 40,   /**< 40-bit address (1 TB) VME64x */
    VME_ADDR_A64                = 64,   /**< 64-bit address (16 EB) VME64x */
    VME_ADDR_CR_CSR             = 128,  /**< CR/CSR space */
} VME_ADDRESS_SPACE;

/**
 * @brief VME data widths
 */
typedef enum _VME_DATA_WIDTH {
    VME_DATA_D08                = 8,    /**< 8-bit data */
    VME_DATA_D16                = 16,   /**< 16-bit data */
    VME_DATA_D32                = 32,   /**< 32-bit data */
    VME_DATA_D64                = 64,   /**< 64-bit data (VME64/VME64x) */
} VME_DATA_WIDTH;

/**
 * @brief VME transfer modes
 */
typedef enum _VME_TRANSFER_MODE {
    VME_XFER_SCT                = 0,    /**< Single Cycle Transfer */
    VME_XFER_BLT                = 1,    /**< Block Transfer */
    VME_XFER_MBLT               = 2,    /**< Multiplexed Block Transfer */
    VME_XFER_2eVME              = 3,    /**< 2-edge VME (DDR) */
    VME_XFER_2eSST              = 4,    /**< 2-edge Source Synchronous Transfer */
} VME_TRANSFER_MODE;

/**
 * @brief VME privilege levels
 */
typedef enum _VME_PRIVILEGE {
    VME_PRIV_USER               = 0,    /**< User mode */
    VME_PRIV_SUPERVISOR         = 1,    /**< Supervisor mode */
} VME_PRIVILEGE;

/**
 * @brief VME access types
 */
typedef enum _VME_ACCESS_TYPE {
    VME_ACCESS_DATA             = 0,    /**< Data access */
    VME_ACCESS_PROGRAM          = 1,    /**< Program access */
} VME_ACCESS_TYPE;

/**
 * @brief VME device classes
 */
typedef enum _VME_DEVICE_CLASS {
    VME_CLASS_UNKNOWN           = 0x00, /**< Unknown device */
    VME_CLASS_PROCESSOR         = 0x01, /**< CPU board */
    VME_CLASS_MEMORY            = 0x02, /**< Memory board */
    VME_CLASS_STORAGE           = 0x03, /**< Storage controller */
    VME_CLASS_NETWORK           = 0x04, /**< Network interface */
    VME_CLASS_DISPLAY           = 0x05, /**< Display controller */
    VME_CLASS_SERIAL            = 0x06, /**< Serial I/O */
    VME_CLASS_PARALLEL          = 0x07, /**< Parallel I/O */
    VME_CLASS_BRIDGE            = 0x08, /**< Bridge/Interface */
    VME_CLASS_INDUSTRIAL_IO     = 0x09, /**< Industrial I/O */
    VME_CLASS_INSTRUMENT        = 0x0A, /**< Instrumentation */
    VME_CLASS_DATA_ACQ          = 0x0B, /**< Data Acquisition */
    VME_CLASS_SIGNAL_PROC       = 0x0C, /**< Signal Processing */
    VME_CLASS_COMM              = 0x0D, /**< Communications */
    VME_CLASS_MILITARY          = 0x0E, /**< Military/Aerospace */
    VME_CLASS_SCIENTIFIC        = 0x0F, /**< Scientific */
} VME_DEVICE_CLASS;

/**
 * @brief VME interrupt acknowledge styles
 */
typedef enum _VME_IACK_STYLE {
    VME_IACK_RORA               = 0,    /**< Release On Register Access */
    VME_IACK_ROAK               = 1,    /**< Release On Acknowledge */
} VME_IACK_STYLE;

/**
 * @brief VME arbiter modes
 */
typedef enum _VME_ARBITER_MODE {
    VME_ARB_PRIORITY            = 0,    /**< Priority mode */
    VME_ARB_ROUND_ROBIN         = 1,    /**< Round-robin mode */
} VME_ARBITER_MODE;

/**
 * @brief VME bus request levels
 */
typedef enum _VME_BUS_REQ_LEVEL {
    VME_BR_LEVEL_0              = 0,    /**< Bus request level 0 */
    VME_BR_LEVEL_1              = 1,    /**< Bus request level 1 */
    VME_BR_LEVEL_2              = 2,    /**< Bus request level 2 */
    VME_BR_LEVEL_3              = 3,    /**< Bus request level 3 */
} VME_BUS_REQ_LEVEL;

//=============================================================================
// VMEbus Structures
//=============================================================================

/**
 * @brief VME address window
 */
typedef struct _VME_ADDRESS_WINDOW {
    VME_ADDRESS_SPACE   eAddressSpace;      /**< Address space (A16/A24/A32/A40/A64) */
    UINT64              u64BaseAddress;     /**< Base address on VME bus */
    UINT64              u64Size;            /**< Window size */
    UINT64              u64LocalAddress;    /**< Local mapped address */
    VME_DATA_WIDTH      eDataWidth;         /**< Data width */
    VME_TRANSFER_MODE   eTransferMode;      /**< Transfer mode */
    VME_PRIVILEGE       ePrivilege;         /**< Privilege level */
    VME_ACCESS_TYPE     eAccessType;        /**< Access type */
    UINT8               uAMCode;            /**< Address modifier code */
    BOOLEAN             bEnabled;           /**< Window enabled */
    BOOLEAN             bPrefetchable;      /**< Prefetchable */
    BOOLEAN             bPosted;            /**< Posted writes */
} VME_ADDRESS_WINDOW;

/**
 * @brief VME interrupt handler
 */
typedef struct _VME_INTERRUPT {
    UINT8               uLevel;             /**< IRQ level (1-7) */
    UINT8               uVector;            /**< Interrupt vector (0-255) */
    VME_IACK_STYLE      eStyle;             /**< IACK style */
    VOID                (*pfnHandler)(VOID *pContext, UINT8 uVector);
    VOID                *pContext;          /**< Handler context */
    BOOLEAN             bEnabled;           /**< Interrupt enabled */
} VME_INTERRUPT;

/**
 * @brief VME configuration ROM info
 */
typedef struct _VME_CR_INFO {
    UINT32              uOUI;               /**< IEEE OUI (24-bit) */
    UINT32              uBoardID;           /**< Board ID */
    UINT32              uRevision;          /**< Revision */
    UINT32              uProgramID;         /**< Program ID */
    CHAR8               szDescription[256]; /**< ASCII description */
    BOOLEAN             bValid;             /**< CR is valid */
} VME_CR_INFO;

/**
 * @brief VME device information
 */
typedef struct _VME_DEVICE_INFO {
    UINT8               uSlot;              /**< Slot number (1-21) */
    UINT8               uGeoAddr;           /**< Geographical address */
    VME_DEVICE_CLASS    eClass;             /**< Device class */
    VME_BUS_STANDARD    eStandard;          /**< Bus standard */

    // Device identification
    CHAR8               szName[64];         /**< Device name */
    CHAR8               szManufacturer[64]; /**< Manufacturer */
    CHAR8               szModel[64];        /**< Model */
    CHAR8               szSerial[32];       /**< Serial number */
    CHAR8               szDescription[128]; /**< Description */

    // Configuration ROM
    VME_CR_INFO         CRInfo;             /**< CR information */
    BOOLEAN             bHasCR;             /**< Has configuration ROM */

    // Address windows
    VME_ADDRESS_WINDOW  *pWindows;          /**< Address windows */
    UINT32              uWindowCount;       /**< Number of windows */

    // Interrupts
    VME_INTERRUPT       *pInterrupts;       /**< Interrupt handlers */
    UINT32              uInterruptCount;    /**< Number of interrupts */

    // Capabilities
    BOOLEAN             bVME64;             /**< VME64 capable */
    BOOLEAN             bVME64X;            /**< VME64x capable */
    BOOLEAN             bBusMaster;         /**< Bus master capable */
    BOOLEAN             bHotSwap;           /**< Hot-swap capable */
    BOOLEAN             b2eVME;             /**< 2eVME capable */
    BOOLEAN             b2eSST;             /**< 2eSST capable */
    UINT8               u2eSSTRate;         /**< 2eSST rate */

    // Bus request
    VME_BUS_REQ_LEVEL   eBusReqLevel;       /**< Bus request level */
    BOOLEAN             bBusReqEnabled;     /**< Bus request enabled */

    // Status
    BOOLEAN             bPresent;           /**< Card present */
    BOOLEAN             bEnabled;           /**< Device enabled */
    BOOLEAN             bError;             /**< Error state */
} VME_DEVICE_INFO;

/**
 * @brief VME slot information
 */
typedef struct _VME_SLOT_INFO {
    UINT8               uSlot;              /**< Slot number */
    UINT8               uGeoAddr;           /**< Geographical address */
    BOOLEAN             bOccupied;          /**< Card present */
    BOOLEAN             bEnabled;           /**< Slot enabled */
    BOOLEAN             bHotSwap;           /**< Hot-swap capable */
    VME_DEVICE_INFO     DeviceInfo;         /**< Device info (if occupied) */
} VME_SLOT_INFO;

/**
 * @brief VME bus controller information
 */
typedef struct _VME_BUS_INFO {
    VME_BUS_STANDARD    eStandard;          /**< Bus standard */
    UINT8               uSlotCount;         /**< Number of slots */
    UINT32              uClockFreq;         /**< Bus clock frequency */

    // Capabilities
    BOOLEAN             bVME64;             /**< VME64 support */
    BOOLEAN             bVME64X;            /**< VME64x support */
    BOOLEAN             bHotSwap;           /**< Hot-swap support */
    BOOLEAN             b2eVME;             /**< 2eVME support */
    BOOLEAN             b2eSST;             /**< 2eSST support */
    UINT8               u2eSSTRate;         /**< Maximum 2eSST rate */

    // Arbiter
    VME_ARBITER_MODE    eArbiterMode;       /**< Arbiter mode */
    UINT32              uTimeoutUS;         /**< Bus timeout (microseconds) */

    // System controller
    BOOLEAN             bSystemController;  /**< Is system controller */
    CHAR8               szSystemType[64];   /**< System type */
} VME_BUS_INFO;

/**
 * @brief VME DMA transfer descriptor
 */
typedef struct _VME_DMA_DESC {
    UINT64              u64SourceAddr;      /**< Source address */
    UINT64              u64DestAddr;        /**< Destination address */
    UINT32              uLength;            /**< Transfer length */
    VME_ADDRESS_SPACE   eSourceSpace;       /**< Source address space */
    VME_ADDRESS_SPACE   eDestSpace;         /**< Destination address space */
    VME_DATA_WIDTH      eDataWidth;         /**< Data width */
    VME_TRANSFER_MODE   eTransferMode;      /**< Transfer mode */
    UINT8               uSourceAM;          /**< Source AM code */
    UINT8               uDestAM;            /**< Destination AM code */
    BOOLEAN             bInterruptOnDone;   /**< Interrupt when done */
} VME_DMA_DESC;

/**
 * @brief Known VME card database entry
 */
typedef struct _VME_CARD_DB_ENTRY {
    CONST CHAR8         *pszName;           /**< Card name */
    CONST CHAR8         *pszManufacturer;   /**< Manufacturer */
    CONST CHAR8         *pszModel;          /**< Model number */
    CONST CHAR8         *pszDescription;    /**< Description */
    UINT32              uOUI;               /**< IEEE OUI */
    UINT32              uBoardID;           /**< Board ID */
    VME_DEVICE_CLASS    eClass;             /**< Device class */
    VME_BUS_STANDARD    eStandard;          /**< Minimum standard */
    BOOLEAN             bBusMaster;         /**< Bus master */
} VME_CARD_DB_ENTRY;

//=============================================================================
// Forward Declarations
//=============================================================================

DECLARE_INTERFACE_(IIOVMEBus, IIOService);
DECLARE_INTERFACE_(IIOVMEDevice, IIOService);

//=============================================================================
// IIOVMEBus Interface
//=============================================================================

/**
 * @brief VMEbus controller interface
 *
 * Represents a VMEbus controller and provides methods for device
 * enumeration, address mapping, interrupt handling, and DMA operations.
 */
#undef INTERFACE
#define INTERFACE IIOVMEBus

DECLARE_INTERFACE_(IIOVMEBus, IIOService)
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

    // IIOVMEBus specific methods

    /**
     * @brief Get VME bus information
     *
     * @param pBusInfo      Receives bus information
     *
     * @retval IO_SUCCESS   Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetBusInfo)(THIS_
        VME_BUS_INFO *pBusInfo
        ) PURE;

    /**
     * @brief Scan VME bus for devices
     *
     * @param puDeviceCount Receives number of detected devices
     *
     * @retval IO_SUCCESS   Scan successful
     */
    STDMETHOD_(IO_RETURN, ScanBus)(THIS_
        UINT32 *puDeviceCount
        ) PURE;

    /**
     * @brief Get slot information
     *
     * @param uSlot         Slot number (1-21)
     * @param pSlotInfo     Receives slot information
     *
     * @retval IO_SUCCESS   Information retrieved
     * @retval IO_NO_DEVICE No card in slot
     */
    STDMETHOD_(IO_RETURN, GetSlotInfo)(THIS_
        UINT8 uSlot,
        VME_SLOT_INFO *pSlotInfo
        ) PURE;

    /**
     * @brief Enable/disable slot
     *
     * @param uSlot         Slot number
     * @param bEnable       TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS   Slot state changed
     */
    STDMETHOD_(IO_RETURN, EnableSlot)(THIS_
        UINT8 uSlot,
        BOOLEAN bEnable
        ) PURE;

    /**
     * @brief Read configuration ROM
     *
     * @param uSlot         Slot number
     * @param uOffset       Offset within CR
     * @param pBuffer       Receives data
     * @param uLength       Number of bytes to read
     *
     * @retval IO_SUCCESS   Read successful
     */
    STDMETHOD_(IO_RETURN, ReadCR)(THIS_
        UINT8 uSlot,
        UINT32 uOffset,
        VOID *pBuffer,
        UINT32 uLength
        ) PURE;

    /**
     * @brief Write to CSR space
     *
     * @param uSlot         Slot number
     * @param uOffset       Offset within CSR
     * @param pBuffer       Data to write
     * @param uLength       Number of bytes to write
     *
     * @retval IO_SUCCESS   Write successful
     */
    STDMETHOD_(IO_RETURN, WriteCSR)(THIS_
        UINT8 uSlot,
        UINT32 uOffset,
        CONST VOID *pBuffer,
        UINT32 uLength
        ) PURE;

    /**
     * @brief Map address window
     *
     * @param pWindow       Window configuration
     *
     * @retval IO_SUCCESS       Mapping successful
     * @retval IO_NO_RESOURCES  Address space unavailable
     */
    STDMETHOD_(IO_RETURN, MapWindow)(THIS_
        VME_ADDRESS_WINDOW *pWindow
        ) PURE;

    /**
     * @brief Unmap address window
     *
     * @param pWindow       Window to unmap
     *
     * @retval IO_SUCCESS   Unmapping successful
     */
    STDMETHOD_(IO_RETURN, UnmapWindow)(THIS_
        VME_ADDRESS_WINDOW *pWindow
        ) PURE;

    /**
     * @brief Register interrupt handler
     *
     * @param pInterrupt    Interrupt configuration
     *
     * @retval IO_SUCCESS       Registration successful
     * @retval IO_NO_INTERRUPT  IRQ level unavailable
     */
    STDMETHOD_(IO_RETURN, RegisterInterrupt)(THIS_
        VME_INTERRUPT *pInterrupt
        ) PURE;

    /**
     * @brief Unregister interrupt handler
     *
     * @param pInterrupt    Interrupt to unregister
     *
     * @retval IO_SUCCESS   Unregistration successful
     */
    STDMETHOD_(IO_RETURN, UnregisterInterrupt)(THIS_
        VME_INTERRUPT *pInterrupt
        ) PURE;

    /**
     * @brief Generate VME interrupt
     *
     * @param uLevel        IRQ level (1-7)
     * @param uVector       Interrupt vector
     *
     * @retval IO_SUCCESS   Interrupt generated
     */
    STDMETHOD_(IO_RETURN, GenerateInterrupt)(THIS_
        UINT8 uLevel,
        UINT8 uVector
        ) PURE;

    /**
     * @brief Setup DMA transfer
     *
     * @param pDesc         DMA descriptor
     *
     * @retval IO_SUCCESS   DMA setup successful
     */
    STDMETHOD_(IO_RETURN, SetupDMA)(THIS_
        CONST VME_DMA_DESC *pDesc
        ) PURE;

    /**
     * @brief Start DMA transfer
     *
     * @retval IO_SUCCESS   Transfer started
     */
    STDMETHOD_(IO_RETURN, StartDMA)(THIS) PURE;

    /**
     * @brief Wait for DMA completion
     *
     * @param uTimeoutMS    Timeout in milliseconds
     *
     * @retval IO_SUCCESS   Transfer completed
     * @retval IO_TIMEOUT   Transfer timed out
     */
    STDMETHOD_(IO_RETURN, WaitDMA)(THIS_
        UINT32 uTimeoutMS
        ) PURE;

    /**
     * @brief Abort DMA transfer
     *
     * @retval IO_SUCCESS   Transfer aborted
     */
    STDMETHOD_(IO_RETURN, AbortDMA)(THIS) PURE;

    /**
     * @brief Set arbiter mode
     *
     * @param eMode         Arbiter mode
     *
     * @retval IO_SUCCESS   Mode set successfully
     */
    STDMETHOD_(IO_RETURN, SetArbiterMode)(THIS_
        VME_ARBITER_MODE eMode
        ) PURE;

    /**
     * @brief Set bus timeout
     *
     * @param uTimeoutUS    Timeout in microseconds
     *
     * @retval IO_SUCCESS   Timeout set successfully
     */
    STDMETHOD_(IO_RETURN, SetBusTimeout)(THIS_
        UINT32 uTimeoutUS
        ) PURE;

    /**
     * @brief Request system controller role
     *
     * @retval IO_SUCCESS   System controller acquired
     * @retval IO_BUSY      Already a system controller
     */
    STDMETHOD_(IO_RETURN, BecomeSystemController)(THIS) PURE;

    /**
     * @brief Enumerate all devices
     *
     * @param ppDevices     Receives array of device interfaces
     * @param puCount       On input: max devices; On output: actual count
     *
     * @retval IO_SUCCESS   Enumeration successful
     */
    STDMETHOD_(IO_RETURN, EnumerateDevices)(THIS_
        IIOVMEDevice **ppDevices,
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Reset VME bus
     *
     * @retval IO_SUCCESS   Bus reset successful
     */
    STDMETHOD_(IO_RETURN, ResetBus)(THIS) PURE;
};

//=============================================================================
// IIOVMEDevice Interface
//=============================================================================

/**
 * @brief VME device interface
 *
 * Represents a VME device and provides methods for resource management
 * and device I/O operations.
 */
#undef INTERFACE
#define INTERFACE IIOVMEDevice

DECLARE_INTERFACE_(IIOVMEDevice, IIOService)
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

    // IIOVMEDevice specific methods

    /**
     * @brief Get device information
     *
     * @param pInfo         Receives device information
     *
     * @retval IO_SUCCESS   Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        VME_DEVICE_INFO *pInfo
        ) PURE;

    /**
     * @brief Get slot number
     *
     * @param puSlot        Receives slot number
     *
     * @retval IO_SUCCESS   Slot number retrieved
     */
    STDMETHOD_(IO_RETURN, GetSlot)(THIS_
        UINT8 *puSlot
        ) PURE;

    /**
     * @brief Get geographical address
     *
     * @param puGeoAddr     Receives geographical address
     *
     * @retval IO_SUCCESS   Address retrieved
     */
    STDMETHOD_(IO_RETURN, GetGeoAddr)(THIS_
        UINT8 *puGeoAddr
        ) PURE;

    /**
     * @brief Read from address window
     *
     * @param uWindowIndex  Window index
     * @param u64Offset     Offset within window
     * @param eDataWidth    Data width
     * @param pBuffer       Receives data
     * @param uLength       Number of items to read
     *
     * @retval IO_SUCCESS   Read successful
     */
    STDMETHOD_(IO_RETURN, Read)(THIS_
        UINT32 uWindowIndex,
        UINT64 u64Offset,
        VME_DATA_WIDTH eDataWidth,
        VOID *pBuffer,
        UINT32 uLength
        ) PURE;

    /**
     * @brief Write to address window
     *
     * @param uWindowIndex  Window index
     * @param u64Offset     Offset within window
     * @param eDataWidth    Data width
     * @param pBuffer       Data to write
     * @param uLength       Number of items to write
     *
     * @retval IO_SUCCESS   Write successful
     */
    STDMETHOD_(IO_RETURN, Write)(THIS_
        UINT32 uWindowIndex,
        UINT64 u64Offset,
        VME_DATA_WIDTH eDataWidth,
        CONST VOID *pBuffer,
        UINT32 uLength
        ) PURE;

    /**
     * @brief Enable device
     *
     * @retval IO_SUCCESS   Device enabled
     */
    STDMETHOD_(IO_RETURN, Enable)(THIS) PURE;

    /**
     * @brief Disable device
     *
     * @retval IO_SUCCESS   Device disabled
     */
    STDMETHOD_(IO_RETURN, Disable)(THIS) PURE;

    /**
     * @brief Reset device
     *
     * @retval IO_SUCCESS   Device reset
     */
    STDMETHOD_(IO_RETURN, Reset)(THIS) PURE;
};

#undef INTERFACE

//=============================================================================
// Helper Macros
//=============================================================================

/**
 * @brief Check if slot number is valid
 */
#define VME_SLOT_IS_VALID(slot) \
    ((slot) >= VME_SLOT_MIN && (slot) <= VME_SLOT_MAX)

/**
 * @brief Check if IRQ level is valid
 */
#define VME_IRQ_IS_VALID(irq) \
    ((irq) >= VME_IRQ_MIN && (irq) <= VME_IRQ_MAX)

/**
 * @brief Calculate CR/CSR address
 */
#define VME_CR_ADDRESS(slot) \
    (VME_CR_BASE + ((slot) * VME_CR_SIZE))

/**
 * @brief Calculate CSR address
 */
#define VME_CSR_ADDRESS(slot) \
    (VME_CR_ADDRESS(slot) + VME_CSR_OFFSET)

//=============================================================================
// Function Declarations
//=============================================================================

/**
 * @brief Initialize VMEbus subsystem
 *
 * @retval IO_SUCCESS   Subsystem initialized successfully
 */
IO_RETURN IOVMEInitialize(VOID);

/**
 * @brief Shutdown VMEbus subsystem
 *
 * @retval IO_SUCCESS   Subsystem shut down successfully
 */
IO_RETURN IOVMEShutdown(VOID);

/**
 * @brief Detect if VMEbus is present in system
 *
 * @param pbPresent     Receives presence flag
 *
 * @retval IO_SUCCESS   Detection successful
 */
IO_RETURN IOVMEDetect(BOOLEAN *pbPresent);

/**
 * @brief Create VMEbus controller instance
 *
 * @param ppBus         Receives bus interface
 *
 * @retval IO_SUCCESS   Bus created successfully
 * @retval IO_NO_MEMORY Insufficient memory
 */
IO_RETURN IOVMEBusCreate(IIOVMEBus **ppBus);

/**
 * @brief Create VME device instance
 *
 * @param pszName       Device name
 * @param ppDevice      Receives device interface
 *
 * @retval IO_SUCCESS   Device created successfully
 * @retval IO_NO_MEMORY Insufficient memory
 */
IO_RETURN IOVMEDeviceCreate(
    CONST CHAR8 *pszName,
    IIOVMEDevice **ppDevice
);

/**
 * @brief Get card database entry
 *
 * @param uOUI          IEEE OUI
 * @param uBoardID      Board ID
 * @param ppEntry       Receives database entry
 *
 * @retval IO_SUCCESS   Entry found
 * @retval IO_NOT_FOUND Entry not found
 */
IO_RETURN IOVMEGetCardInfo(
    UINT32 uOUI,
    UINT32 uBoardID,
    CONST VME_CARD_DB_ENTRY **ppEntry
);

/**
 * @brief Decode geographical address
 *
 * @param uGeoAddr      Geographical address pins
 * @param puSlot        Receives slot number
 *
 * @retval IO_SUCCESS   Decoded successfully
 */
IO_RETURN IOVMEDecodeGeoAddr(
    UINT8 uGeoAddr,
    UINT8 *puSlot
);

#ifdef __cplusplus
}
#endif

#endif // IOKIT_VMEBUS_H
