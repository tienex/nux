/**
 * @file sbus.h
 * @brief SBus Family Interface - Sun Microsystems SBus Expansion Bus
 *
 * This header defines the SBus family interface for Sun Microsystems' expansion
 * bus architecture used in SPARCstation systems (1989-1996).
 *
 * SBus is a 32-bit synchronous bus with:
 * - 25/33 MHz clock frequency (25 MHz early systems, 33 MHz later)
 * - Up to 100 MB/s transfer rate (33 MHz burst mode)
 * - Auto-configuration via FCode (Forth-based firmware)
 * - 1-3 expansion slots (system dependent)
 * - Slot sizes: 1, 2, or 3 units wide
 * - DVMA (Direct Virtual Memory Access) support
 * - 7 interrupt levels (IPL 1-7)
 * - OpenBoot PROM integration
 * - Device tree based enumeration
 *
 * Supported systems:
 * - SPARCstation 1, 1+, 2, IPC, IPX, SLC
 * - SPARCstation 5, 10, 20
 * - SPARCclassic, SPARCserver series
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_SBUS_H
#define IOKIT_SBUS_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Interface GUIDs
//=============================================================================

/**
 * @brief IIOSBusBus interface GUID
 * {D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A}
 */
DEFINE_GUID(IID_IIOSBusBus,
    0xD0E1F2A3, 0xB4C5, 0x6D7E, 0x8F, 0x9A, 0x0B, 0x1C, 0x2D, 0x3E, 0x4F, 0x5A);

/**
 * @brief IIOSBusDevice interface GUID
 * {E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B}
 */
DEFINE_GUID(IID_IIOSBusDevice,
    0xE1F2A3B4, 0xC5D6, 0x7E8F, 0x9A, 0x0B, 0x1C, 0x2D, 0x3E, 0x4F, 0x5A, 0x6B);

//=============================================================================
// SBus Constants
//=============================================================================

/**
 * @brief SBus slot configuration
 */
#define SBUS_SLOT_MIN           0       /**< First SBus slot */
#define SBUS_SLOT_MAX           2       /**< Last SBus slot (0-2 = 3 slots max) */
#define SBUS_MAX_SLOTS          3       /**< Maximum number of slots */

/**
 * @brief SBus slot sizes (in units)
 */
#define SBUS_SLOT_SIZE_1        1       /**< 1-unit wide slot */
#define SBUS_SLOT_SIZE_2        2       /**< 2-unit wide slot */
#define SBUS_SLOT_SIZE_3        3       /**< 3-unit wide slot */

/**
 * @brief SBus clock frequencies
 */
#define SBUS_CLOCK_25MHZ        25000000    /**< 25 MHz (early systems) */
#define SBUS_CLOCK_33MHZ        33000000    /**< 33 MHz (later systems) */

/**
 * @brief SBus transfer rates
 */
#define SBUS_XFER_RATE_25MHZ    80000000    /**< ~80 MB/s at 25 MHz */
#define SBUS_XFER_RATE_33MHZ    100000000   /**< ~100 MB/s at 33 MHz */

/**
 * @brief SBus address space per slot
 */
#define SBUS_SLOT_ADDRESS_SIZE  0x10000000  /**< 256 MB per slot */

/**
 * @brief SBus interrupt levels (IPL)
 */
#define SBUS_IPL_MIN            1       /**< Minimum interrupt level */
#define SBUS_IPL_MAX            7       /**< Maximum interrupt level */
#define SBUS_IPL_COUNT          7       /**< Number of interrupt levels */

/**
 * @brief SBus DVMA constants
 */
#define SBUS_DVMA_BASE          0xF0000000  /**< DVMA base address */
#define SBUS_DVMA_SIZE          0x01000000  /**< 16 MB DVMA space */
#define SBUS_DVMA_PAGE_SIZE     0x1000      /**< 4 KB pages */

/**
 * @brief FCode ROM constants
 */
#define FCODE_MAGIC             0xF00DBA11  /**< FCode magic signature */
#define FCODE_FORMAT_08         0x08        /**< FCode format version 0.8 */
#define FCODE_FORMAT_10         0x10        /**< FCode format version 1.0 */
#define FCODE_FORMAT_20         0x20        /**< FCode format version 2.0 */
#define FCODE_FORMAT_30         0x30        /**< FCode format version 3.0 */

/**
 * @brief OpenBoot PROM constants
 */
#define OBP_NAME_MAX            32      /**< Maximum device name length */
#define OBP_PROP_MAX            128     /**< Maximum property name length */
#define OBP_VALUE_MAX           256     /**< Maximum property value length */

//=============================================================================
// SBus Enumerations
//=============================================================================

/**
 * @brief SBus device classes
 */
typedef enum _SBUS_DEVICE_CLASS {
    SBUS_CLASS_UNKNOWN          = 0x00, /**< Unknown device */
    SBUS_CLASS_DISPLAY          = 0x01, /**< Display/Graphics */
    SBUS_CLASS_NETWORK          = 0x02, /**< Network adapter */
    SBUS_CLASS_SCSI             = 0x03, /**< SCSI controller */
    SBUS_CLASS_AUDIO            = 0x04, /**< Audio device */
    SBUS_CLASS_SERIAL           = 0x05, /**< Serial I/O */
    SBUS_CLASS_PARALLEL         = 0x06, /**< Parallel I/O */
    SBUS_CLASS_BRIDGE           = 0x07, /**< Bridge device */
    SBUS_CLASS_STORAGE          = 0x08, /**< Storage controller */
    SBUS_CLASS_PROCESSOR        = 0x09, /**< Processor/Accelerator */
    SBUS_CLASS_MEMORY           = 0x0A, /**< Memory expansion */
    SBUS_CLASS_VIDEO_IN         = 0x0B, /**< Video capture */
    SBUS_CLASS_MULTIMEDIA       = 0x0C, /**< Multimedia */
} SBUS_DEVICE_CLASS;

/**
 * @brief SBus transfer modes
 */
typedef enum _SBUS_TRANSFER_MODE {
    SBUS_XFER_STANDARD          = 0,    /**< Standard transfer */
    SBUS_XFER_BURST             = 1,    /**< Burst transfer */
    SBUS_XFER_BLOCK             = 2,    /**< Block transfer */
} SBUS_TRANSFER_MODE;

/**
 * @brief SBus interrupt trigger modes
 */
typedef enum _SBUS_IRQ_TRIGGER {
    SBUS_IRQ_LEVEL              = 0,    /**< Level-triggered */
    SBUS_IRQ_EDGE               = 1,    /**< Edge-triggered */
} SBUS_IRQ_TRIGGER;

/**
 * @brief DVMA transfer direction
 */
typedef enum _SBUS_DVMA_DIRECTION {
    SBUS_DVMA_READ              = 0,    /**< Device reads from memory */
    SBUS_DVMA_WRITE             = 1,    /**< Device writes to memory */
    SBUS_DVMA_BIDIRECTIONAL     = 2,    /**< Bidirectional */
} SBUS_DVMA_DIRECTION;

/**
 * @brief FCode evaluation result
 */
typedef enum _FCODE_RESULT {
    FCODE_SUCCESS               = 0,    /**< FCode executed successfully */
    FCODE_ERROR_FORMAT          = 1,    /**< Invalid FCode format */
    FCODE_ERROR_CHECKSUM        = 2,    /**< Checksum error */
    FCODE_ERROR_VERSION         = 3,    /**< Unsupported version */
    FCODE_ERROR_EXECUTION       = 4,    /**< Execution error */
    FCODE_ERROR_NOT_FOUND       = 5,    /**< FCode not found */
} FCODE_RESULT;

//=============================================================================
// SBus Structures
//=============================================================================

/**
 * @brief FCode ROM header
 */
typedef struct _FCODE_HEADER {
    UINT8       uFormatID;              /**< FCode format version */
    UINT8       uChecksum;              /**< Header checksum */
    UINT32      uLength;                /**< Total FCode length */
    UINT8       Data[0];                /**< FCode data follows */
} FCODE_HEADER;

/**
 * @brief OpenBoot PROM property
 */
typedef struct _OBP_PROPERTY {
    CHAR8       szName[OBP_PROP_MAX];   /**< Property name */
    UINT32      uLength;                /**< Value length */
    UINT8       *pValue;                /**< Property value */
} OBP_PROPERTY;

/**
 * @brief OpenBoot PROM device node
 */
typedef struct _OBP_NODE {
    CHAR8       szName[OBP_NAME_MAX];   /**< Device name */
    CHAR8       szType[OBP_NAME_MAX];   /**< Device type */
    UINT32      uNodeID;                /**< Node ID */
    OBP_PROPERTY *pProperties;          /**< Array of properties */
    UINT32      uPropertyCount;         /**< Number of properties */
    struct _OBP_NODE *pParent;          /**< Parent node */
    struct _OBP_NODE *pChild;           /**< First child node */
    struct _OBP_NODE *pSibling;         /**< Next sibling node */
} OBP_NODE;

/**
 * @brief SBus register specification
 */
typedef struct _SBUS_REG {
    UINT32      uSlot;                  /**< Slot number */
    UINT32      uOffset;                /**< Offset within slot */
    UINT32      uSize;                  /**< Register size */
} SBUS_REG;

/**
 * @brief SBus interrupt specification
 */
typedef struct _SBUS_INTERRUPT {
    UINT8       uLevel;                 /**< Interrupt level (1-7) */
    UINT8       uVector;                /**< Interrupt vector */
    SBUS_IRQ_TRIGGER eTrigger;          /**< Trigger mode */
    BOOLEAN     bShared;                /**< Shareable interrupt */
    VOID        (*pfnHandler)(VOID *);  /**< Interrupt handler */
    VOID        *pContext;              /**< Handler context */
} SBUS_INTERRUPT;

/**
 * @brief DVMA mapping
 */
typedef struct _SBUS_DVMA_MAPPING {
    UINT32      uDVMAAddress;           /**< DVMA virtual address */
    UINT32      uPhysicalAddress;       /**< Physical address */
    UINT32      uSize;                  /**< Mapping size */
    SBUS_DVMA_DIRECTION eDirection;     /**< Transfer direction */
    BOOLEAN     bCached;                /**< Cacheable mapping */
    BOOLEAN     bActive;                /**< Mapping active */
} SBUS_DVMA_MAPPING;

/**
 * @brief SBus device information
 */
typedef struct _SBUS_DEVICE_INFO {
    UINT8       uSlot;                  /**< Slot number (0-2) */
    UINT8       uSlotSize;              /**< Slot size in units (1-3) */
    SBUS_DEVICE_CLASS eClass;           /**< Device class */

    // Device identification
    CHAR8       szName[OBP_NAME_MAX];   /**< Device name (from OBP) */
    CHAR8       szType[OBP_NAME_MAX];   /**< Device type */
    CHAR8       szModel[64];            /**< Model string */
    CHAR8       szManufacturer[64];     /**< Manufacturer */
    CHAR8       szCompatible[128];      /**< Compatible devices */

    // Hardware resources
    SBUS_REG    *pRegisters;            /**< Register mappings */
    UINT32      uRegisterCount;         /**< Number of register sets */

    SBUS_INTERRUPT *pInterrupts;        /**< Interrupt specifications */
    UINT32      uInterruptCount;        /**< Number of interrupts */

    // FCode support
    BOOLEAN     bHasFCode;              /**< FCode ROM present */
    UINT32      uFCodeOffset;           /**< FCode ROM offset */
    UINT32      uFCodeLength;           /**< FCode ROM length */
    UINT8       uFCodeVersion;          /**< FCode format version */

    // DVMA support
    BOOLEAN     bDVMACapable;           /**< Supports DVMA */
    UINT32      uDVMABurst;             /**< Burst size (bytes) */

    // OpenBoot PROM
    OBP_NODE    *pOBPNode;              /**< OpenBoot device tree node */

    // Bus timing
    UINT32      uClockFreq;             /**< Bus clock frequency */
    UINT8       uBurstSize;             /**< Maximum burst size */

    // Flags
    BOOLEAN     bEnabled;               /**< Device enabled */
    BOOLEAN     bBusMaster;             /**< Bus mastering capable */
    BOOLEAN     b64BitCapable;          /**< 64-bit capable */
} SBUS_DEVICE_INFO;

/**
 * @brief SBus slot information
 */
typedef struct _SBUS_SLOT_INFO {
    UINT8       uSlot;                  /**< Slot number */
    UINT32      uBaseAddress;           /**< Base physical address */
    UINT8       uSize;                  /**< Slot size (units) */
    BOOLEAN     bOccupied;              /**< Card present */
    BOOLEAN     bEnabled;               /**< Slot enabled */
    SBUS_DEVICE_INFO DeviceInfo;        /**< Device info (if occupied) */
} SBUS_SLOT_INFO;

/**
 * @brief SBus controller information
 */
typedef struct _SBUS_BUS_INFO {
    UINT32      uClockFreq;             /**< Bus clock frequency */
    UINT8       uSlotCount;             /**< Number of slots */
    UINT32      uMaxBurst;              /**< Maximum burst size */
    BOOLEAN     bDVMASupported;         /**< DVMA supported */
    UINT32      uDVMABase;              /**< DVMA base address */
    UINT32      uDVMASize;              /**< DVMA space size */
    CHAR8       szSystemType[32];       /**< System type (e.g., "SPARCstation 5") */
} SBUS_BUS_INFO;

/**
 * @brief Known SBus card database entry
 */
typedef struct _SBUS_CARD_DB_ENTRY {
    CONST CHAR8     *pszName;           /**< Device name */
    CONST CHAR8     *pszType;           /**< Device type */
    CONST CHAR8     *pszManufacturer;   /**< Manufacturer */
    CONST CHAR8     *pszModel;          /**< Model name */
    CONST CHAR8     *pszDescription;    /**< Description */
    SBUS_DEVICE_CLASS eClass;           /**< Device class */
    UINT8           uSlotSize;          /**< Required slot size */
    BOOLEAN         bDVMARequired;      /**< Requires DVMA */
} SBUS_CARD_DB_ENTRY;

/**
 * @brief FCode interpreter context
 */
typedef struct _FCODE_CONTEXT {
    UINT8       *pCode;                 /**< FCode bytecode */
    UINT32      uLength;                /**< Code length */
    UINT32      uPC;                    /**< Program counter */
    VOID        *pDevice;               /**< Associated device */
    BOOLEAN     bDebug;                 /**< Debug mode */
} FCODE_CONTEXT;

//=============================================================================
// Forward Declarations
//=============================================================================

DECLARE_INTERFACE_(IIOSBusBus, IIOService);
DECLARE_INTERFACE_(IIOSBusDevice, IIOService);

//=============================================================================
// IIOSBusBus Interface
//=============================================================================

/**
 * @brief SBus bus controller interface
 *
 * Represents an SBus controller and provides methods for device
 * enumeration, resource allocation, and hardware management.
 */
#undef INTERFACE
#define INTERFACE IIOSBusBus

DECLARE_INTERFACE_(IIOSBusBus, IIOService)
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

    // IIOSBusBus specific methods

    /**
     * @brief Get SBus controller information
     *
     * @param pBusInfo      Receives bus information
     *
     * @retval IO_SUCCESS   Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetBusInfo)(THIS_
        SBUS_BUS_INFO *pBusInfo
        ) PURE;

    /**
     * @brief Detect installed cards
     *
     * @param puCardCount   Receives number of detected cards
     *
     * @retval IO_SUCCESS   Detection successful
     */
    STDMETHOD_(IO_RETURN, DetectCards)(THIS_
        UINT32 *puCardCount
        ) PURE;

    /**
     * @brief Get slot information
     *
     * @param uSlot         Slot number (0-2)
     * @param pSlotInfo     Receives slot information
     *
     * @retval IO_SUCCESS   Information retrieved
     * @retval IO_NO_DEVICE No card in slot
     */
    STDMETHOD_(IO_RETURN, GetSlotInfo)(THIS_
        UINT8 uSlot,
        SBUS_SLOT_INFO *pSlotInfo
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
     * @brief Read FCode ROM
     *
     * @param uSlot         Slot number
     * @param uOffset       Offset within FCode ROM
     * @param pBuffer       Receives data
     * @param uLength       Number of bytes to read
     *
     * @retval IO_SUCCESS   Read successful
     */
    STDMETHOD_(IO_RETURN, ReadFCodeROM)(THIS_
        UINT8 uSlot,
        UINT32 uOffset,
        VOID *pBuffer,
        UINT32 uLength
        ) PURE;

    /**
     * @brief Evaluate FCode
     *
     * @param pContext      FCode interpreter context
     * @param pResult       Receives result code
     *
     * @retval IO_SUCCESS   FCode evaluated successfully
     */
    STDMETHOD_(IO_RETURN, EvaluateFCode)(THIS_
        FCODE_CONTEXT *pContext,
        FCODE_RESULT *pResult
        ) PURE;

    /**
     * @brief Allocate DVMA mapping
     *
     * @param uPhysAddr     Physical address
     * @param uSize         Mapping size
     * @param eDirection    Transfer direction
     * @param pMapping      Receives mapping information
     *
     * @retval IO_SUCCESS       Allocation successful
     * @retval IO_NO_RESOURCES  DVMA space exhausted
     */
    STDMETHOD_(IO_RETURN, AllocateDVMA)(THIS_
        UINT32 uPhysAddr,
        UINT32 uSize,
        SBUS_DVMA_DIRECTION eDirection,
        SBUS_DVMA_MAPPING *pMapping
        ) PURE;

    /**
     * @brief Free DVMA mapping
     *
     * @param pMapping      Mapping to free
     *
     * @retval IO_SUCCESS   Deallocation successful
     */
    STDMETHOD_(IO_RETURN, FreeDVMA)(THIS_
        SBUS_DVMA_MAPPING *pMapping
        ) PURE;

    /**
     * @brief Route interrupt
     *
     * @param uSlot         Slot number
     * @param pInterrupt    Interrupt specification
     *
     * @retval IO_SUCCESS       Interrupt routed
     * @retval IO_NO_INTERRUPT  No interrupt available
     */
    STDMETHOD_(IO_RETURN, RouteInterrupt)(THIS_
        UINT8 uSlot,
        SBUS_INTERRUPT *pInterrupt
        ) PURE;

    /**
     * @brief Enable interrupt
     *
     * @param pInterrupt    Interrupt to enable
     *
     * @retval IO_SUCCESS   Interrupt enabled
     */
    STDMETHOD_(IO_RETURN, EnableInterrupt)(THIS_
        SBUS_INTERRUPT *pInterrupt
        ) PURE;

    /**
     * @brief Disable interrupt
     *
     * @param pInterrupt    Interrupt to disable
     *
     * @retval IO_SUCCESS   Interrupt disabled
     */
    STDMETHOD_(IO_RETURN, DisableInterrupt)(THIS_
        SBUS_INTERRUPT *pInterrupt
        ) PURE;

    /**
     * @brief Enumerate all devices
     *
     * @param ppDevices     Receives array of device interfaces
     * @param puCount       On input: max devices; On output: actual count
     *
     * @retval IO_SUCCESS   Enumeration successful
     */
    STDMETHOD_(IO_RETURN, EnumerateDevices)(THIS_
        IIOSBusDevice **ppDevices,
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Get OpenBoot device tree
     *
     * @param ppRootNode    Receives root node of device tree
     *
     * @retval IO_SUCCESS   Device tree retrieved
     */
    STDMETHOD_(IO_RETURN, GetDeviceTree)(THIS_
        OBP_NODE **ppRootNode
        ) PURE;

    /**
     * @brief Set bus transfer mode
     *
     * @param eMode         Transfer mode
     *
     * @retval IO_SUCCESS   Mode set successfully
     */
    STDMETHOD_(IO_RETURN, SetTransferMode)(THIS_
        SBUS_TRANSFER_MODE eMode
        ) PURE;

    /**
     * @brief Sync DVMA operations
     *
     * @retval IO_SUCCESS   Sync completed
     */
    STDMETHOD_(IO_RETURN, SyncDVMA)(THIS) PURE;
};

//=============================================================================
// IIOSBusDevice Interface
//=============================================================================

/**
 * @brief SBus device interface
 *
 * Represents an SBus device and provides methods for resource management
 * and device I/O operations.
 */
#undef INTERFACE
#define INTERFACE IIOSBusDevice

DECLARE_INTERFACE_(IIOSBusDevice, IIOService)
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

    // IIOSBusDevice specific methods

    /**
     * @brief Get device information
     *
     * @param pInfo         Receives device information
     *
     * @retval IO_SUCCESS   Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        SBUS_DEVICE_INFO *pInfo
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
     * @brief Map register space
     *
     * @param uRegIndex     Register set index
     * @param ppAddress     Receives mapped address
     * @param puSize        Receives mapping size
     *
     * @retval IO_SUCCESS   Mapping successful
     */
    STDMETHOD_(IO_RETURN, MapRegisters)(THIS_
        UINT32 uRegIndex,
        VOID **ppAddress,
        UINT32 *puSize
        ) PURE;

    /**
     * @brief Unmap register space
     *
     * @param pAddress      Mapped address
     *
     * @retval IO_SUCCESS   Unmapping successful
     */
    STDMETHOD_(IO_RETURN, UnmapRegisters)(THIS_
        VOID *pAddress
        ) PURE;

    /**
     * @brief Read register
     *
     * @param uRegIndex     Register set index
     * @param uOffset       Offset within register set
     * @param uSize         Size (1, 2, or 4 bytes)
     * @param puValue       Receives value
     *
     * @retval IO_SUCCESS   Read successful
     */
    STDMETHOD_(IO_RETURN, ReadRegister)(THIS_
        UINT32 uRegIndex,
        UINT32 uOffset,
        UINT8 uSize,
        UINT32 *puValue
        ) PURE;

    /**
     * @brief Write register
     *
     * @param uRegIndex     Register set index
     * @param uOffset       Offset within register set
     * @param uSize         Size (1, 2, or 4 bytes)
     * @param uValue        Value to write
     *
     * @retval IO_SUCCESS   Write successful
     */
    STDMETHOD_(IO_RETURN, WriteRegister)(THIS_
        UINT32 uRegIndex,
        UINT32 uOffset,
        UINT8 uSize,
        UINT32 uValue
        ) PURE;

    /**
     * @brief Enable device interrupt
     *
     * @param uIndex        Interrupt index
     * @param pfnHandler    Interrupt handler
     * @param pContext      Handler context
     *
     * @retval IO_SUCCESS   Interrupt enabled
     */
    STDMETHOD_(IO_RETURN, EnableInterrupt)(THIS_
        UINT32 uIndex,
        VOID (*pfnHandler)(VOID *pContext),
        VOID *pContext
        ) PURE;

    /**
     * @brief Disable device interrupt
     *
     * @param uIndex        Interrupt index
     *
     * @retval IO_SUCCESS   Interrupt disabled
     */
    STDMETHOD_(IO_RETURN, DisableInterrupt)(THIS_
        UINT32 uIndex
        ) PURE;

    /**
     * @brief Allocate DVMA buffer
     *
     * @param uSize         Buffer size
     * @param eDirection    Transfer direction
     * @param pMapping      Receives DVMA mapping
     *
     * @retval IO_SUCCESS       Allocation successful
     * @retval IO_NO_RESOURCES  DVMA space exhausted
     */
    STDMETHOD_(IO_RETURN, AllocateDVMABuffer)(THIS_
        UINT32 uSize,
        SBUS_DVMA_DIRECTION eDirection,
        SBUS_DVMA_MAPPING *pMapping
        ) PURE;

    /**
     * @brief Free DVMA buffer
     *
     * @param pMapping      DVMA mapping to free
     *
     * @retval IO_SUCCESS   Deallocation successful
     */
    STDMETHOD_(IO_RETURN, FreeDVMABuffer)(THIS_
        SBUS_DVMA_MAPPING *pMapping
        ) PURE;

    /**
     * @brief Sync DVMA buffer
     *
     * @param pMapping      DVMA mapping to sync
     * @param eDirection    Sync direction
     *
     * @retval IO_SUCCESS   Sync completed
     */
    STDMETHOD_(IO_RETURN, SyncDVMABuffer)(THIS_
        SBUS_DVMA_MAPPING *pMapping,
        SBUS_DVMA_DIRECTION eDirection
        ) PURE;

    /**
     * @brief Get OpenBoot property
     *
     * @param pszName       Property name
     * @param pBuffer       Receives property value
     * @param puLength      On input: buffer size; On output: actual size
     *
     * @retval IO_SUCCESS   Property retrieved
     * @retval IO_NOT_FOUND Property not found
     */
    STDMETHOD_(IO_RETURN, GetOBPProperty)(THIS_
        CONST CHAR8 *pszName,
        VOID *pBuffer,
        UINT32 *puLength
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
#define SBUS_SLOT_IS_VALID(slot) \
    ((slot) >= SBUS_SLOT_MIN && (slot) <= SBUS_SLOT_MAX)

/**
 * @brief Check if interrupt level is valid
 */
#define SBUS_IPL_IS_VALID(ipl) \
    ((ipl) >= SBUS_IPL_MIN && (ipl) <= SBUS_IPL_MAX)

/**
 * @brief Calculate slot address
 */
#define SBUS_SLOT_ADDRESS(base, slot) \
    ((base) + ((slot) * SBUS_SLOT_ADDRESS_SIZE))

/**
 * @brief Align address to DVMA page
 */
#define SBUS_DVMA_PAGE_ALIGN(addr) \
    (((addr) + SBUS_DVMA_PAGE_SIZE - 1) & ~(SBUS_DVMA_PAGE_SIZE - 1))

/**
 * @brief Calculate number of DVMA pages
 */
#define SBUS_DVMA_PAGES(size) \
    (((size) + SBUS_DVMA_PAGE_SIZE - 1) / SBUS_DVMA_PAGE_SIZE)

//=============================================================================
// Function Declarations
//=============================================================================

/**
 * @brief Initialize SBus subsystem
 *
 * @retval IO_SUCCESS   Subsystem initialized successfully
 */
IO_RETURN IOSBusInitialize(VOID);

/**
 * @brief Shutdown SBus subsystem
 *
 * @retval IO_SUCCESS   Subsystem shut down successfully
 */
IO_RETURN IOSBusShutdown(VOID);

/**
 * @brief Detect if SBus is present in system
 *
 * @param pbPresent     Receives presence flag
 *
 * @retval IO_SUCCESS   Detection successful
 */
IO_RETURN IOSBusDetect(BOOLEAN *pbPresent);

/**
 * @brief Create SBus controller instance
 *
 * @param ppBus         Receives bus interface
 *
 * @retval IO_SUCCESS   Bus created successfully
 * @retval IO_NO_MEMORY Insufficient memory
 */
IO_RETURN IOSBusBusCreate(IIOSBusBus **ppBus);

/**
 * @brief Create SBus device instance
 *
 * @param pszName       Device name
 * @param ppDevice      Receives device interface
 *
 * @retval IO_SUCCESS   Device created successfully
 * @retval IO_NO_MEMORY Insufficient memory
 */
IO_RETURN IOSBusDeviceCreate(
    CONST CHAR8 *pszName,
    IIOSBusDevice **ppDevice
);

/**
 * @brief Get card database entry
 *
 * @param pszName       Device name
 * @param ppEntry       Receives database entry
 *
 * @retval IO_SUCCESS   Entry found
 * @retval IO_NOT_FOUND Entry not found
 */
IO_RETURN IOSBusGetCardInfo(
    CONST CHAR8 *pszName,
    CONST SBUS_CARD_DB_ENTRY **ppEntry
);

/**
 * @brief Validate FCode ROM
 *
 * @param pFCode        FCode header
 * @param uMaxLength    Maximum length to validate
 *
 * @retval IO_SUCCESS   FCode is valid
 * @retval IO_ERROR     FCode is invalid
 */
IO_RETURN IOSBusValidateFCode(
    CONST FCODE_HEADER *pFCode,
    UINT32 uMaxLength
);

#ifdef __cplusplus
}
#endif

#endif // IOKIT_SBUS_H
