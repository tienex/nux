/**
 * @file turbochannel.h
 * @brief TURBOchannel Family Interface - DEC Expansion Bus
 *
 * This header defines the TURBOchannel family interface for Digital Equipment
 * Corporation's high-performance expansion bus architecture used in DECstation
 * and AlphaStation systems (1989-1996).
 *
 * TURBOchannel is a 32-bit synchronous bus with:
 * - 100 MB/s peak transfer rate (25 MHz, 32-bit wide)
 * - Option ROM for automatic device configuration
 * - DMA support with scatter-gather capability
 * - Up to 7 physical slots (3-7 standard, plus CPU slots)
 * - Memory-mapped I/O
 * - Interrupt routing via system interrupt controller
 *
 * Supported systems:
 * - DECstation 5000/200 (Personal DECstation)
 * - DECstation 5000/240 (TURBOchannel)
 * - DECstation 5000/260
 * - AlphaStation 200/400 series
 * - AlphaServer 1000 series
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_TURBOCHANNEL_H
#define IOKIT_TURBOCHANNEL_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Interface GUIDs
//=============================================================================

/**
 * @brief IIOTURBOchannelBus interface GUID
 * {D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A}
 */
DEFINE_GUID(IID_IIOTURBOchannelBus,
    0xD0E1F2A3, 0xB4C5, 0x6D7E, 0x8F, 0x9A, 0x0B, 0x1C, 0x2D, 0x3E, 0x4F, 0x5A);

/**
 * @brief IIOTURBOchannelDevice interface GUID
 * {E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B}
 */
DEFINE_GUID(IID_IIOTURBOchannelDevice,
    0xE1F2A3B4, 0xC5D6, 0x7E8F, 0x9A, 0x0B, 0x1C, 0x2D, 0x3E, 0x4F, 0x5A, 0x6B);

//=============================================================================
// TURBOchannel Constants
//=============================================================================

/**
 * @brief TURBOchannel bus characteristics
 */
#define TC_BUS_WIDTH            32          /**< 32-bit data bus */
#define TC_BUS_CLOCK            25000000    /**< 25 MHz clock */
#define TC_TRANSFER_RATE        100000000   /**< 100 MB/s peak transfer rate */
#define TC_ADDRESS_BITS         32          /**< 32-bit addressing */

/**
 * @brief TURBOchannel slot configuration
 */
#define TC_SLOT_MIN             0           /**< First TC slot */
#define TC_SLOT_MAX             6           /**< Last TC slot */
#define TC_SLOT_COUNT           7           /**< Maximum number of slots */
#define TC_SLOT_SIZE            0x00400000  /**< 4 MB per slot address space */
#define TC_ROM_SIZE             0x00020000  /**< 128 KB option ROM size */
#define TC_ROM_STRIDE           4           /**< ROM byte stride (32-bit aligned) */

/**
 * @brief TURBOchannel address space
 */
#define TC_BASE_ADDR_5000_200   0x10000000  /**< Base for DECstation 5000/200 */
#define TC_BASE_ADDR_5000_240   0x1E000000  /**< Base for DECstation 5000/240 */
#define TC_SLOT_BASE(slot)      (TC_BASE_ADDR_5000_200 + ((slot) * TC_SLOT_SIZE))

/**
 * @brief Option ROM offsets
 */
#define TC_ROM_OFFSET           0x003C0000  /**< Option ROM offset in slot */
#define TC_ROM_WIDTH_OFFSET     0x00000000  /**< ROM width field offset */
#define TC_ROM_STRIDE_OFFSET    0x00000004  /**< ROM stride field offset */
#define TC_ROM_SIZE_OFFSET      0x00000008  /**< ROM size field offset */
#define TC_ROM_SLOT_OFFSET      0x0000000C  /**< Slot number offset */
#define TC_ROM_VENDOR_OFFSET    0x00000010  /**< Vendor string offset */
#define TC_ROM_MODULE_OFFSET    0x00000018  /**< Module name offset */
#define TC_ROM_FIRMWARE_OFFSET  0x00000020  /**< Firmware revision offset */

/**
 * @brief TURBOchannel ROM magic values
 */
#define TC_ROM_MAGIC            0x00000001  /**< Valid ROM magic value */
#define TC_ROM_WIDTH_BYTE       0x00000001  /**< Byte-wide ROM */
#define TC_ROM_WIDTH_WORD       0x00000002  /**< Word-wide ROM */
#define TC_ROM_WIDTH_LONG       0x00000004  /**< Longword-wide ROM */

//=============================================================================
// TURBOchannel Module IDs
//=============================================================================

/**
 * @brief Standard TURBOchannel module identifiers
 * These are 8-character ASCII strings stored in the option ROM
 */

// Graphics modules
#define TC_MODULE_PMAG_AA       "PMAG-AA "  /**< 8-plane 1024x864 monochrome */
#define TC_MODULE_PMAG_BA       "PMAG-BA "  /**< 8-plane 1024x864 color */
#define TC_MODULE_PMAG_CA       "PMAG-CA "  /**< 8-plane 1024x768 color */
#define TC_MODULE_PMAG_DA       "PMAG-DA "  /**< 8-plane 1280x1024 color */
#define TC_MODULE_PMAG_DV       "PMAG-DV "  /**< 8-plane 1024x768 color with Z-buffer */
#define TC_MODULE_PMAG_FA       "PMAG-FA "  /**< 8-plane 1280x1024 color */
#define TC_MODULE_PMAG_JA       "PMAG-JA "  /**< 24-plane 1280x1024 true color */
#define TC_MODULE_PMAGB_AA      "PMAGB-AA"  /**< 8-plane 1024x864 SFB */
#define TC_MODULE_PMAGB_BA      "PMAGB-BA"  /**< 8-plane 1280x1024 HX */
#define TC_MODULE_PMAGB_FA      "PMAGB-FA"  /**< 8-plane 1280x1024 HX+ */
#define TC_MODULE_PMAGB_VA      "PMAGB-VA"  /**< 8-plane 1024x768 SFB+ */
#define TC_MODULE_PMAGD_AA      "PMAGD-AA"  /**< 24-plane 2048x1024 true color */
#define TC_MODULE_PMAGD_BA      "PMAGD-BA"  /**< 24-plane 1280x1024 ZLX-E1 */

// SCSI controllers
#define TC_MODULE_PMAZ_AA       "PMAZ-AA "  /**< 53C94 SCSI controller */
#define TC_MODULE_PMAZ_AB       "PMAZ-AB "  /**< 53C94 Fast SCSI controller */
#define TC_MODULE_PMAZ_BA       "PMAZ-BA "  /**< Fast/Wide SCSI-2 controller */
#define TC_MODULE_PMAZ_DS       "PMAZ-DS "  /**< Dual SCSI controller */
#define TC_MODULE_PMAZ_FS       "PMAZ-FS "  /**< Fast SCSI-2 controller */

// Network adapters
#define TC_MODULE_PMAD_AA       "PMAD-AA "  /**< LANCE Ethernet (AMD 7990) */
#define TC_MODULE_PMAD_AB       "PMAD-AB "  /**< LANCE Ethernet variant */
#define TC_MODULE_PMAF_AA       "PMAF-AA "  /**< FDDI controller (DEC DEFTA) */
#define TC_MODULE_PMAF_FA       "PMAF-FA "  /**< FDDI-II controller */
#define TC_MODULE_PMAT_AA       "PMAT-AA "  /**< ATM OC-3c controller */

// Serial/Communications
#define TC_MODULE_PMAGC_AA      "PMAGC-AA"  /**< Async serial (8 ports) */
#define TC_MODULE_PMAGC_BA      "PMAGC-BA"  /**< Async serial (16 ports) */
#define TC_MODULE_PMTNV_AA      "PMTNV-AA"  /**< ISDN Basic Rate Interface */

// Memory expansion
#define TC_MODULE_PMEM_AA       "PMEM-AA "  /**< 8 MB memory module */
#define TC_MODULE_PMEM_BA       "PMEM-BA "  /**< 16 MB memory module */
#define TC_MODULE_PMEM_CA       "PMEM-CA "  /**< 32 MB memory module */

// Audio
#define TC_MODULE_LOFI          "LOFI    "  /**< Audio/ISDN module */
#define TC_MODULE_PMAGB_JA      "PMAGB-JA"  /**< Audio I/O module */

// CPU modules (for multiprocessor systems)
#define TC_MODULE_KN02_AA       "KN02-AA "  /**< R3000 CPU module */
#define TC_MODULE_KN03_AA       "KN03-AA "  /**< R4000 CPU module */

// Miscellaneous
#define TC_MODULE_T1D4PKT       "T1D4PKT "  /**< T1 packet interface */
#define TC_MODULE_T3PKT         "T3PKT   "  /**< T3 packet interface */
#define TC_MODULE_FORE_ATM      "FORE-ATM"  /**< FORE Systems ATM adapter */

//=============================================================================
// TURBOchannel Structures
//=============================================================================

/**
 * @brief TURBOchannel option ROM structure
 */
typedef struct _TC_OPTION_ROM {
    UINT32      uWidth;             /**< ROM width (1, 2, or 4 bytes) */
    UINT32      uStride;            /**< Byte stride between ROM locations */
    UINT32      uSize;              /**< Size of ROM in bytes */
    UINT32      uSlot;              /**< Slot number (0-6) */
    CHAR8       szVendor[16];       /**< Vendor string (null-terminated) */
    CHAR8       szModule[16];       /**< Module name (8 chars + padding) */
    CHAR8       szFirmware[16];     /**< Firmware revision string */
    UINT32      uModuleID;          /**< Numeric module ID */
    UINT32      uRevision;          /**< Module revision */
    UINT32      uFlags;             /**< Module flags */
    UINT8       *pROMData;          /**< Pointer to raw ROM data */
    UINT32      uROMDataSize;       /**< Size of ROM data buffer */
} TC_OPTION_ROM;

/**
 * @brief TURBOchannel device capabilities
 */
typedef enum _TC_DEVICE_CAPABILITY {
    TC_CAP_DMA              = 0x0001,   /**< DMA capable */
    TC_CAP_SCATTER_GATHER   = 0x0002,   /**< Scatter-gather DMA */
    TC_CAP_BURST_MODE       = 0x0004,   /**< Burst transfer mode */
    TC_CAP_INTERRUPT        = 0x0008,   /**< Interrupt capable */
    TC_CAP_MEMORY           = 0x0010,   /**< Memory expansion */
    TC_CAP_IO               = 0x0020,   /**< I/O device */
    TC_CAP_MULTIFUNCTION    = 0x0040,   /**< Multi-function device */
} TC_DEVICE_CAPABILITY;

/**
 * @brief TURBOchannel device type
 */
typedef enum _TC_DEVICE_TYPE {
    TC_TYPE_UNKNOWN         = 0,
    TC_TYPE_GRAPHICS        = 1,    /**< Graphics/display adapter */
    TC_TYPE_SCSI            = 2,    /**< SCSI controller */
    TC_TYPE_NETWORK         = 3,    /**< Network adapter */
    TC_TYPE_SERIAL          = 4,    /**< Serial/communications */
    TC_TYPE_MEMORY          = 5,    /**< Memory expansion */
    TC_TYPE_AUDIO           = 6,    /**< Audio I/O */
    TC_TYPE_CPU             = 7,    /**< CPU module */
    TC_TYPE_MISC            = 8,    /**< Miscellaneous */
} TC_DEVICE_TYPE;

/**
 * @brief TURBOchannel slot information
 */
typedef struct _TC_SLOT_INFO {
    UINT8               uSlot;          /**< Slot number (0-6) */
    UINT32              uBaseAddress;   /**< Base address in memory map */
    UINT32              uSlotSize;      /**< Size of slot address space */
    BOOLEAN             bCardPresent;   /**< TRUE if card installed */
    BOOLEAN             bEnabled;       /**< TRUE if slot enabled */
    BOOLEAN             bROMValid;      /**< TRUE if option ROM is valid */
    TC_OPTION_ROM       ROM;            /**< Option ROM data */
    TC_DEVICE_TYPE      DeviceType;     /**< Device type */
    UINT32              uCapabilities;  /**< Device capabilities (bitfield) */
} TC_SLOT_INFO;

/**
 * @brief TURBOchannel bus information
 */
typedef struct _TC_BUS_INFO {
    UINT32              uBusWidth;      /**< Bus width in bits */
    UINT32              uBusClock;      /**< Bus clock in Hz */
    UINT32              uTransferRate;  /**< Peak transfer rate in bytes/sec */
    UINT8               uSlotCount;     /**< Number of slots */
    UINT32              uBaseAddress;   /**< Base address of bus */
    CHAR8               szSystemModel[64]; /**< System model (e.g., "DECstation 5000/200") */
    UINT32              uSystemRevision; /**< System board revision */
} TC_BUS_INFO;

/**
 * @brief TURBOchannel DMA descriptor
 */
typedef struct _TC_DMA_DESCRIPTOR {
    UINT32              uPhysicalAddress;   /**< Physical address */
    UINT32              uLength;            /**< Transfer length */
    UINT32              uFlags;             /**< DMA flags */
    struct _TC_DMA_DESCRIPTOR *pNext;       /**< Next descriptor (for scatter-gather) */
} TC_DMA_DESCRIPTOR;

/**
 * @brief TURBOchannel DMA flags
 */
#define TC_DMA_READ             0x0001  /**< DMA read (device to memory) */
#define TC_DMA_WRITE            0x0002  /**< DMA write (memory to device) */
#define TC_DMA_INTERRUPT        0x0004  /**< Generate interrupt on completion */
#define TC_DMA_CHAIN            0x0008  /**< Chain to next descriptor */

/**
 * @brief TURBOchannel interrupt information
 */
typedef struct _TC_INTERRUPT_INFO {
    UINT8               uIRQ;           /**< Interrupt request line */
    UINT8               uPriority;      /**< Interrupt priority */
    BOOLEAN             bShared;        /**< TRUE if interrupt is shared */
    VOID                (*pfnHandler)(VOID *pContext); /**< Interrupt handler */
    VOID                *pContext;      /**< Handler context */
} TC_INTERRUPT_INFO;

/**
 * @brief TURBOchannel device information
 */
typedef struct _TC_DEVICE_INFO {
    UINT8               uSlot;          /**< Slot number */
    UINT32              uBaseAddress;   /**< Base address */
    CHAR8               szVendor[16];   /**< Vendor name */
    CHAR8               szModule[16];   /**< Module name */
    CHAR8               szFirmware[16]; /**< Firmware revision */
    TC_DEVICE_TYPE      DeviceType;     /**< Device type */
    UINT32              uCapabilities;  /**< Capabilities bitfield */
    UINT32              uModuleID;      /**< Module ID */
    UINT32              uRevision;      /**< Module revision */
    TC_INTERRUPT_INFO   Interrupt;      /**< Interrupt information */
} TC_DEVICE_INFO;

//=============================================================================
// TURBOchannel Card Database
//=============================================================================

/**
 * @brief TURBOchannel card database entry
 */
typedef struct _TC_CARD_DB_ENTRY {
    CONST CHAR8         *pszModuleName;     /**< Module name (8 chars) */
    TC_DEVICE_TYPE      DeviceType;         /**< Device type */
    CONST CHAR8         *pszVendor;         /**< Vendor name */
    CONST CHAR8         *pszProductName;    /**< Product name */
    CONST CHAR8         *pszDescription;    /**< Detailed description */
    UINT32              uCapabilities;      /**< Device capabilities */
    UINT32              uMemorySize;        /**< Memory size (if applicable) */
} TC_CARD_DB_ENTRY;

//=============================================================================
// Forward Declarations
//=============================================================================

typedef struct IIOTURBOchannelBus IIOTURBOchannelBus;
typedef struct IIOTURBOchannelDevice IIOTURBOchannelDevice;

//=============================================================================
// IIOTURBOchannelBus Interface
//=============================================================================

/**
 * @brief TURBOchannel bus controller interface
 *
 * Provides methods for bus management, slot enumeration, and device detection.
 */
struct IIOTURBOchannelBus {
    IIOService Base;

    /**
     * @brief Get bus information
     *
     * @param this          This interface pointer
     * @param pBusInfo      Receives bus information
     *
     * @retval IO_SUCCESS   Information retrieved successfully
     */
    IO_RETURN (*GetBusInfo)(
        IIOTURBOchannelBus  *this,
        TC_BUS_INFO         *pBusInfo
    );

    /**
     * @brief Detect installed cards
     *
     * @param this          This interface pointer
     * @param puCardCount   Receives number of detected cards
     *
     * @retval IO_SUCCESS   Detection completed successfully
     */
    IO_RETURN (*DetectCards)(
        IIOTURBOchannelBus  *this,
        UINT32              *puCardCount
    );

    /**
     * @brief Get slot information
     *
     * @param this          This interface pointer
     * @param uSlot         Slot number (0-6)
     * @param pSlotInfo     Receives slot information
     *
     * @retval IO_SUCCESS       Information retrieved
     * @retval IO_ERROR_INVALID_PARAMETER Invalid slot number
     */
    IO_RETURN (*GetSlotInfo)(
        IIOTURBOchannelBus  *this,
        UINT8               uSlot,
        TC_SLOT_INFO        *pSlotInfo
    );

    /**
     * @brief Enable or disable a slot
     *
     * @param this          This interface pointer
     * @param uSlot         Slot number (0-6)
     * @param bEnable       TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS   Slot state changed
     */
    IO_RETURN (*EnableSlot)(
        IIOTURBOchannelBus  *this,
        UINT8               uSlot,
        BOOLEAN             bEnable
    );

    /**
     * @brief Read option ROM data
     *
     * @param this          This interface pointer
     * @param uSlot         Slot number (0-6)
     * @param uOffset       Offset within ROM
     * @param pBuffer       Buffer to receive data
     * @param uLength       Number of bytes to read
     *
     * @retval IO_SUCCESS   Data read successfully
     */
    IO_RETURN (*ReadOptionROM)(
        IIOTURBOchannelBus  *this,
        UINT8               uSlot,
        UINT32              uOffset,
        VOID                *pBuffer,
        UINT32              uLength
    );

    /**
     * @brief Parse option ROM
     *
     * @param this          This interface pointer
     * @param uSlot         Slot number (0-6)
     * @param pROM          Receives parsed ROM data
     *
     * @retval IO_SUCCESS       ROM parsed successfully
     * @retval IO_ERROR_NOT_FOUND No valid ROM found
     */
    IO_RETURN (*ParseOptionROM)(
        IIOTURBOchannelBus  *this,
        UINT8               uSlot,
        TC_OPTION_ROM       *pROM
    );

    /**
     * @brief Enumerate all devices
     *
     * @param this          This interface pointer
     * @param pppDevices    Receives array of device interfaces
     * @param puCount       On input: max devices; On output: actual count
     *
     * @retval IO_SUCCESS   Enumeration successful
     */
    IO_RETURN (*EnumerateDevices)(
        IIOTURBOchannelBus      *this,
        IIOTURBOchannelDevice   ***pppDevices,
        UINT32                  *puCount
    );

    /**
     * @brief Configure DMA for device
     *
     * @param this          This interface pointer
     * @param pDevice       Device to configure
     * @param pDescriptor   DMA descriptor chain
     *
     * @retval IO_SUCCESS   DMA configured
     */
    IO_RETURN (*ConfigureDMA)(
        IIOTURBOchannelBus      *this,
        IIOTURBOchannelDevice   *pDevice,
        TC_DMA_DESCRIPTOR       *pDescriptor
    );

    /**
     * @brief Start DMA transfer
     *
     * @param this          This interface pointer
     * @param pDevice       Device to start transfer
     *
     * @retval IO_SUCCESS   Transfer started
     */
    IO_RETURN (*StartDMA)(
        IIOTURBOchannelBus      *this,
        IIOTURBOchannelDevice   *pDevice
    );

    /**
     * @brief Stop DMA transfer
     *
     * @param this          This interface pointer
     * @param pDevice       Device to stop transfer
     *
     * @retval IO_SUCCESS   Transfer stopped
     */
    IO_RETURN (*StopDMA)(
        IIOTURBOchannelBus      *this,
        IIOTURBOchannelDevice   *pDevice
    );

    /**
     * @brief Get DMA status
     *
     * @param this          This interface pointer
     * @param pDevice       Device to query
     * @param puStatus      Receives DMA status flags
     *
     * @retval IO_SUCCESS   Status retrieved
     */
    IO_RETURN (*GetDMAStatus)(
        IIOTURBOchannelBus      *this,
        IIOTURBOchannelDevice   *pDevice,
        UINT32                  *puStatus
    );
};

//=============================================================================
// IIOTURBOchannelDevice Interface
//=============================================================================

/**
 * @brief TURBOchannel device interface
 *
 * Represents a device installed in a TURBOchannel slot.
 */
struct IIOTURBOchannelDevice {
    IIOService Base;

    /**
     * @brief Get slot number
     *
     * @param this          This interface pointer
     * @param puSlot        Receives slot number
     *
     * @retval IO_SUCCESS   Slot number retrieved
     */
    IO_RETURN (*GetSlot)(
        IIOTURBOchannelDevice   *this,
        UINT8                   *puSlot
    );

    /**
     * @brief Get device information
     *
     * @param this          This interface pointer
     * @param pDeviceInfo   Receives device information
     *
     * @retval IO_SUCCESS   Information retrieved
     */
    IO_RETURN (*GetDeviceInfo)(
        IIOTURBOchannelDevice   *this,
        TC_DEVICE_INFO          *pDeviceInfo
    );

    /**
     * @brief Get base address
     *
     * @param this              This interface pointer
     * @param puBaseAddress     Receives base address
     *
     * @retval IO_SUCCESS   Base address retrieved
     */
    IO_RETURN (*GetBaseAddress)(
        IIOTURBOchannelDevice   *this,
        UINT32                  *puBaseAddress
    );

    /**
     * @brief Read device memory
     *
     * @param this          This interface pointer
     * @param uOffset       Offset from base address
     * @param pBuffer       Buffer to receive data
     * @param uLength       Number of bytes to read
     *
     * @retval IO_SUCCESS   Data read successfully
     */
    IO_RETURN (*ReadMemory)(
        IIOTURBOchannelDevice   *this,
        UINT32                  uOffset,
        VOID                    *pBuffer,
        UINT32                  uLength
    );

    /**
     * @brief Write device memory
     *
     * @param this          This interface pointer
     * @param uOffset       Offset from base address
     * @param pBuffer       Buffer containing data to write
     * @param uLength       Number of bytes to write
     *
     * @retval IO_SUCCESS   Data written successfully
     */
    IO_RETURN (*WriteMemory)(
        IIOTURBOchannelDevice   *this,
        UINT32                  uOffset,
        CONST VOID              *pBuffer,
        UINT32                  uLength
    );

    /**
     * @brief Read 32-bit register
     *
     * @param this          This interface pointer
     * @param uOffset       Register offset
     * @param puValue       Receives register value
     *
     * @retval IO_SUCCESS   Register read successfully
     */
    IO_RETURN (*ReadRegister32)(
        IIOTURBOchannelDevice   *this,
        UINT32                  uOffset,
        UINT32                  *puValue
    );

    /**
     * @brief Write 32-bit register
     *
     * @param this          This interface pointer
     * @param uOffset       Register offset
     * @param uValue        Value to write
     *
     * @retval IO_SUCCESS   Register written successfully
     */
    IO_RETURN (*WriteRegister32)(
        IIOTURBOchannelDevice   *this,
        UINT32                  uOffset,
        UINT32                  uValue
    );

    /**
     * @brief Enable device
     *
     * @param this          This interface pointer
     *
     * @retval IO_SUCCESS   Device enabled
     */
    IO_RETURN (*Enable)(
        IIOTURBOchannelDevice   *this
    );

    /**
     * @brief Disable device
     *
     * @param this          This interface pointer
     *
     * @retval IO_SUCCESS   Device disabled
     */
    IO_RETURN (*Disable)(
        IIOTURBOchannelDevice   *this
    );

    /**
     * @brief Register interrupt handler
     *
     * @param this          This interface pointer
     * @param pfnHandler    Interrupt handler function
     * @param pContext      Context to pass to handler
     *
     * @retval IO_SUCCESS       Handler registered
     * @retval IO_NO_INTERRUPT  Device does not support interrupts
     */
    IO_RETURN (*RegisterInterruptHandler)(
        IIOTURBOchannelDevice   *this,
        VOID                    (*pfnHandler)(VOID *pContext),
        VOID                    *pContext
    );

    /**
     * @brief Unregister interrupt handler
     *
     * @param this          This interface pointer
     *
     * @retval IO_SUCCESS   Handler unregistered
     */
    IO_RETURN (*UnregisterInterruptHandler)(
        IIOTURBOchannelDevice   *this
    );

    /**
     * @brief Enable interrupts
     *
     * @param this          This interface pointer
     *
     * @retval IO_SUCCESS       Interrupts enabled
     * @retval IO_NO_INTERRUPT  Device does not support interrupts
     */
    IO_RETURN (*EnableInterrupts)(
        IIOTURBOchannelDevice   *this
    );

    /**
     * @brief Disable interrupts
     *
     * @param this          This interface pointer
     *
     * @retval IO_SUCCESS   Interrupts disabled
     */
    IO_RETURN (*DisableInterrupts)(
        IIOTURBOchannelDevice   *this
    );

    /**
     * @brief Get option ROM
     *
     * @param this          This interface pointer
     * @param pROM          Receives option ROM data
     *
     * @retval IO_SUCCESS   ROM data retrieved
     */
    IO_RETURN (*GetOptionROM)(
        IIOTURBOchannelDevice   *this,
        TC_OPTION_ROM           *pROM
    );

    /**
     * @brief Setup DMA transfer
     *
     * @param this          This interface pointer
     * @param pDescriptor   DMA descriptor chain
     *
     * @retval IO_SUCCESS       DMA configured
     * @retval IO_NOT_SUPPORTED Device does not support DMA
     */
    IO_RETURN (*SetupDMA)(
        IIOTURBOchannelDevice   *this,
        TC_DMA_DESCRIPTOR       *pDescriptor
    );

    /**
     * @brief Start DMA
     *
     * @param this          This interface pointer
     *
     * @retval IO_SUCCESS   DMA started
     */
    IO_RETURN (*StartDMA)(
        IIOTURBOchannelDevice   *this
    );

    /**
     * @brief Stop DMA
     *
     * @param this          This interface pointer
     *
     * @retval IO_SUCCESS   DMA stopped
     */
    IO_RETURN (*StopDMA)(
        IIOTURBOchannelDevice   *this
    );

    /**
     * @brief Get DMA status
     *
     * @param this          This interface pointer
     * @param puStatus      Receives status flags
     *
     * @retval IO_SUCCESS   Status retrieved
     */
    IO_RETURN (*GetDMAStatus)(
        IIOTURBOchannelDevice   *this,
        UINT32                  *puStatus
    );
};

//=============================================================================
// Helper Macros
//=============================================================================

/**
 * @brief Calculate slot base address
 */
#define TC_SLOT_TO_ADDRESS(base, slot) \
    ((base) + ((slot) * TC_SLOT_SIZE))

/**
 * @brief Check if slot number is valid
 */
#define TC_SLOT_IS_VALID(slot) \
    ((slot) >= TC_SLOT_MIN && (slot) <= TC_SLOT_MAX)

/**
 * @brief Get option ROM address for a slot
 */
#define TC_GET_ROM_ADDRESS(base, slot) \
    (TC_SLOT_TO_ADDRESS(base, slot) + TC_ROM_OFFSET)

/**
 * @brief Compare module names (8 characters)
 */
#define TC_MODULE_MATCH(a, b) \
    (strncmp((a), (b), 8) == 0)

//=============================================================================
// Function Declarations
//=============================================================================

/**
 * @brief Initialize TURBOchannel subsystem
 *
 * @retval IO_SUCCESS   Subsystem initialized
 */
IO_RETURN IOTURBOchannelInitialize(VOID);

/**
 * @brief Shutdown TURBOchannel subsystem
 *
 * @retval IO_SUCCESS   Subsystem shut down
 */
IO_RETURN IOTURBOchannelShutdown(VOID);

/**
 * @brief Get TURBOchannel bus instance
 *
 * @param ppBus         Receives bus interface
 *
 * @retval IO_SUCCESS       Bus instance retrieved
 * @retval IO_ERROR_NOT_FOUND No TURBOchannel bus present
 */
IO_RETURN IOTURBOchannelGetBus(IIOTURBOchannelBus **ppBus);

/**
 * @brief Detect if TURBOchannel is present
 *
 * @param pbPresent     Receives presence flag
 *
 * @retval IO_SUCCESS   Detection completed
 */
IO_RETURN IOTURBOchannelDetect(BOOLEAN *pbPresent);

/**
 * @brief Get card database entry
 *
 * @param pszModuleName Module name (8 characters)
 * @param ppEntry       Receives database entry
 *
 * @retval IO_SUCCESS       Entry found
 * @retval IO_ERROR_NOT_FOUND Module not in database
 */
IO_RETURN IOTURBOchannelGetCardInfo(
    CONST CHAR8             *pszModuleName,
    CONST TC_CARD_DB_ENTRY  **ppEntry
);

/**
 * @brief Parse module name from option ROM
 *
 * @param pROMData      Pointer to ROM data
 * @param uROMSize      Size of ROM data
 * @param pszModule     Buffer to receive module name (16 bytes)
 *
 * @retval IO_SUCCESS       Module name parsed
 * @retval IO_ERROR_INVALID Invalid ROM data
 */
IO_RETURN IOTURBOchannelParseModuleName(
    CONST UINT8 *pROMData,
    UINT32      uROMSize,
    CHAR8       *pszModule
);

/**
 * @brief Parse vendor name from option ROM
 *
 * @param pROMData      Pointer to ROM data
 * @param uROMSize      Size of ROM data
 * @param pszVendor     Buffer to receive vendor name (16 bytes)
 *
 * @retval IO_SUCCESS       Vendor name parsed
 * @retval IO_ERROR_INVALID Invalid ROM data
 */
IO_RETURN IOTURBOchannelParseVendorName(
    CONST UINT8 *pROMData,
    UINT32      uROMSize,
    CHAR8       *pszVendor
);

/**
 * @brief Validate option ROM
 *
 * @param pROMData      Pointer to ROM data
 * @param uROMSize      Size of ROM data
 * @param pbValid       Receives validation result
 *
 * @retval IO_SUCCESS   Validation completed
 */
IO_RETURN IOTURBOchannelValidateROM(
    CONST UINT8 *pROMData,
    UINT32      uROMSize,
    BOOLEAN     *pbValid
);

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_TURBOCHANNEL_H */
