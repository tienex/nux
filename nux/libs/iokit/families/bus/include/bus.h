/**
 * @file bus.h
 * @brief Bus Family Interface - Unified Bus Abstraction
 *
 * This header defines the Bus family interface providing a unified abstraction
 * layer for all system buses regardless of type (PCIe, USB, ISA, SATA, etc.).
 * This allows upper layers to enumerate and manage devices on any bus type
 * through a common interface.
 *
 * The Bus family provides:
 * - Protocol-agnostic bus operations and device enumeration
 * - Unified hot-plug and device attachment/detachment handling
 * - Common bus topology management and traversal
 * - Bus reset and power management
 * - Device discovery and matching services
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_BUS_H
#define IOKIT_BUS_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOBus interface GUID
 * {C4D5E6F7-A8B9-4C5D-9E8F-0A1B2C3D4E5F}
 */
DEFINE_GUID(IID_IIOBus,
    0xC4D5E6F7, 0xA8B9, 0x4C5D, 0x9E, 0x8F, 0x0A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F);

/**
 * @brief IIOBusDevice interface GUID
 * {D5E6F7A8-B9C0-4D5E-8F9A-1B2C3D4E5F6A}
 */
DEFINE_GUID(IID_IIOBusDevice,
    0xD5E6F7A8, 0xB9C0, 0x4D5E, 0x8F, 0x9A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F, 0x6A);

/**
 * @brief Bus Types
 *
 * Identifies the type of system bus.
 */
typedef enum _BUS_TYPE {
    BUS_TYPE_UNKNOWN        = 0x00,     /**< Unknown or unidentified bus */
    BUS_TYPE_PCI            = 0x01,     /**< PCI (Peripheral Component Interconnect) */
    BUS_TYPE_PCIE           = 0x02,     /**< PCI Express */
    BUS_TYPE_USB            = 0x03,     /**< Universal Serial Bus */
    BUS_TYPE_ISA            = 0x04,     /**< ISA (Industry Standard Architecture) */
    BUS_TYPE_EISA           = 0x05,     /**< EISA (Extended ISA) */
    BUS_TYPE_SATA           = 0x06,     /**< Serial ATA (as a bus) */
    BUS_TYPE_SAS            = 0x07,     /**< Serial Attached SCSI */
    BUS_TYPE_SCSI           = 0x08,     /**< SCSI bus */
    BUS_TYPE_IDE            = 0x09,     /**< IDE/PATA bus */
    BUS_TYPE_I2C            = 0x0A,     /**< I2C (Inter-Integrated Circuit) */
    BUS_TYPE_SPI            = 0x0B,     /**< SPI (Serial Peripheral Interface) */
    BUS_TYPE_THUNDERBOLT    = 0x0C,     /**< Thunderbolt */
    BUS_TYPE_FIREWIRE       = 0x0D,     /**< IEEE 1394 (FireWire) */
    BUS_TYPE_PCMCIA         = 0x0E,     /**< PCMCIA/PC Card */
    BUS_TYPE_CARDBUS        = 0x0F,     /**< CardBus */
    BUS_TYPE_AGP            = 0x10,     /**< AGP (Accelerated Graphics Port) */
    BUS_TYPE_HYPERTRANSPORT = 0x11,     /**< HyperTransport */
    BUS_TYPE_INFINIBAND     = 0x12,     /**< InfiniBand */
    BUS_TYPE_VME            = 0x13,     /**< VMEbus */
    BUS_TYPE_NUBUS          = 0x14,     /**< NuBus (legacy Apple) */
    BUS_TYPE_ZORRO          = 0x15,     /**< Zorro (Amiga) */
    BUS_TYPE_SBUS           = 0x16,     /**< SBus (Sun) */
    BUS_TYPE_VLB            = 0x17,     /**< VESA Local Bus */
    BUS_TYPE_MCA            = 0x18,     /**< MCA (Micro Channel Architecture) */
    BUS_TYPE_PLATFORM       = 0x19,     /**< Platform/System bus */
    BUS_TYPE_VIRTUAL        = 0x1A,     /**< Virtual bus */
    BUS_TYPE_SOC            = 0x1B,     /**< SoC internal bus */
    BUS_TYPE_CXL            = 0x1C,     /**< Compute Express Link */
    BUS_TYPE_CCIX           = 0x1D,     /**< Cache Coherent Interconnect for Accelerators */
} BUS_TYPE;

/**
 * @brief Bus Speed Types
 */
typedef enum _BUS_SPEED {
    BUS_SPEED_UNKNOWN       = 0,        /**< Unknown speed */
    BUS_SPEED_LOW           = 1,        /**< Low speed */
    BUS_SPEED_FULL          = 2,        /**< Full speed */
    BUS_SPEED_HIGH          = 3,        /**< High speed */
    BUS_SPEED_SUPER         = 4,        /**< Super speed */
    BUS_SPEED_ULTRA         = 5,        /**< Ultra speed */
} BUS_SPEED;

/**
 * @brief Bus Power States
 */
typedef enum _BUS_POWER_STATE {
    BUS_POWER_OFF           = 0,        /**< Bus powered off */
    BUS_POWER_SUSPEND       = 1,        /**< Bus suspended (low power) */
    BUS_POWER_ON            = 2,        /**< Bus fully powered */
    BUS_POWER_RESET         = 3,        /**< Bus in reset state */
} BUS_POWER_STATE;

/**
 * @brief Bus Capabilities (Bitfield)
 */
#define BUS_CAP_HOT_PLUG            0x00000001  /**< Hot-plug support */
#define BUS_CAP_HOT_SWAP            0x00000002  /**< Hot-swap support */
#define BUS_CAP_POWER_MANAGEMENT    0x00000004  /**< Power management */
#define BUS_CAP_ENUMERATION         0x00000008  /**< Dynamic enumeration */
#define BUS_CAP_TOPOLOGY_INFO       0x00000010  /**< Topology information available */
#define BUS_CAP_BANDWIDTH_INFO      0x00000020  /**< Bandwidth information available */
#define BUS_CAP_ERROR_HANDLING      0x00000040  /**< Advanced error handling */
#define BUS_CAP_IOMMU               0x00000080  /**< IOMMU/SMMU support */
#define BUS_CAP_MSI                 0x00000100  /**< MSI interrupt support */
#define BUS_CAP_MSIX                0x00000200  /**< MSI-X interrupt support */
#define BUS_CAP_DMA                 0x00000400  /**< DMA capable */
#define BUS_CAP_64BIT_ADDRESSING    0x00000800  /**< 64-bit addressing */
#define BUS_CAP_HIERARCHICAL        0x00001000  /**< Hierarchical topology */
#define BUS_CAP_PEER_TO_PEER        0x00002000  /**< Peer-to-peer transfers */
#define BUS_CAP_LINK_TRAINING       0x00004000  /**< Link training support */
#define BUS_CAP_LANE_REVERSAL       0x00008000  /**< Lane reversal support */
#define BUS_CAP_ASYNC_NOTIFY        0x00010000  /**< Asynchronous notifications */
#define BUS_CAP_SEGMENT_SUPPORT     0x00020000  /**< Multiple segment support */

/**
 * @brief Bus Status Flags
 */
#define BUS_STATUS_ACTIVE           0x00000001  /**< Bus is active */
#define BUS_STATUS_ENUMERATED       0x00000002  /**< Devices enumerated */
#define BUS_STATUS_ERROR            0x00000004  /**< Bus error condition */
#define BUS_STATUS_RESETTING        0x00000008  /**< Bus reset in progress */
#define BUS_STATUS_SUSPENDED        0x00000010  /**< Bus suspended */
#define BUS_STATUS_LINK_UP          0x00000020  /**< Link is up/trained */
#define BUS_STATUS_LINK_DOWN        0x00000040  /**< Link is down */
#define BUS_STATUS_LINK_TRAINING    0x00000080  /**< Link training */
#define BUS_STATUS_BANDWIDTH_LIMITED 0x00000100 /**< Bandwidth limited */

/**
 * @brief Device Attachment Flags
 */
#define BUS_ATTACH_FLAG_PROBE       0x00000001  /**< Probe device during attach */
#define BUS_ATTACH_FLAG_START       0x00000002  /**< Start device during attach */
#define BUS_ATTACH_FLAG_ASYNC       0x00000004  /**< Asynchronous attachment */
#define BUS_ATTACH_FLAG_FORCE       0x00000008  /**< Force attachment */

/**
 * @brief Device Detachment Flags
 */
#define BUS_DETACH_FLAG_GRACEFUL    0x00000001  /**< Graceful detachment */
#define BUS_DETACH_FLAG_FORCE       0x00000002  /**< Force detachment */
#define BUS_DETACH_FLAG_ASYNC       0x00000004  /**< Asynchronous detachment */
#define BUS_DETACH_FLAG_EJECT       0x00000008  /**< Eject device after detach */

/**
 * @brief Bus Information
 *
 * Comprehensive information about a system bus including its type,
 * capabilities, topology, and performance characteristics.
 */
typedef struct _BUS_INFO {
    BUS_TYPE            Type;                   /**< Bus type */
    CHAR8               BusName[64];            /**< Bus name/description */
    UINT32              BusNumber;              /**< Bus number/ID */
    UINT32              Segment;                /**< Bus segment (for multi-segment buses) */

    // Capabilities and Features
    UINT32              Capabilities;           /**< Capability flags (BUS_CAP_*) */
    UINT32              MaxDevices;             /**< Maximum devices supported */
    UINT32              NumDevices;             /**< Current number of devices */
    UINT32              MaxFunctions;           /**< Max functions per device (if applicable) */

    // Topology
    UINT32              ParentBus;              /**< Parent bus number (hierarchical) */
    UINT32              BridgeDevice;           /**< Bridge device number (if bridge) */
    UINT8               Depth;                  /**< Depth in bus hierarchy */
    BOOLEAN             bIsRootBus;             /**< Is this a root bus */
    BOOLEAN             bIsBridge;              /**< Is this a bridge */

    // Performance
    UINT64              MaxBandwidth;           /**< Maximum bandwidth (bytes/sec) */
    UINT64              AvailableBandwidth;     /**< Available bandwidth (bytes/sec) */
    BUS_SPEED           Speed;                  /**< Bus speed category */
    UINT32              ClockFrequency;         /**< Clock frequency (Hz) */
    UINT32              DataWidth;              /**< Data width (bits) */
    UINT32              NumLanes;               /**< Number of lanes (PCIe, etc.) */

    // Power Management
    BUS_POWER_STATE     PowerState;             /**< Current power state */
    BOOLEAN             bPowerManagementSupport;/**< Power management supported */

    // Status
    UINT32              Status;                 /**< Status flags (BUS_STATUS_*) */
    UINT32              ErrorCount;             /**< Total error count */
    UINT64              Uptime;                 /**< Bus uptime (seconds) */

    // Vendor Information
    UINT16              VendorID;               /**< Vendor ID (if applicable) */
    UINT16              DeviceID;               /**< Device ID (if applicable) */
    CHAR8               VendorName[40];         /**< Vendor name */
} BUS_INFO;

/**
 * @brief Bus Device Location
 *
 * Describes the location of a device on a bus using bus-specific addressing.
 */
typedef struct _BUS_DEVICE_LOCATION {
    BUS_TYPE            BusType;                /**< Bus type */
    UINT32              BusNumber;              /**< Bus number */
    UINT32              Segment;                /**< Segment number */
    UINT32              Device;                 /**< Device number/address */
    UINT32              Function;               /**< Function number (multi-function) */
    UINT32              Port;                   /**< Port number (USB, SATA, etc.) */
    UINT8               Channel;                /**< Channel (IDE, etc.) */
    UINT8               Target;                 /**< Target ID (SCSI) */
    UINT8               LUN;                    /**< Logical Unit Number (SCSI) */
    CHAR8               Path[256];              /**< Full device path string */
} BUS_DEVICE_LOCATION;

/**
 * @brief Bus Device Information
 *
 * Information about a device attached to a bus.
 */
typedef struct _BUS_DEVICE_INFO {
    BUS_DEVICE_LOCATION Location;               /**< Device location on bus */
    CHAR8               DeviceName[64];         /**< Device name */
    CHAR8               DriverName[64];         /**< Attached driver name */

    // Classification
    UINT16              VendorID;               /**< Vendor ID */
    UINT16              DeviceID;               /**< Device ID */
    UINT16              SubsystemVendorID;      /**< Subsystem vendor ID */
    UINT16              SubsystemDeviceID;      /**< Subsystem device ID */
    UINT8               ClassCode;              /**< Class code */
    UINT8               SubClass;               /**< Subclass */
    UINT8               ProgIf;                 /**< Programming interface */
    UINT8               RevisionID;             /**< Revision ID */

    // Capabilities
    UINT32              DeviceCapabilities;     /**< Device-specific capabilities */
    UINT64              ResourceSize;           /**< Total resource size (memory, etc.) */
    UINT32              InterruptCount;         /**< Number of interrupts */

    // State
    BOOLEAN             bPresent;               /**< Device is present */
    BOOLEAN             bEnabled;               /**< Device is enabled */
    BOOLEAN             bAttached;              /**< Driver attached */
    BOOLEAN             bStarted;               /**< Device started */
    UINT32              PowerState;             /**< Current power state */
} BUS_DEVICE_INFO;

/**
 * @brief Bus Topology Node
 *
 * Represents a node in the bus topology tree.
 */
typedef struct _BUS_TOPOLOGY_NODE {
    BUS_DEVICE_LOCATION Location;               /**< Node location */
    UINT32              NodeType;               /**< Node type (bus/bridge/device) */
    UINT32              NumChildren;            /**< Number of child nodes */
    struct _BUS_TOPOLOGY_NODE *pParent;         /**< Parent node */
    struct _BUS_TOPOLOGY_NODE **ppChildren;     /**< Array of child nodes */
    VOID               *pDeviceData;            /**< Device-specific data */
} BUS_TOPOLOGY_NODE;

/**
 * @brief Bus Reset Types
 */
typedef enum _BUS_RESET_TYPE {
    BUS_RESET_SOFT              = 0,    /**< Soft reset (if supported) */
    BUS_RESET_HARD              = 1,    /**< Hard reset */
    BUS_RESET_FUNCTION          = 2,    /**< Function-level reset (FLR) */
    BUS_RESET_HOT               = 3,    /**< Hot reset */
    BUS_RESET_LINK              = 4,    /**< Link reset */
} BUS_RESET_TYPE;

/**
 * @brief Bus Event Types
 */
typedef enum _BUS_EVENT_TYPE {
    BUS_EVENT_DEVICE_ATTACHED   = 1,    /**< Device attached */
    BUS_EVENT_DEVICE_DETACHED   = 2,    /**< Device detached */
    BUS_EVENT_DEVICE_CHANGED    = 3,    /**< Device changed */
    BUS_EVENT_BUS_RESET         = 4,    /**< Bus was reset */
    BUS_EVENT_ERROR             = 5,    /**< Bus error occurred */
    BUS_EVENT_POWER_CHANGE      = 6,    /**< Power state changed */
    BUS_EVENT_LINK_CHANGE       = 7,    /**< Link state changed */
} BUS_EVENT_TYPE;

/**
 * @brief Bus Event Callback
 *
 * Callback function type for bus events.
 *
 * @param EventType         Type of event
 * @param pBus              Bus that generated the event
 * @param pDevice           Device involved (may be NULL)
 * @param pContext          User-provided context
 */
typedef VOID (*BUS_EVENT_CALLBACK)(
    BUS_EVENT_TYPE EventType,
    IIOBus *pBus,
    IIOBusDevice *pDevice,
    VOID *pContext
);

//
// Forward declarations
//
DECLARE_INTERFACE_(IIOBus, IIOService);
DECLARE_INTERFACE_(IIOBusDevice, IIOService);

/**
 * @brief IIOBus - Bus Interface
 *
 * This interface represents a system bus (PCIe, USB, ISA, etc.) and provides
 * methods for device enumeration, topology management, and bus control.
 */
#undef INTERFACE
#define INTERFACE IIOBus

DECLARE_INTERFACE_(IIOBus, IIOService)
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

    // IIOBus methods

    /**
     * @brief Get bus information
     *
     * Retrieves comprehensive information about the bus including type,
     * capabilities, topology, and performance characteristics.
     *
     * @param pBusInfo          Receives bus information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetBusInfo)(THIS_
        BUS_INFO *pBusInfo
        ) PURE;

    /**
     * @brief Enumerate devices on bus
     *
     * Scans the bus and enumerates all connected devices. Creates
     * device instances for each discovered device and notifies
     * the system for driver matching.
     *
     * @retval IO_SUCCESS       Enumeration completed successfully
     * @retval IO_ERROR         Enumeration failed
     */
    STDMETHOD_(IO_RETURN, EnumerateDevices)(THIS) PURE;

    /**
     * @brief Get device count
     *
     * Returns the number of devices currently attached to this bus.
     *
     * @param puCount           Receives device count
     *
     * @retval IO_SUCCESS       Count retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDeviceCount)(THIS_
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Get device by index
     *
     * Retrieves a bus device interface by index.
     *
     * @param uIndex            Device index (0-based)
     * @param ppDevice          Receives bus device interface
     *
     * @retval IO_SUCCESS       Device retrieved successfully
     * @retval IO_NO_DEVICE     Invalid index
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDevice)(THIS_
        UINT32 uIndex,
        IIOBusDevice **ppDevice
        ) PURE;

    /**
     * @brief Get device by location
     *
     * Retrieves a bus device by its bus-specific location/address.
     *
     * @param pLocation         Device location
     * @param ppDevice          Receives bus device interface
     *
     * @retval IO_SUCCESS       Device retrieved successfully
     * @retval IO_NO_DEVICE     Device not found
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDeviceByLocation)(THIS_
        CONST BUS_DEVICE_LOCATION *pLocation,
        IIOBusDevice **ppDevice
        ) PURE;

    /**
     * @brief Reset bus
     *
     * Performs a bus-level reset. The type of reset depends on the
     * bus type and specified reset type.
     *
     * @param ResetType         Type of reset to perform
     *
     * @retval IO_SUCCESS       Reset completed successfully
     * @retval IO_UNSUPPORTED   Reset type not supported
     * @retval IO_ERROR         Reset failed
     */
    STDMETHOD_(IO_RETURN, ResetBus)(THIS_
        BUS_RESET_TYPE ResetType
        ) PURE;

    /**
     * @brief Get bus topology
     *
     * Retrieves the hierarchical topology of the bus and its devices.
     *
     * @param ppTopology        Receives topology root node
     *
     * @retval IO_SUCCESS       Topology retrieved successfully
     * @retval IO_UNSUPPORTED   Topology not available
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetTopology)(THIS_
        BUS_TOPOLOGY_NODE **ppTopology
        ) PURE;

    /**
     * @brief Enable/disable hot-plug support
     *
     * Enables or disables hot-plug device detection on this bus.
     *
     * @param bEnable           TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS       Hot-plug configured successfully
     * @retval IO_UNSUPPORTED   Hot-plug not supported
     */
    STDMETHOD_(IO_RETURN, SetHotPlugEnable)(THIS_
        BOOLEAN bEnable
        ) PURE;

    /**
     * @brief Register event callback
     *
     * Registers a callback function to receive bus events (device
     * attach/detach, errors, etc.)
     *
     * @param pfnCallback       Callback function
     * @param pContext          User context passed to callback
     *
     * @retval IO_SUCCESS       Callback registered successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, RegisterEventCallback)(THIS_
        BUS_EVENT_CALLBACK pfnCallback,
        VOID *pContext
        ) PURE;

    /**
     * @brief Unregister event callback
     *
     * Unregisters a previously registered event callback.
     *
     * @param pfnCallback       Callback function to unregister
     *
     * @retval IO_SUCCESS       Callback unregistered successfully
     * @retval IO_NO_MATCH      Callback not found
     */
    STDMETHOD_(IO_RETURN, UnregisterEventCallback)(THIS_
        BUS_EVENT_CALLBACK pfnCallback
        ) PURE;

    /**
     * @brief Set bus power state
     *
     * Changes the power state of the entire bus.
     *
     * @param PowerState        Desired power state
     *
     * @retval IO_SUCCESS       Power state changed successfully
     * @retval IO_UNSUPPORTED   Power management not supported
     * @retval IO_ERROR         Power state change failed
     */
    STDMETHOD_(IO_RETURN, SetPowerState)(THIS_
        BUS_POWER_STATE PowerState
        ) PURE;

    /**
     * @brief Get available bandwidth
     *
     * Retrieves information about bus bandwidth usage and availability.
     *
     * @param puMaxBandwidth    Receives maximum bandwidth (bytes/sec)
     * @param puAvailable       Receives available bandwidth (bytes/sec)
     *
     * @retval IO_SUCCESS       Bandwidth info retrieved successfully
     * @retval IO_UNSUPPORTED   Bandwidth info not available
     */
    STDMETHOD_(IO_RETURN, GetBandwidth)(THIS_
        UINT64 *puMaxBandwidth,
        UINT64 *puAvailable
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOBusDevice - Bus Device Interface
 *
 * This interface represents a device attached to a bus. It inherits from
 * IIOService and adds bus-specific device operations.
 */
#undef INTERFACE
#define INTERFACE IIOBusDevice

DECLARE_INTERFACE_(IIOBusDevice, IIOService)
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

    // IIOBusDevice methods

    /**
     * @brief Get device information
     *
     * Retrieves comprehensive information about the bus device.
     *
     * @param pDeviceInfo       Receives device information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        BUS_DEVICE_INFO *pDeviceInfo
        ) PURE;

    /**
     * @brief Get device location
     *
     * Retrieves the bus-specific location/address of this device.
     *
     * @param pLocation         Receives device location
     *
     * @retval IO_SUCCESS       Location retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetLocation)(THIS_
        BUS_DEVICE_LOCATION *pLocation
        ) PURE;

    /**
     * @brief Get parent bus
     *
     * Retrieves the bus interface that this device is attached to.
     *
     * @param ppBus             Receives bus interface
     *
     * @retval IO_SUCCESS       Bus retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetBus)(THIS_
        IIOBus **ppBus
        ) PURE;

    /**
     * @brief Reset device
     *
     * Performs a device-level reset.
     *
     * @param ResetType         Type of reset to perform
     *
     * @retval IO_SUCCESS       Reset completed successfully
     * @retval IO_UNSUPPORTED   Reset type not supported
     * @retval IO_ERROR         Reset failed
     */
    STDMETHOD_(IO_RETURN, ResetDevice)(THIS_
        BUS_RESET_TYPE ResetType
        ) PURE;

    /**
     * @brief Enable/disable device
     *
     * Enables or disables the device on the bus.
     *
     * @param bEnable           TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS       Device state changed successfully
     * @retval IO_ERROR         State change failed
     */
    STDMETHOD_(IO_RETURN, SetDeviceEnable)(THIS_
        BOOLEAN bEnable
        ) PURE;

    /**
     * @brief Set device power state
     *
     * Changes the power state of the device.
     *
     * @param uPowerState       Desired power state (device-specific)
     *
     * @retval IO_SUCCESS       Power state changed successfully
     * @retval IO_UNSUPPORTED   Power management not supported
     * @retval IO_ERROR         Power state change failed
     */
    STDMETHOD_(IO_RETURN, SetPowerState)(THIS_
        UINT32 uPowerState
        ) PURE;

    /**
     * @brief Eject device
     *
     * Ejects the device from the bus (for removable devices).
     *
     * @retval IO_SUCCESS       Device ejected successfully
     * @retval IO_UNSUPPORTED   Device not removable/ejectable
     * @retval IO_ERROR         Eject failed
     */
    STDMETHOD_(IO_RETURN, Eject)(THIS) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOBus methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOBus_GetBusInfo(p,a)                  (p)->lpVtbl->GetBusInfo(p,a)
#define IIOBus_EnumerateDevices(p)              (p)->lpVtbl->EnumerateDevices(p)
#define IIOBus_GetDeviceCount(p,a)              (p)->lpVtbl->GetDeviceCount(p,a)
#define IIOBus_GetDevice(p,a,b)                 (p)->lpVtbl->GetDevice(p,a,b)
#define IIOBus_GetDeviceByLocation(p,a,b)       (p)->lpVtbl->GetDeviceByLocation(p,a,b)
#define IIOBus_ResetBus(p,a)                    (p)->lpVtbl->ResetBus(p,a)
#define IIOBus_GetTopology(p,a)                 (p)->lpVtbl->GetTopology(p,a)
#define IIOBus_SetHotPlugEnable(p,a)            (p)->lpVtbl->SetHotPlugEnable(p,a)
#define IIOBus_RegisterEventCallback(p,a,b)     (p)->lpVtbl->RegisterEventCallback(p,a,b)
#define IIOBus_UnregisterEventCallback(p,a)     (p)->lpVtbl->UnregisterEventCallback(p,a)
#define IIOBus_SetPowerState(p,a)               (p)->lpVtbl->SetPowerState(p,a)
#define IIOBus_GetBandwidth(p,a,b)              (p)->lpVtbl->GetBandwidth(p,a,b)

#define IIOBusDevice_GetDeviceInfo(p,a)         (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOBusDevice_GetLocation(p,a)           (p)->lpVtbl->GetLocation(p,a)
#define IIOBusDevice_GetBus(p,a)                (p)->lpVtbl->GetBus(p,a)
#define IIOBusDevice_ResetDevice(p,a)           (p)->lpVtbl->ResetDevice(p,a)
#define IIOBusDevice_SetDeviceEnable(p,a)       (p)->lpVtbl->SetDeviceEnable(p,a)
#define IIOBusDevice_SetPowerState(p,a)         (p)->lpVtbl->SetPowerState(p,a)
#define IIOBusDevice_Eject(p)                   (p)->lpVtbl->Eject(p)

#endif

/**
 * @brief Initialize Bus family subsystem
 *
 * Initializes the bus abstraction layer and registers it with IOKit.
 *
 * @retval IO_SUCCESS   Initialization successful
 * @retval IO_ERROR     Initialization failed
 */
IO_RETURN
BusInitialize(
    VOID
    );

/**
 * @brief Shutdown Bus family subsystem
 *
 * Shuts down the bus abstraction layer and releases resources.
 *
 * @retval IO_SUCCESS   Shutdown successful
 */
IO_RETURN
BusShutdown(
    VOID
    );

/**
 * @brief Create a bus instance
 *
 * Creates a bus interface wrapping a bus-specific implementation.
 *
 * @param pBusImplementation    Bus-specific implementation
 * @param BusType               Bus type
 * @param ppBus                 Receives bus interface
 *
 * @retval IO_SUCCESS           Bus created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid argument
 */
IO_RETURN
BusCreate(
    IIOService *pBusImplementation,
    BUS_TYPE BusType,
    IIOBus **ppBus
    );

/**
 * @brief Create a bus device instance
 *
 * Creates a bus device interface for a device on a bus.
 *
 * @param pDeviceImplementation Device-specific implementation
 * @param pBus                  Parent bus
 * @param pLocation             Device location on bus
 * @param ppDevice              Receives bus device interface
 *
 * @retval IO_SUCCESS           Device created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid argument
 */
IO_RETURN
BusDeviceCreate(
    IIOService *pDeviceImplementation,
    IIOBus *pBus,
    CONST BUS_DEVICE_LOCATION *pLocation,
    IIOBusDevice **ppDevice
    );

/**
 * @brief Detect bus type
 *
 * Helper function to detect the bus type from a device or bus implementation.
 *
 * @param pService          Service to examine
 * @param pBusType          Receives detected bus type
 *
 * @retval IO_SUCCESS       Bus type detected successfully
 * @retval IO_NO_MATCH      Bus type could not be determined
 * @retval IO_BAD_ARGUMENT  Invalid argument
 */
IO_RETURN
BusDetectType(
    IIOService *pService,
    BUS_TYPE *pBusType
    );

/**
 * @brief Get bus type name
 *
 * Returns a human-readable string for a bus type.
 *
 * @param BusType           Bus type
 *
 * @return String representation of bus type
 */
CONST CHAR8*
BusGetTypeName(
    BUS_TYPE BusType
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_BUS_H */
