/**
 * @file pcie.h
 * @brief PCIe Family Interface - PCI Express Bus Driver
 *
 * This header defines the PCIe family interface for PCI Express bus management,
 * device enumeration, configuration space access, and resource allocation.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_PCIE_H
#define IOKIT_PCIE_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOPCIDevice interface GUID
 * {A3F7B9D4-6E2C-4A8F-9D5E-7C4A8B6F3E2D}
 */
DEFINE_GUID(IID_IIOPCIDevice,
    0xA3F7B9D4, 0x6E2C, 0x4A8F, 0x9D, 0x5E, 0x7C, 0x4A, 0x8B, 0x6F, 0x3E, 0x2D);

/**
 * @brief IIOPCIBridge interface GUID
 * {B4E8C6A5-7F3D-4B9E-8E6F-8D5B9C7E4F3A}
 */
DEFINE_GUID(IID_IIOPCIBridge,
    0xB4E8C6A5, 0x7F3D, 0x4B9E, 0x8E, 0x6F, 0x8D, 0x5B, 0x9C, 0x7E, 0x4F, 0x3A);

/**
 * @brief PCI Configuration Space Registers
 */
#define PCI_CFG_VENDOR_ID           0x00    /**< Vendor ID (16-bit) */
#define PCI_CFG_DEVICE_ID           0x02    /**< Device ID (16-bit) */
#define PCI_CFG_COMMAND             0x04    /**< Command register (16-bit) */
#define PCI_CFG_STATUS              0x06    /**< Status register (16-bit) */
#define PCI_CFG_REVISION_ID         0x08    /**< Revision ID (8-bit) */
#define PCI_CFG_CLASS_CODE          0x09    /**< Class code (24-bit) */
#define PCI_CFG_CACHE_LINE_SIZE     0x0C    /**< Cache line size (8-bit) */
#define PCI_CFG_LATENCY_TIMER       0x0D    /**< Latency timer (8-bit) */
#define PCI_CFG_HEADER_TYPE         0x0E    /**< Header type (8-bit) */
#define PCI_CFG_BIST                0x0F    /**< BIST (8-bit) */
#define PCI_CFG_BAR0                0x10    /**< Base Address Register 0 */
#define PCI_CFG_BAR1                0x14    /**< Base Address Register 1 */
#define PCI_CFG_BAR2                0x18    /**< Base Address Register 2 */
#define PCI_CFG_BAR3                0x1C    /**< Base Address Register 3 */
#define PCI_CFG_BAR4                0x20    /**< Base Address Register 4 */
#define PCI_CFG_BAR5                0x24    /**< Base Address Register 5 */
#define PCI_CFG_CARDBUS_CIS_PTR     0x28    /**< CardBus CIS pointer */
#define PCI_CFG_SUBSYS_VENDOR_ID    0x2C    /**< Subsystem vendor ID */
#define PCI_CFG_SUBSYS_DEVICE_ID    0x2E    /**< Subsystem device ID */
#define PCI_CFG_ROM_BASE_ADDR       0x30    /**< Expansion ROM base */
#define PCI_CFG_CAPABILITIES_PTR    0x34    /**< Capabilities pointer */
#define PCI_CFG_INTERRUPT_LINE      0x3C    /**< Interrupt line */
#define PCI_CFG_INTERRUPT_PIN       0x3D    /**< Interrupt pin */
#define PCI_CFG_MIN_GRANT           0x3E    /**< Min grant */
#define PCI_CFG_MAX_LATENCY         0x3F    /**< Max latency */

/**
 * @brief PCI Command Register Bits
 */
#define PCI_CMD_IO_SPACE            0x0001  /**< I/O space enable */
#define PCI_CMD_MEMORY_SPACE        0x0002  /**< Memory space enable */
#define PCI_CMD_BUS_MASTER          0x0004  /**< Bus master enable */
#define PCI_CMD_SPECIAL_CYCLES      0x0008  /**< Special cycles enable */
#define PCI_CMD_MWI_ENABLE          0x0010  /**< Memory write & invalidate */
#define PCI_CMD_VGA_PALETTE_SNOOP   0x0020  /**< VGA palette snoop */
#define PCI_CMD_PARITY_ERROR        0x0040  /**< Parity error response */
#define PCI_CMD_SERR_ENABLE         0x0100  /**< SERR# enable */
#define PCI_CMD_FAST_B2B_ENABLE     0x0200  /**< Fast back-to-back enable */
#define PCI_CMD_INTERRUPT_DISABLE   0x0400  /**< Interrupt disable */

/**
 * @brief PCI Status Register Bits
 */
#define PCI_STATUS_INTERRUPT        0x0008  /**< Interrupt status */
#define PCI_STATUS_CAP_LIST         0x0010  /**< Capabilities list */
#define PCI_STATUS_66MHZ            0x0020  /**< 66MHz capable */
#define PCI_STATUS_FAST_B2B         0x0080  /**< Fast back-to-back capable */
#define PCI_STATUS_MASTER_PARITY    0x0100  /**< Master data parity error */
#define PCI_STATUS_DEVSEL_MASK      0x0600  /**< DEVSEL timing */
#define PCI_STATUS_SIG_TARGET_ABORT 0x0800  /**< Signaled target abort */
#define PCI_STATUS_RCV_TARGET_ABORT 0x1000  /**< Received target abort */
#define PCI_STATUS_RCV_MASTER_ABORT 0x2000  /**< Received master abort */
#define PCI_STATUS_SIG_SYS_ERROR    0x4000  /**< Signaled system error */
#define PCI_STATUS_PARITY_ERROR     0x8000  /**< Detected parity error */

/**
 * @brief PCI Class Codes
 */
#define PCI_CLASS_UNCLASSIFIED      0x00    /**< Unclassified */
#define PCI_CLASS_STORAGE           0x01    /**< Mass storage controller */
#define PCI_CLASS_NETWORK           0x02    /**< Network controller */
#define PCI_CLASS_DISPLAY           0x03    /**< Display controller */
#define PCI_CLASS_MULTIMEDIA        0x04    /**< Multimedia controller */
#define PCI_CLASS_MEMORY            0x05    /**< Memory controller */
#define PCI_CLASS_BRIDGE            0x06    /**< Bridge device */
#define PCI_CLASS_COMMUNICATION     0x07    /**< Communication controller */
#define PCI_CLASS_SYSTEM            0x08    /**< System peripheral */
#define PCI_CLASS_INPUT             0x09    /**< Input device */
#define PCI_CLASS_DOCKING           0x0A    /**< Docking station */
#define PCI_CLASS_PROCESSOR         0x0B    /**< Processor */
#define PCI_CLASS_SERIAL_BUS        0x0C    /**< Serial bus controller */
#define PCI_CLASS_WIRELESS          0x0D    /**< Wireless controller */
#define PCI_CLASS_INTELLIGENT_IO    0x0E    /**< Intelligent I/O controller */
#define PCI_CLASS_SATELLITE         0x0F    /**< Satellite controller */
#define PCI_CLASS_ENCRYPTION        0x10    /**< Encryption/Decryption */
#define PCI_CLASS_DATA_ACQ          0x11    /**< Data acquisition controller */

/**
 * @brief PCI Subclasses for Serial Bus (0x0C)
 */
#define PCI_SUBCLASS_SERIAL_FIREWIRE    0x00    /**< FireWire (IEEE 1394) */
#define PCI_SUBCLASS_SERIAL_ACCESS_BUS  0x01    /**< ACCESS bus */
#define PCI_SUBCLASS_SERIAL_SSA         0x02    /**< SSA */
#define PCI_SUBCLASS_SERIAL_USB         0x03    /**< USB controller */
#define PCI_SUBCLASS_SERIAL_FIBER       0x04    /**< Fiber channel */
#define PCI_SUBCLASS_SERIAL_SMBUS       0x05    /**< SMBus */
#define PCI_SUBCLASS_SERIAL_THUNDERBOLT 0x0A    /**< Thunderbolt */

/**
 * @brief PCI Header Types
 */
#define PCI_HEADER_TYPE_NORMAL      0x00    /**< Normal device */
#define PCI_HEADER_TYPE_BRIDGE      0x01    /**< PCI-to-PCI bridge */
#define PCI_HEADER_TYPE_CARDBUS     0x02    /**< CardBus bridge */
#define PCI_HEADER_TYPE_MULTIFUNCTION 0x80  /**< Multi-function bit */

/**
 * @brief PCI Capability IDs
 */
#define PCI_CAP_ID_PM               0x01    /**< Power management */
#define PCI_CAP_ID_AGP              0x02    /**< AGP */
#define PCI_CAP_ID_VPD              0x03    /**< Vital product data */
#define PCI_CAP_ID_SLOTID           0x04    /**< Slot identification */
#define PCI_CAP_ID_MSI              0x05    /**< Message signaled interrupts */
#define PCI_CAP_ID_CHSWP            0x06    /**< CompactPCI hot swap */
#define PCI_CAP_ID_PCIX             0x07    /**< PCI-X */
#define PCI_CAP_ID_HT               0x08    /**< HyperTransport */
#define PCI_CAP_ID_VNDR             0x09    /**< Vendor specific */
#define PCI_CAP_ID_DBG              0x0A    /**< Debug port */
#define PCI_CAP_ID_CCRC             0x0B    /**< CompactPCI central resource */
#define PCI_CAP_ID_SHPC             0x0C    /**< PCI standard hot-plug */
#define PCI_CAP_ID_SSVID            0x0D    /**< Bridge subsystem vendor/device */
#define PCI_CAP_ID_AGP3             0x0E    /**< AGP 8x */
#define PCI_CAP_ID_SECDEV           0x0F    /**< Secure device */
#define PCI_CAP_ID_EXP              0x10    /**< PCI Express */
#define PCI_CAP_ID_MSIX             0x11    /**< MSI-X */
#define PCI_CAP_ID_SATA             0x12    /**< SATA data/index config */
#define PCI_CAP_ID_AF               0x13    /**< Advanced features */

/**
 * @brief PCIe Extended Capability IDs
 */
#define PCIE_EXT_CAP_ID_AER         0x0001  /**< Advanced error reporting */
#define PCIE_EXT_CAP_ID_VC          0x0002  /**< Virtual channel */
#define PCIE_EXT_CAP_ID_DSN         0x0003  /**< Device serial number */
#define PCIE_EXT_CAP_ID_PWR         0x0004  /**< Power budgeting */
#define PCIE_EXT_CAP_ID_RCLD        0x0005  /**< Root complex link declaration */
#define PCIE_EXT_CAP_ID_RCILC       0x0006  /**< Root complex internal link control */
#define PCIE_EXT_CAP_ID_RCEC        0x0007  /**< Root complex event collector */
#define PCIE_EXT_CAP_ID_MFVC        0x0008  /**< Multi-function virtual channel */
#define PCIE_EXT_CAP_ID_VC9         0x0009  /**< Virtual channel (second ID) */
#define PCIE_EXT_CAP_ID_RCRB        0x000A  /**< Root complex register block */
#define PCIE_EXT_CAP_ID_VNDR        0x000B  /**< Vendor specific */
#define PCIE_EXT_CAP_ID_CAC         0x000C  /**< Configuration access correlation */
#define PCIE_EXT_CAP_ID_ACS         0x000D  /**< Access control services */
#define PCIE_EXT_CAP_ID_ARI         0x000E  /**< Alternative routing-ID */
#define PCIE_EXT_CAP_ID_ATS         0x000F  /**< Address translation services */
#define PCIE_EXT_CAP_ID_SRIOV       0x0010  /**< Single root I/O virtualization */
#define PCIE_EXT_CAP_ID_MRIOV       0x0011  /**< Multi root I/O virtualization */
#define PCIE_EXT_CAP_ID_MCAST       0x0012  /**< Multicast */
#define PCIE_EXT_CAP_ID_PRI         0x0013  /**< Page request interface */
#define PCIE_EXT_CAP_ID_REBAR       0x0015  /**< Resizable BAR */
#define PCIE_EXT_CAP_ID_DPA         0x0016  /**< Dynamic power allocation */
#define PCIE_EXT_CAP_ID_TPH         0x0017  /**< TPH requester */
#define PCIE_EXT_CAP_ID_LTR         0x0018  /**< Latency tolerance reporting */
#define PCIE_EXT_CAP_ID_SECPCI      0x0019  /**< Secondary PCIe capability */
#define PCIE_EXT_CAP_ID_PMUX        0x001A  /**< Protocol multiplexing */
#define PCIE_EXT_CAP_ID_PASID       0x001B  /**< Process address space ID */
#define PCIE_EXT_CAP_ID_LNR         0x001C  /**< LN requester */
#define PCIE_EXT_CAP_ID_DPC         0x001D  /**< Downstream port containment */
#define PCIE_EXT_CAP_ID_L1PM        0x001E  /**< L1 PM substates */
#define PCIE_EXT_CAP_ID_PTM         0x001F  /**< Precision time measurement */

/**
 * @brief PCI BAR (Base Address Register) structure
 */
typedef struct _PCI_BAR {
    UINT64  PhysicalAddress;        /**< Physical address */
    UINT64  Size;                   /**< Size in bytes */
    UINT32  Flags;                  /**< BAR flags */
    BOOLEAN bIsMem;                 /**< TRUE if memory space, FALSE if I/O */
    BOOLEAN bIs64Bit;               /**< TRUE if 64-bit BAR */
    BOOLEAN bIsPrefetchable;        /**< TRUE if prefetchable */
} PCI_BAR;

/**
 * @brief PCI device location
 */
typedef struct _PCI_LOCATION {
    UINT16  Segment;                /**< Segment (group) number */
    UINT8   Bus;                    /**< Bus number */
    UINT8   Device;                 /**< Device number (0-31) */
    UINT8   Function;               /**< Function number (0-7) */
} PCI_LOCATION;

/**
 * @brief PCI device information
 */
typedef struct _PCI_DEVICE_INFO {
    PCI_LOCATION    Location;           /**< Device location */
    UINT16          VendorID;           /**< Vendor ID */
    UINT16          DeviceID;           /**< Device ID */
    UINT16          SubsysVendorID;     /**< Subsystem vendor ID */
    UINT16          SubsysDeviceID;     /**< Subsystem device ID */
    UINT8           RevisionID;         /**< Revision ID */
    UINT8           ClassCode;          /**< Class code */
    UINT8           SubClass;           /**< Subclass */
    UINT8           ProgIf;             /**< Programming interface */
    UINT8           HeaderType;         /**< Header type */
    UINT8           InterruptLine;      /**< Interrupt line */
    UINT8           InterruptPin;       /**< Interrupt pin */
    PCI_BAR         BARs[6];            /**< Base address registers */
} PCI_DEVICE_INFO;

/**
 * @brief MSI capability structure
 */
typedef struct _PCI_MSI_CAPABILITY {
    UINT8   CapabilityID;           /**< Capability ID (0x05) */
    UINT8   NextPointer;            /**< Next capability pointer */
    UINT16  MessageControl;         /**< Message control */
    UINT32  MessageAddress;         /**< Message address (lower 32 bits) */
    UINT32  MessageAddressHi;       /**< Message address (upper 32 bits, 64-bit only) */
    UINT16  MessageData;            /**< Message data */
    UINT32  MaskBits;               /**< Mask bits (per-vector masking) */
    UINT32  PendingBits;            /**< Pending bits */
} PCI_MSI_CAPABILITY;

/**
 * @brief MSI-X capability structure
 */
typedef struct _PCI_MSIX_CAPABILITY {
    UINT8   CapabilityID;           /**< Capability ID (0x11) */
    UINT8   NextPointer;            /**< Next capability pointer */
    UINT16  MessageControl;         /**< Message control */
    UINT32  TableOffsetBIR;         /**< Table offset and BIR */
    UINT32  PBAOffsetBIR;           /**< PBA offset and BIR */
} PCI_MSIX_CAPABILITY;

/**
 * @brief IIOPCIDevice - PCI Device interface
 *
 * This interface represents a single PCI/PCIe device and provides methods
 * for configuration space access, BAR management, and interrupt setup.
 */
#undef INTERFACE
#define INTERFACE IIOPCIDevice

DECLARE_INTERFACE_(IIOPCIDevice, IIOService)
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

    // IIOPCIDevice methods

    /**
     * @brief Get PCI device information
     *
     * Retrieves comprehensive device information including location,
     * IDs, class codes, and BAR configuration.
     *
     * @param pDeviceInfo   Receives device information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        PCI_DEVICE_INFO *pDeviceInfo
        ) PURE;

    /**
     * @brief Read from configuration space
     *
     * Reads a value from the device's PCI configuration space.
     *
     * @param uOffset       Configuration space offset
     * @param uSize         Size of read (1, 2, or 4 bytes)
     * @param puValue       Receives the value
     *
     * @retval IO_SUCCESS       Read successful
     * @retval IO_BAD_ARGUMENT  Invalid offset or size
     */
    STDMETHOD_(IO_RETURN, ConfigRead)(THIS_
        UINT32 uOffset,
        UINT32 uSize,
        UINT32 *puValue
        ) PURE;

    /**
     * @brief Write to configuration space
     *
     * Writes a value to the device's PCI configuration space.
     *
     * @param uOffset       Configuration space offset
     * @param uSize         Size of write (1, 2, or 4 bytes)
     * @param uValue        Value to write
     *
     * @retval IO_SUCCESS       Write successful
     * @retval IO_BAD_ARGUMENT  Invalid offset or size
     */
    STDMETHOD_(IO_RETURN, ConfigWrite)(THIS_
        UINT32 uOffset,
        UINT32 uSize,
        UINT32 uValue
        ) PURE;

    /**
     * @brief Map BAR into memory
     *
     * Maps a Base Address Register into kernel virtual memory.
     *
     * @param uBARIndex     BAR index (0-5)
     * @param ppAddress     Receives mapped address
     * @param pcbSize       Receives mapped size
     *
     * @retval IO_SUCCESS       BAR mapped successfully
     * @retval IO_BAD_ARGUMENT  Invalid BAR index
     * @retval IO_VM_ERROR      Mapping failed
     */
    STDMETHOD_(IO_RETURN, MapBAR)(THIS_
        UINT32 uBARIndex,
        VOID **ppAddress,
        UINT64 *pcbSize
        ) PURE;

    /**
     * @brief Unmap BAR from memory
     *
     * Unmaps a previously mapped BAR.
     *
     * @param uBARIndex     BAR index (0-5)
     *
     * @retval IO_SUCCESS       BAR unmapped successfully
     * @retval IO_BAD_ARGUMENT  Invalid BAR index
     */
    STDMETHOD_(IO_RETURN, UnmapBAR)(THIS_
        UINT32 uBARIndex
        ) PURE;

    /**
     * @brief Enable bus mastering
     *
     * Enables the device to perform DMA operations.
     *
     * @param bEnable       TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS       Bus mastering configured
     */
    STDMETHOD_(IO_RETURN, SetBusMaster)(THIS_
        BOOLEAN bEnable
        ) PURE;

    /**
     * @brief Enable memory/IO space
     *
     * Enables or disables memory and I/O space access.
     *
     * @param bMemory       TRUE to enable memory space
     * @param bIO           TRUE to enable I/O space
     *
     * @retval IO_SUCCESS       Spaces configured
     */
    STDMETHOD_(IO_RETURN, SetMemoryIOEnable)(THIS_
        BOOLEAN bMemory,
        BOOLEAN bIO
        ) PURE;

    /**
     * @brief Find capability
     *
     * Finds a PCI capability in the capability list.
     *
     * @param uCapabilityID Capability ID to find
     * @param puOffset      Receives offset of capability
     *
     * @retval IO_SUCCESS       Capability found
     * @retval IO_NO_MATCH      Capability not found
     */
    STDMETHOD_(IO_RETURN, FindCapability)(THIS_
        UINT8 uCapabilityID,
        UINT32 *puOffset
        ) PURE;

    /**
     * @brief Setup MSI interrupts
     *
     * Configures Message Signaled Interrupts.
     *
     * @param uNumVectors   Number of vectors to allocate
     * @param pfnHandler    Interrupt handler function
     * @param pContext      Handler context
     *
     * @retval IO_SUCCESS       MSI configured successfully
     * @retval IO_UNSUPPORTED   MSI not supported
     * @retval IO_NO_RESOURCES  Insufficient interrupt vectors
     */
    STDMETHOD_(IO_RETURN, SetupMSI)(THIS_
        UINT32 uNumVectors,
        VOID (*pfnHandler)(VOID *pContext, UINT32 uVector),
        VOID *pContext
        ) PURE;

    /**
     * @brief Setup MSI-X interrupts
     *
     * Configures MSI-X (extended message signaled interrupts).
     *
     * @param uNumVectors   Number of vectors to allocate
     * @param pfnHandler    Interrupt handler function
     * @param pContext      Handler context
     *
     * @retval IO_SUCCESS       MSI-X configured successfully
     * @retval IO_UNSUPPORTED   MSI-X not supported
     * @retval IO_NO_RESOURCES  Insufficient interrupt vectors
     */
    STDMETHOD_(IO_RETURN, SetupMSIX)(THIS_
        UINT32 uNumVectors,
        VOID (*pfnHandler)(VOID *pContext, UINT32 uVector),
        VOID *pContext
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOPCIBridge - PCI Bridge interface
 *
 * This interface represents a PCI-to-PCI bridge and provides methods
 * for bus enumeration, secondary bus management, and hot-plug support.
 */
#undef INTERFACE
#define INTERFACE IIOPCIBridge

DECLARE_INTERFACE_(IIOPCIBridge, IIOPCIDevice)
{
    // All IIOPCIDevice methods inherited...

    /**
     * @brief Enumerate devices on secondary bus
     *
     * Scans the secondary bus and enumerates all connected devices.
     *
     * @retval IO_SUCCESS       Enumeration successful
     * @retval IO_ERROR         Enumeration failed
     */
    STDMETHOD_(IO_RETURN, EnumerateBus)(THIS) PURE;

    /**
     * @brief Get secondary bus number
     *
     * Returns the bus number of the bridge's secondary bus.
     *
     * @param puBusNumber   Receives bus number
     *
     * @retval IO_SUCCESS       Bus number retrieved
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetSecondaryBusNumber)(THIS_
        UINT8 *puBusNumber
        ) PURE;

    /**
     * @brief Enable/disable hot-plug
     *
     * Enables or disables hot-plug capability on this bridge.
     *
     * @param bEnable       TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS       Hot-plug configured
     * @retval IO_UNSUPPORTED   Hot-plug not supported
     */
    STDMETHOD_(IO_RETURN, SetHotplugEnable)(THIS_
        BOOLEAN bEnable
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOPCIDevice methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOPCIDevice_GetDeviceInfo(p,a)             (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOPCIDevice_ConfigRead(p,a,b,c)            (p)->lpVtbl->ConfigRead(p,a,b,c)
#define IIOPCIDevice_ConfigWrite(p,a,b,c)           (p)->lpVtbl->ConfigWrite(p,a,b,c)
#define IIOPCIDevice_MapBAR(p,a,b,c)                (p)->lpVtbl->MapBAR(p,a,b,c)
#define IIOPCIDevice_UnmapBAR(p,a)                  (p)->lpVtbl->UnmapBAR(p,a)
#define IIOPCIDevice_SetBusMaster(p,a)              (p)->lpVtbl->SetBusMaster(p,a)
#define IIOPCIDevice_SetMemoryIOEnable(p,a,b)       (p)->lpVtbl->SetMemoryIOEnable(p,a,b)
#define IIOPCIDevice_FindCapability(p,a,b)          (p)->lpVtbl->FindCapability(p,a,b)
#define IIOPCIDevice_SetupMSI(p,a,b,c)              (p)->lpVtbl->SetupMSI(p,a,b,c)
#define IIOPCIDevice_SetupMSIX(p,a,b,c)             (p)->lpVtbl->SetupMSIX(p,a,b,c)

#define IIOPCIBridge_EnumerateBus(p)                (p)->lpVtbl->EnumerateBus(p)
#define IIOPCIBridge_GetSecondaryBusNumber(p,a)     (p)->lpVtbl->GetSecondaryBusNumber(p,a)
#define IIOPCIBridge_SetHotplugEnable(p,a)          (p)->lpVtbl->SetHotplugEnable(p,a)

#endif

/**
 * @brief Create a PCI device instance
 *
 * @param pLocation     PCI device location
 * @param ppDevice      Receives pointer to device interface
 *
 * @retval IO_SUCCESS   Device created successfully
 * @retval IO_NO_MEMORY Insufficient memory
 */
IO_RETURN
PCIDeviceCreate(
    CONST PCI_LOCATION *pLocation,
    IIOPCIDevice **ppDevice
    );

/**
 * @brief Scan PCI bus for devices
 *
 * @param uBusNumber    Bus number to scan
 * @param ppDevices     Receives array of discovered devices
 * @param puDeviceCount On input: max devices; On output: actual count
 *
 * @retval IO_SUCCESS   Scan successful
 */
IO_RETURN
PCIScanBus(
    UINT8 uBusNumber,
    IIOPCIDevice **ppDevices,
    UINT32 *puDeviceCount
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_PCIE_H */
