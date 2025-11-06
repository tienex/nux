/**
 * @file mca.h
 * @brief MCA Family Interface - IBM Micro Channel Architecture Bus
 *
 * This header defines the MCA (Micro Channel Architecture) family interface
 * for IBM's proprietary expansion bus introduced with the PS/2 in 1987.
 *
 * MCA is a 16/32-bit bus with:
 * - 10 MHz bus clock
 * - Up to 320 MB/s bandwidth (32-bit streaming data mode)
 * - 8-16 expansion slots (typically 8)
 * - Programmable Option Select (POS) registers for configuration
 * - Software-configurable (no DIP switches required)
 * - Bus arbitration for multi-master operation
 * - Automatic configuration via Adapter Description Files (ADF)
 * - Shared memory and I/O addressing
 * - Level-sensitive interrupt support
 * - Video extension for high-speed graphics
 *
 * Supported systems:
 * - IBM PS/2 Models 50, 55, 60, 65, 70, 80, 90, 95
 * - IBM RS/6000 workstations
 * - NCR System 3000 series
 * - Tandy 5000 MC
 * - Reply Model 32
 * - Various IBM servers (3Server, AS/400)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_MCA_H
#define IOKIT_MCA_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Interface GUIDs
//=============================================================================

/**
 * @brief IIOMCABus interface GUID
 * {F0E1D2C3-B4A5-9687-7859-4A3B2C1D0E0F}
 */
DEFINE_GUID(IID_IIOMCABus,
    0xF0E1D2C3, 0xB4A5, 0x9687, 0x78, 0x59, 0x4A, 0x3B, 0x2C, 0x1D, 0x0E, 0x0F);

/**
 * @brief IIOMCADevice interface GUID
 * {E1D2C3B4-A596-8778-594A-3B2C1D0E0F10}
 */
DEFINE_GUID(IID_IIOMCADevice,
    0xE1D2C3B4, 0xA596, 0x8778, 0x59, 0x4A, 0x3B, 0x2C, 0x1D, 0x0E, 0x0F, 0x10);

//=============================================================================
// MCA Bus Constants
//=============================================================================

/**
 * @brief MCA slot configuration
 */
#define MCA_SLOT_MIN            0       /**< First MCA slot */
#define MCA_SLOT_MAX            15      /**< Last possible MCA slot */
#define MCA_SLOT_TYPICAL        8       /**< Typical slot count */

/**
 * @brief MCA bus speeds
 */
#define MCA_BUS_CLOCK_HZ        10000000    /**< 10 MHz bus clock */
#define MCA_BUS_BANDWIDTH_16    20000000    /**< 20 MB/s (16-bit) */
#define MCA_BUS_BANDWIDTH_32    40000000    /**< 40 MB/s (32-bit) */
#define MCA_BUS_BANDWIDTH_STREAM 320000000  /**< 320 MB/s (streaming) */

/**
 * @brief POS (Programmable Option Select) register addresses
 */
#define MCA_POS_SETUP_REG       0x96    /**< POS setup register */
#define MCA_POS_ADAPTER_ENABLE  0x08    /**< Adapter enable port */

// POS registers per slot (accessed after selecting slot)
#define MCA_POS_ID_LOW          0x100   /**< Adapter ID low byte */
#define MCA_POS_ID_HIGH         0x101   /**< Adapter ID high byte */
#define MCA_POS_OPTION_1        0x102   /**< Option select 1 */
#define MCA_POS_OPTION_2        0x103   /**< Option select 2 */
#define MCA_POS_OPTION_3        0x104   /**< Option select 3 */
#define MCA_POS_OPTION_4        0x105   /**< Option select 4 */
#define MCA_POS_SUBADDR_EXT_LSB 0x106   /**< Subaddress extension LSB */
#define MCA_POS_SUBADDR_EXT_MSB 0x107   /**< Subaddress extension MSB */

/**
 * @brief POS register bit definitions
 */
#define MCA_POS_CARD_ENABLE     0x01    /**< Card enable bit (POS register 2) */
#define MCA_POS_CARD_SETUP      0x08    /**< Card setup mode */
#define MCA_POS_CHANNEL_RESET   0x80    /**< Channel reset bit */

/**
 * @brief MCA arbitration levels (0-15, lower = higher priority)
 */
#define MCA_ARB_LEVEL_MIN       0       /**< Highest priority */
#define MCA_ARB_LEVEL_MAX       15      /**< Lowest priority */
#define MCA_ARB_LEVEL_DEFAULT   9       /**< Default arbitration level */

/**
 * @brief MCA DMA modes
 */
#define MCA_DMA_8BIT            0       /**< 8-bit DMA */
#define MCA_DMA_16BIT           1       /**< 16-bit DMA */
#define MCA_DMA_32BIT           2       /**< 32-bit DMA */

/**
 * @brief MCA interrupt lines
 */
#define MCA_IRQ_3               3       /**< IRQ 3 */
#define MCA_IRQ_4               4       /**< IRQ 4 */
#define MCA_IRQ_5               5       /**< IRQ 5 */
#define MCA_IRQ_7               7       /**< IRQ 7 */
#define MCA_IRQ_9               9       /**< IRQ 9 */
#define MCA_IRQ_10              10      /**< IRQ 10 */
#define MCA_IRQ_11              11      /**< IRQ 11 */
#define MCA_IRQ_12              12      /**< IRQ 12 */
#define MCA_IRQ_14              14      /**< IRQ 14 */
#define MCA_IRQ_15              15      /**< IRQ 15 */

/**
 * @brief MCA adapter ID special values
 */
#define MCA_ID_INVALID          0x0000  /**< Invalid/no adapter */
#define MCA_ID_RESERVED         0xFFFF  /**< Reserved ID */

//=============================================================================
// MCA Enumerations
//=============================================================================

/**
 * @brief MCA bus width
 */
typedef enum _MCA_BUS_WIDTH {
    MCA_BUS_WIDTH_16BIT     = 16,       /**< 16-bit bus */
    MCA_BUS_WIDTH_32BIT     = 32,       /**< 32-bit bus */
} MCA_BUS_WIDTH;

/**
 * @brief MCA transfer modes
 */
typedef enum _MCA_TRANSFER_MODE {
    MCA_TRANSFER_STANDARD       = 0,    /**< Standard transfer */
    MCA_TRANSFER_BURST          = 1,    /**< Burst mode */
    MCA_TRANSFER_STREAMING      = 2,    /**< Streaming data mode (up to 320 MB/s) */
} MCA_TRANSFER_MODE;

/**
 * @brief MCA arbitration mode
 */
typedef enum _MCA_ARB_MODE {
    MCA_ARB_FAIRNESS            = 0,    /**< Fairness arbitration */
    MCA_ARB_PRIORITY            = 1,    /**< Priority-based arbitration */
} MCA_ARB_MODE;

/**
 * @brief MCA interrupt trigger mode
 */
typedef enum _MCA_IRQ_TRIGGER {
    MCA_IRQ_EDGE_TRIGGERED      = 0,    /**< Edge-triggered */
    MCA_IRQ_LEVEL_TRIGGERED     = 1,    /**< Level-triggered (MCA standard) */
} MCA_IRQ_TRIGGER;

/**
 * @brief MCA card categories
 */
typedef enum _MCA_CARD_CATEGORY {
    MCA_CAT_UNKNOWN             = 0x00, /**< Unknown/unclassified */
    MCA_CAT_DISK                = 0x01, /**< Disk controller */
    MCA_CAT_DISPLAY             = 0x02, /**< Display adapter */
    MCA_CAT_NETWORK             = 0x03, /**< Network adapter */
    MCA_CAT_COMMUNICATIONS      = 0x04, /**< Communications */
    MCA_CAT_MEMORY              = 0x05, /**< Memory expansion */
    MCA_CAT_MULTIFUNCTION       = 0x06, /**< Multi-function */
    MCA_CAT_SCSI                = 0x07, /**< SCSI controller */
    MCA_CAT_AUDIO               = 0x08, /**< Audio adapter */
    MCA_CAT_SYSTEM              = 0x09, /**< System board */
} MCA_CARD_CATEGORY;

//=============================================================================
// MCA Structures
//=============================================================================

/**
 * @brief MCA adapter ID structure
 */
typedef struct _MCA_ADAPTER_ID {
    UINT16      uAdapterID;             /**< 16-bit adapter ID */
    UINT8       uCardRevision;          /**< Card revision */
    BOOLEAN     bEnabled;               /**< Card is enabled */
} MCA_ADAPTER_ID;

/**
 * @brief MCA POS register data
 */
typedef struct _MCA_POS_DATA {
    UINT16      uAdapterID;             /**< Adapter ID (POS 0-1) */
    UINT8       uPOS[6];                /**< POS registers 2-7 */
    BOOLEAN     bCardEnabled;           /**< Card enabled bit */
    UINT8       uIOAddress;             /**< Decoded I/O address */
    UINT8       uMemAddress;            /**< Decoded memory address */
    UINT8       uInterrupt;             /**< Decoded interrupt */
    UINT8       uDMAChannel;            /**< Decoded DMA channel */
    UINT8       uArbitrationLevel;      /**< Arbitration level */
} MCA_POS_DATA;

/**
 * @brief MCA I/O address range
 */
typedef struct _MCA_IO_RANGE {
    UINT16      uStart;                 /**< Starting I/O address */
    UINT16      uLength;                /**< Length in bytes */
    BOOLEAN     bEnabled;               /**< Range is enabled */
    BOOLEAN     b32BitCapable;          /**< Supports 32-bit access */
} MCA_IO_RANGE;

/**
 * @brief MCA memory range
 */
typedef struct _MCA_MEMORY_RANGE {
    UINT32      uStart;                 /**< Starting physical address */
    UINT32      uLength;                /**< Length in bytes */
    BOOLEAN     bEnabled;               /**< Range is enabled */
    BOOLEAN     bShared;                /**< Shared memory */
    BOOLEAN     bCacheable;             /**< Cacheable memory */
    BOOLEAN     b32BitCapable;          /**< Supports 32-bit access */
} MCA_MEMORY_RANGE;

/**
 * @brief MCA DMA channel configuration
 */
typedef struct _MCA_DMA_CONFIG {
    UINT8       uChannel;               /**< DMA channel (0-7) */
    UINT8       uMode;                  /**< DMA mode (8/16/32-bit) */
    BOOLEAN     bBusMaster;             /**< Bus mastering capable */
    BOOLEAN     bEnabled;               /**< DMA enabled */
    UINT32      uMaxTransferSize;       /**< Maximum transfer size */
} MCA_DMA_CONFIG;

/**
 * @brief MCA interrupt configuration
 */
typedef struct _MCA_IRQ_CONFIG {
    UINT8       uIRQ;                   /**< Interrupt line */
    MCA_IRQ_TRIGGER eTrigger;           /**< Trigger mode */
    BOOLEAN     bShared;                /**< Shareable interrupt */
    BOOLEAN     bEnabled;               /**< Interrupt enabled */
} MCA_IRQ_CONFIG;

/**
 * @brief MCA bus arbitration configuration
 */
typedef struct _MCA_ARB_CONFIG {
    UINT8       uArbitrationLevel;      /**< Arbitration level (0-15) */
    MCA_ARB_MODE eMode;                 /**< Arbitration mode */
    UINT32      uTimeout;               /**< Arbitration timeout (microseconds) */
    BOOLEAN     bBusMasterEnabled;      /**< Bus mastering enabled */
} MCA_ARB_CONFIG;

/**
 * @brief MCA streaming data configuration
 */
typedef struct _MCA_STREAM_CONFIG {
    BOOLEAN     bEnabled;               /**< Streaming mode enabled */
    UINT32      uBurstLength;           /**< Burst length (bytes) */
    UINT32      uBandwidth;             /**< Available bandwidth (bytes/sec) */
    BOOLEAN     bPreemptible;           /**< Can be preempted */
} MCA_STREAM_CONFIG;

/**
 * @brief MCA device information
 */
typedef struct _MCA_DEVICE_INFO {
    UINT8               uSlot;                  /**< Slot number */
    MCA_ADAPTER_ID      AdapterID;              /**< Adapter identification */
    MCA_POS_DATA        POSData;                /**< POS register data */
    CHAR8               szName[128];            /**< Device name */
    CHAR8               szVendor[128];          /**< Vendor name */
    CHAR8               szDescription[256];     /**< Description */
    MCA_CARD_CATEGORY   eCategory;              /**< Card category */
    MCA_BUS_WIDTH       eBusWidth;              /**< Bus width (16/32-bit) */

    // Resources
    MCA_IO_RANGE        IOPorts[8];             /**< I/O port ranges */
    UINT32              uIOCount;               /**< Number of I/O ranges */

    MCA_MEMORY_RANGE    Memory[4];              /**< Memory ranges */
    UINT32              uMemoryCount;           /**< Number of memory ranges */

    MCA_IRQ_CONFIG      Interrupts[2];          /**< Interrupt configurations */
    UINT32              uIRQCount;              /**< Number of interrupts */

    MCA_DMA_CONFIG      DMAChannels[2];         /**< DMA configurations */
    UINT32              uDMACount;              /**< Number of DMA channels */

    MCA_ARB_CONFIG      Arbitration;            /**< Arbitration config */
    MCA_STREAM_CONFIG   Streaming;              /**< Streaming config */

    BOOLEAN             bBusMaster;             /**< Bus master capable */
    BOOLEAN             bMatchedMemory;         /**< Matched memory cycle support */
    BOOLEAN             bVideoExtension;        /**< Video extension support */
} MCA_DEVICE_INFO;

/**
 * @brief MCA bus information
 */
typedef struct _MCA_BUS_INFO {
    BOOLEAN             bPresent;               /**< MCA bus present */
    MCA_BUS_WIDTH       eBusWidth;              /**< Bus width */
    UINT32              uBusClock;              /**< Bus clock (Hz) */
    UINT32              uMaxBandwidth;          /**< Maximum bandwidth */
    UINT8               uSlotCount;             /**< Number of slots */
    BOOLEAN             bStreamingSupport;      /**< Streaming data support */
    BOOLEAN             bBurstSupport;          /**< Burst mode support */
    CHAR8               szSystemID[64];         /**< System identifier */
} MCA_BUS_INFO;

//=============================================================================
// MCA Card Database (50+ known cards)
//=============================================================================

/**
 * @brief MCA card database entry
 */
typedef struct _MCA_CARD_DB_ENTRY {
    UINT16              uAdapterID;         /**< Adapter ID */
    CONST CHAR8         *pszVendor;         /**< Vendor name */
    CONST CHAR8         *pszName;           /**< Card name */
    CONST CHAR8         *pszDescription;    /**< Description */
    MCA_CARD_CATEGORY   eCategory;          /**< Category */
    MCA_BUS_WIDTH       eBusWidth;          /**< Bus width */
} MCA_CARD_DB_ENTRY;

/**
 * @brief Known MCA cards database (50+ popular cards)
 *
 * Note: MCA adapter IDs are assigned by IBM. Format is manufacturer + product code.
 */
extern CONST MCA_CARD_DB_ENTRY g_MCACardDatabase[];
extern CONST UINT32 g_uMCACardDatabaseSize;

// Sample of known MCA adapter IDs:

// IBM Display Adapters
#define MCA_ID_IBM_VGA              0x8EFC  /**< IBM PS/2 VGA Adapter */
#define MCA_ID_IBM_8514A            0x8FDB  /**< IBM 8514/A Display Adapter */
#define MCA_ID_IBM_XGA              0x8FD9  /**< IBM XGA Display Adapter */
#define MCA_ID_IBM_XGA2             0x8FDA  /**< IBM XGA-2 Display Adapter */

// IBM Network Adapters
#define MCA_ID_IBM_TOKEN_RING       0x6042  /**< IBM Token Ring Adapter */
#define MCA_ID_IBM_TOKEN_RING_16_4  0xE000  /**< IBM Token Ring 16/4 Adapter */
#define MCA_ID_IBM_ETHERNET         0x6FC0  /**< IBM Ethernet Adapter */
#define MCA_ID_IBM_ETHERNET_A       0xEFE5  /**< IBM Ethernet Adapter/A */

// IBM SCSI Controllers
#define MCA_ID_IBM_SCSI             0x8EFC  /**< IBM SCSI Adapter */
#define MCA_ID_IBM_SCSI_W_CACHE     0x8EFD  /**< IBM SCSI w/Cache */
#define MCA_ID_IBM_FAST_SCSI        0xDDFF  /**< IBM Fast SCSI Adapter */
#define MCA_ID_IBM_FAST_WIDE_SCSI   0xEFDD  /**< IBM Fast-Wide SCSI */

// IBM Serial/Parallel
#define MCA_ID_IBM_SERIAL_PARALLEL  0x7EFE  /**< IBM Serial/Parallel Adapter */
#define MCA_ID_IBM_MULTIPORT        0xEFEE  /**< IBM Multi-Protocol Adapter */

// IBM Memory
#define MCA_ID_IBM_MEMORY_2MB       0x8FFE  /**< IBM 2MB Memory Expansion */
#define MCA_ID_IBM_MEMORY_4MB       0x8FFF  /**< IBM 4MB Memory Expansion */
#define MCA_ID_IBM_MEMORY_8MB       0x9000  /**< IBM 8MB Memory Expansion */

// Adaptec SCSI Controllers (MCA versions)
#define MCA_ID_ADAPTEC_1640         0x0F1F  /**< Adaptec AHA-1640 SCSI */
#define MCA_ID_ADAPTEC_1641         0x627C  /**< Adaptec AHA-1641 SCSI */

// 3Com Network Adapters
#define MCA_ID_3COM_3C523           0x6042  /**< 3Com 3C523 EtherLink/MC */
#define MCA_ID_3COM_3C529           0x61DB  /**< 3Com 3C529 EtherLink III */
#define MCA_ID_3COM_3C527           0x627D  /**< 3Com 3C527 EtherLink/MC 32 */

// Western Digital / SMC Network
#define MCA_ID_WD_8003E             0x61C8  /**< Western Digital WD8003E/A */
#define MCA_ID_WD_8013E             0x61C9  /**< Western Digital WD8013E/A */
#define MCA_ID_SMC_ELITE16          0xEFE5  /**< SMC Elite16 Ultra */

// Intel Network Adapters
#define MCA_ID_INTEL_ETHEREXPRESS   0x6FC0  /**< Intel EtherExpress MCA */
#define MCA_ID_INTEL_TOKENEXPRESS   0x6FC1  /**< Intel TokenExpress MCA */

// Novell Network
#define MCA_ID_NOVELL_NE2           0x6042  /**< Novell NE/2 Ethernet */

// Future Domain SCSI
#define MCA_ID_FD_MCS600            0x6127  /**< Future Domain MCS-600/700 */

// BusLogic SCSI
#define MCA_ID_BUSLOGIC_BT640       0xEDD0  /**< BusLogic BT-640A SCSI */

// DPT SCSI
#define MCA_ID_DPT_PM2011           0xEFE1  /**< DPT PM2011/9X SCSI */

// Compaq Cards
#define MCA_ID_COMPAQ_ETHERNET      0x6042  /**< Compaq Ethernet MCA */

// Paradise / Western Digital Video
#define MCA_ID_PARADISE_VGA         0x8EFC  /**< Paradise VGA Professional */
#define MCA_ID_WD_VGA               0x8EFD  /**< Western Digital VGA */

// ATI Video
#define MCA_ID_ATI_MACH32           0x9E01  /**< ATI Mach32 MCA */

// Matrox Video
#define MCA_ID_MATROX_MGA           0x9F01  /**< Matrox MGA MCA */

// Sound Cards
#define MCA_ID_PROAUDIO_SPECTRUM    0x9EFF  /**< Pro AudioSpectrum MCA */
#define MCA_ID_ROLAND_MPU401        0x9F00  /**< Roland MPU-401 MCA */

// NCR SCSI
#define MCA_ID_NCR_53C90            0xEFEC  /**< NCR 53C90 SCSI */

//=============================================================================
// Forward Declarations
//=============================================================================

DECLARE_INTERFACE_(IIOMCABus, IIOService);
DECLARE_INTERFACE_(IIOMCADevice, IIOService);

//=============================================================================
// IIOMCABus Interface
//=============================================================================

/**
 * @brief IIOMCABus - MCA Bus Controller Interface
 *
 * Represents an MCA bus controller and provides methods for slot
 * enumeration, POS register access, arbitration, and device management.
 */
#undef INTERFACE
#define INTERFACE IIOMCABus

DECLARE_INTERFACE_(IIOMCABus, IIOService)
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

    // IIOMCABus specific methods

    /**
     * @brief Get MCA bus information
     *
     * @param pBusInfo      Receives bus information
     *
     * @retval IO_SUCCESS   Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetBusInfo)(THIS_
        MCA_BUS_INFO *pBusInfo
        ) PURE;

    /**
     * @brief Enumerate MCA slots
     *
     * @param ppDevices     Receives array of device interfaces
     * @param puCount       On input: max devices; On output: actual count
     *
     * @retval IO_SUCCESS   Enumeration successful
     */
    STDMETHOD_(IO_RETURN, EnumerateSlots)(THIS_
        IIOMCADevice **ppDevices,
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Read POS register
     *
     * @param uSlot         Slot number (0-15)
     * @param uRegister     POS register offset (0x100-0x107)
     * @param puValue       Receives register value
     *
     * @retval IO_SUCCESS   Read successful
     * @retval IO_NO_DEVICE No device in slot
     */
    STDMETHOD_(IO_RETURN, ReadPOS)(THIS_
        UINT8 uSlot,
        UINT16 uRegister,
        UINT8 *puValue
        ) PURE;

    /**
     * @brief Write POS register
     *
     * @param uSlot         Slot number (0-15)
     * @param uRegister     POS register offset (0x100-0x107)
     * @param uValue        Value to write
     *
     * @retval IO_SUCCESS   Write successful
     * @retval IO_NO_DEVICE No device in slot
     */
    STDMETHOD_(IO_RETURN, WritePOS)(THIS_
        UINT8 uSlot,
        UINT16 uRegister,
        UINT8 uValue
        ) PURE;

    /**
     * @brief Get adapter ID from slot
     *
     * @param uSlot         Slot number (0-15)
     * @param pAdapterID    Receives adapter ID
     *
     * @retval IO_SUCCESS   ID retrieved successfully
     * @retval IO_NO_DEVICE No device in slot
     */
    STDMETHOD_(IO_RETURN, GetAdapterID)(THIS_
        UINT8 uSlot,
        MCA_ADAPTER_ID *pAdapterID
        ) PURE;

    /**
     * @brief Enable/disable adapter in slot
     *
     * @param uSlot         Slot number (0-15)
     * @param bEnable       TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS   Adapter state changed
     * @retval IO_NO_DEVICE No device in slot
     */
    STDMETHOD_(IO_RETURN, EnableAdapter)(THIS_
        UINT8 uSlot,
        BOOLEAN bEnable
        ) PURE;

    /**
     * @brief Configure bus arbitration
     *
     * @param pConfig       Arbitration configuration
     *
     * @retval IO_SUCCESS   Arbitration configured
     */
    STDMETHOD_(IO_RETURN, ConfigureArbitration)(THIS_
        CONST MCA_ARB_CONFIG *pConfig
        ) PURE;

    /**
     * @brief Request bus mastership
     *
     * @param uSlot         Slot number requesting mastership
     * @param uTimeout      Timeout in microseconds
     *
     * @retval IO_SUCCESS       Bus granted
     * @retval IO_TIMEOUT       Request timed out
     */
    STDMETHOD_(IO_RETURN, RequestBus)(THIS_
        UINT8 uSlot,
        UINT32 uTimeout
        ) PURE;

    /**
     * @brief Release bus mastership
     *
     * @param uSlot         Slot number releasing mastership
     *
     * @retval IO_SUCCESS   Bus released
     */
    STDMETHOD_(IO_RETURN, ReleaseBus)(THIS_
        UINT8 uSlot
        ) PURE;

    /**
     * @brief Configure streaming data mode
     *
     * @param uSlot         Slot number
     * @param pConfig       Streaming configuration
     *
     * @retval IO_SUCCESS       Streaming configured
     * @retval IO_UNSUPPORTED   Streaming not supported
     */
    STDMETHOD_(IO_RETURN, ConfigureStreaming)(THIS_
        UINT8 uSlot,
        CONST MCA_STREAM_CONFIG *pConfig
        ) PURE;

    /**
     * @brief Allocate shared slot (for multi-slot adapters)
     *
     * @param uPrimarySlot  Primary slot
     * @param uSharedSlot   Shared slot to allocate
     *
     * @retval IO_SUCCESS       Slot allocated
     * @retval IO_NO_RESOURCES  Slot unavailable
     */
    STDMETHOD_(IO_RETURN, AllocateSharedSlot)(THIS_
        UINT8 uPrimarySlot,
        UINT8 uSharedSlot
        ) PURE;

    /**
     * @brief Free shared slot
     *
     * @param uPrimarySlot  Primary slot
     * @param uSharedSlot   Shared slot to free
     *
     * @retval IO_SUCCESS   Slot freed
     */
    STDMETHOD_(IO_RETURN, FreeSharedSlot)(THIS_
        UINT8 uPrimarySlot,
        UINT8 uSharedSlot
        ) PURE;

    /**
     * @brief Reset MCA channel
     *
     * @retval IO_SUCCESS   Channel reset successfully
     */
    STDMETHOD_(IO_RETURN, ResetChannel)(THIS) PURE;
};

//=============================================================================
// IIOMCADevice Interface
//=============================================================================

/**
 * @brief IIOMCADevice - MCA Device Interface
 *
 * Represents an MCA adapter card and provides methods for configuration,
 * I/O operations, and resource management.
 */
#undef INTERFACE
#define INTERFACE IIOMCADevice

DECLARE_INTERFACE_(IIOMCADevice, IIOService)
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

    // IIOMCADevice specific methods

    /**
     * @brief Get device information
     *
     * @param pInfo         Receives device information
     *
     * @retval IO_SUCCESS   Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        MCA_DEVICE_INFO *pInfo
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
     * @brief Get adapter ID
     *
     * @param pAdapterID    Receives adapter ID
     *
     * @retval IO_SUCCESS   ID retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetAdapterID)(THIS_
        MCA_ADAPTER_ID *pAdapterID
        ) PURE;

    /**
     * @brief Read POS register
     *
     * @param uRegister     POS register offset
     * @param puValue       Receives register value
     *
     * @retval IO_SUCCESS   Read successful
     */
    STDMETHOD_(IO_RETURN, ReadPOSRegister)(THIS_
        UINT16 uRegister,
        UINT8 *puValue
        ) PURE;

    /**
     * @brief Write POS register
     *
     * @param uRegister     POS register offset
     * @param uValue        Value to write
     *
     * @retval IO_SUCCESS   Write successful
     */
    STDMETHOD_(IO_RETURN, WritePOSRegister)(THIS_
        UINT16 uRegister,
        UINT8 uValue
        ) PURE;

    /**
     * @brief Parse POS registers to decode configuration
     *
     * @param pPOSData      Receives parsed POS data
     *
     * @retval IO_SUCCESS   Parse successful
     */
    STDMETHOD_(IO_RETURN, ParsePOS)(THIS_
        MCA_POS_DATA *pPOSData
        ) PURE;

    /**
     * @brief Read from I/O port
     *
     * @param uPort         Port address
     * @param uSize         Size (1, 2, or 4 bytes)
     * @param puValue       Receives value
     *
     * @retval IO_SUCCESS   Read successful
     */
    STDMETHOD_(IO_RETURN, ReadPort)(THIS_
        UINT16 uPort,
        UINT8 uSize,
        UINT32 *puValue
        ) PURE;

    /**
     * @brief Write to I/O port
     *
     * @param uPort         Port address
     * @param uSize         Size (1, 2, or 4 bytes)
     * @param uValue        Value to write
     *
     * @retval IO_SUCCESS   Write successful
     */
    STDMETHOD_(IO_RETURN, WritePort)(THIS_
        UINT16 uPort,
        UINT8 uSize,
        UINT32 uValue
        ) PURE;

    /**
     * @brief Enable interrupt
     *
     * @param uIRQ          IRQ line
     * @param pfnHandler    Interrupt handler
     * @param pContext      Handler context
     *
     * @retval IO_SUCCESS       Interrupt enabled
     * @retval IO_NO_INTERRUPT  IRQ not available
     */
    STDMETHOD_(IO_RETURN, EnableInterrupt)(THIS_
        UINT8 uIRQ,
        VOID (*pfnHandler)(VOID *pContext),
        VOID *pContext
        ) PURE;

    /**
     * @brief Disable interrupt
     *
     * @param uIRQ          IRQ line
     *
     * @retval IO_SUCCESS   Interrupt disabled
     */
    STDMETHOD_(IO_RETURN, DisableInterrupt)(THIS_
        UINT8 uIRQ
        ) PURE;

    /**
     * @brief Configure DMA transfer
     *
     * @param pConfig       DMA configuration
     *
     * @retval IO_SUCCESS   DMA configured
     */
    STDMETHOD_(IO_RETURN, ConfigureDMA)(THIS_
        CONST MCA_DMA_CONFIG *pConfig
        ) PURE;

    /**
     * @brief Setup streaming data transfer
     *
     * @param pBuffer       Buffer address
     * @param cbLength      Transfer length
     * @param bWrite        TRUE for write, FALSE for read
     *
     * @retval IO_SUCCESS       Transfer configured
     * @retval IO_UNSUPPORTED   Streaming not supported
     */
    STDMETHOD_(IO_RETURN, SetupStreamingTransfer)(THIS_
        VOID *pBuffer,
        UINT32 cbLength,
        BOOLEAN bWrite
        ) PURE;

    /**
     * @brief Enable/disable device
     *
     * @param bEnable       TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS   Device state changed
     */
    STDMETHOD_(IO_RETURN, Enable)(THIS_
        BOOLEAN bEnable
        ) PURE;

    /**
     * @brief Reset device
     *
     * @retval IO_SUCCESS   Device reset successfully
     */
    STDMETHOD_(IO_RETURN, Reset)(THIS) PURE;
};

#undef INTERFACE

//=============================================================================
// Convenience Macros
//=============================================================================

#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOMCABus_GetBusInfo(p,a)               (p)->lpVtbl->GetBusInfo(p,a)
#define IIOMCABus_EnumerateSlots(p,a,b)         (p)->lpVtbl->EnumerateSlots(p,a,b)
#define IIOMCABus_ReadPOS(p,a,b,c)              (p)->lpVtbl->ReadPOS(p,a,b,c)
#define IIOMCABus_WritePOS(p,a,b,c)             (p)->lpVtbl->WritePOS(p,a,b,c)
#define IIOMCABus_GetAdapterID(p,a,b)           (p)->lpVtbl->GetAdapterID(p,a,b)
#define IIOMCABus_EnableAdapter(p,a,b)          (p)->lpVtbl->EnableAdapter(p,a,b)
#define IIOMCABus_ConfigureArbitration(p,a)     (p)->lpVtbl->ConfigureArbitration(p,a)
#define IIOMCABus_RequestBus(p,a,b)             (p)->lpVtbl->RequestBus(p,a,b)
#define IIOMCABus_ReleaseBus(p,a)               (p)->lpVtbl->ReleaseBus(p,a)
#define IIOMCABus_ConfigureStreaming(p,a,b)     (p)->lpVtbl->ConfigureStreaming(p,a,b)
#define IIOMCABus_AllocateSharedSlot(p,a,b)     (p)->lpVtbl->AllocateSharedSlot(p,a,b)
#define IIOMCABus_FreeSharedSlot(p,a,b)         (p)->lpVtbl->FreeSharedSlot(p,a,b)
#define IIOMCABus_ResetChannel(p)               (p)->lpVtbl->ResetChannel(p)

#define IIOMCADevice_GetDeviceInfo(p,a)         (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOMCADevice_GetSlot(p,a)               (p)->lpVtbl->GetSlot(p,a)
#define IIOMCADevice_GetAdapterID(p,a)          (p)->lpVtbl->GetAdapterID(p,a)
#define IIOMCADevice_ReadPOSRegister(p,a,b)     (p)->lpVtbl->ReadPOSRegister(p,a,b)
#define IIOMCADevice_WritePOSRegister(p,a,b)    (p)->lpVtbl->WritePOSRegister(p,a,b)
#define IIOMCADevice_ParsePOS(p,a)              (p)->lpVtbl->ParsePOS(p,a)
#define IIOMCADevice_ReadPort(p,a,b,c)          (p)->lpVtbl->ReadPort(p,a,b,c)
#define IIOMCADevice_WritePort(p,a,b,c)         (p)->lpVtbl->WritePort(p,a,b,c)
#define IIOMCADevice_EnableInterrupt(p,a,b,c)   (p)->lpVtbl->EnableInterrupt(p,a,b,c)
#define IIOMCADevice_DisableInterrupt(p,a)      (p)->lpVtbl->DisableInterrupt(p,a)
#define IIOMCADevice_ConfigureDMA(p,a)          (p)->lpVtbl->ConfigureDMA(p,a)
#define IIOMCADevice_SetupStreamingTransfer(p,a,b,c) (p)->lpVtbl->SetupStreamingTransfer(p,a,b,c)
#define IIOMCADevice_Enable(p,a)                (p)->lpVtbl->Enable(p,a)
#define IIOMCADevice_Reset(p)                   (p)->lpVtbl->Reset(p)

#endif

//=============================================================================
// Public API Functions
//=============================================================================

/**
 * @brief Initialize MCA subsystem
 *
 * @retval IO_SUCCESS   Subsystem initialized successfully
 */
IO_RETURN
MCAInitialize(
    VOID
    );

/**
 * @brief Shutdown MCA subsystem
 *
 * @retval IO_SUCCESS   Subsystem shut down successfully
 */
IO_RETURN
MCAShutdown(
    VOID
    );

/**
 * @brief Detect MCA bus presence
 *
 * @param pbPresent     Receives TRUE if MCA bus present
 *
 * @retval IO_SUCCESS   Detection completed
 */
IO_RETURN
MCADetect(
    BOOLEAN *pbPresent
    );

/**
 * @brief Create an MCA bus instance
 *
 * @param ppBus         Receives bus interface
 *
 * @retval IO_SUCCESS           Bus created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_UNSUPPORTED       MCA not supported
 */
IO_RETURN
IOMCABusCreate(
    IIOMCABus **ppBus
    );

/**
 * @brief Create an MCA device instance
 *
 * @param uSlot         Slot number
 * @param ppDevice      Receives device interface
 *
 * @retval IO_SUCCESS           Device created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_NO_DEVICE         No device in slot
 */
IO_RETURN
IOMCADeviceCreate(
    UINT8 uSlot,
    IIOMCADevice **ppDevice
    );

/**
 * @brief Lookup card in database by adapter ID
 *
 * @param uAdapterID    Adapter ID to lookup
 * @param ppEntry       Receives database entry (NULL if not found)
 *
 * @retval IO_SUCCESS   Lookup completed (check *ppEntry for NULL)
 */
IO_RETURN
MCALookupCard(
    UINT16 uAdapterID,
    CONST MCA_CARD_DB_ENTRY **ppEntry
    );

/**
 * @brief Parse adapter ID into components
 *
 * @param uAdapterID    16-bit adapter ID
 * @param puVendor      Receives vendor code (optional, can be NULL)
 * @param puProduct     Receives product code (optional, can be NULL)
 *
 * @retval IO_SUCCESS   ID parsed successfully
 */
IO_RETURN
MCAParseAdapterID(
    UINT16 uAdapterID,
    UINT16 *puVendor,
    UINT16 *puProduct
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_MCA_H */
