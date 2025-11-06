/**
 * @file thunderbolt.h
 * @brief Thunderbolt Family Interface - Thunderbolt 1/2/3/4/5 and USB4 Support
 *
 * This header defines the Thunderbolt family interface for managing
 * Thunderbolt controllers, devices, and tunneling protocols.
 *
 * Supports:
 * - Thunderbolt 1 (Light Peak): 10 Gbps, Mini DisplayPort
 * - Thunderbolt 2: 20 Gbps, DisplayPort 1.2
 * - Thunderbolt 3: 40 Gbps, USB-C connector, USB 3.1
 * - Thunderbolt 4: 40 Gbps, USB4, PCIe 4.0
 * - Thunderbolt 5: 80 Gbps bidirectional, 120 Gbps asymmetric, USB4 v2
 * - USB4 v1: 40 Gbps, based on TB3 protocol
 * - USB4 v2: 80 Gbps, enhanced tunneling
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_THUNDERBOLT_H
#define IOKIT_THUNDERBOLT_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>
#include <iokit/families/pcie/pcie.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOThunderboltController interface GUID
 * {C5D9E7F3-8A4B-4E6C-9F7D-6E8A5B9C4D3E}
 */
DEFINE_GUID(IID_IIOThunderboltController,
    0xC5D9E7F3, 0x8A4B, 0x4E6C, 0x9F, 0x7D, 0x6E, 0x8A, 0x5B, 0x9C, 0x4D, 0x3E);

/**
 * @brief IIOThunderboltDevice interface GUID
 * {D6E8F4A2-9B5C-4F7D-8E6F-7D9A6C8E5F4A}
 */
DEFINE_GUID(IID_IIOThunderboltDevice,
    0xD6E8F4A2, 0x9B5C, 0x4F7D, 0x8E, 0x6F, 0x7D, 0x9A, 0x6C, 0x8E, 0x5F, 0x4A);

/**
 * @brief Thunderbolt Generation
 */
typedef enum _TB_GENERATION {
    TB_GEN_UNKNOWN      = 0,        /**< Unknown generation */
    TB_GEN_1            = 1,        /**< Thunderbolt 1 (10 Gbps) */
    TB_GEN_2            = 2,        /**< Thunderbolt 2 (20 Gbps) */
    TB_GEN_3            = 3,        /**< Thunderbolt 3 (40 Gbps, USB-C) */
    TB_GEN_4            = 4,        /**< Thunderbolt 4 (40 Gbps, USB4 v1) */
    TB_GEN_5            = 5,        /**< Thunderbolt 5 (80/120 Gbps, USB4 v2) */
    USB4_V1             = 0x41,     /**< USB4 v1 (40 Gbps) */
    USB4_V2             = 0x42,     /**< USB4 v2 (80 Gbps) */
} TB_GENERATION;

/**
 * @brief Thunderbolt Security Levels
 */
typedef enum _TB_SECURITY_LEVEL {
    TB_SECURITY_NONE        = 0,    /**< No security (legacy mode) */
    TB_SECURITY_USER        = 1,    /**< User authorization required */
    TB_SECURITY_SECURE      = 2,    /**< Secure connect (challenge-response) */
    TB_SECURITY_DPONLY      = 3,    /**< DisplayPort only (no PCIe tunneling) */
    TB_SECURITY_USBONLY     = 4,    /**< USB only (TB3/4) */
    TB_SECURITY_NOPCIE      = 5,    /**< No PCIe tunneling */
} TB_SECURITY_LEVEL;

/**
 * @brief Thunderbolt Device Type
 */
typedef enum _TB_DEVICE_TYPE {
    TB_DEVICE_HOST          = 0x01, /**< Host controller */
    TB_DEVICE_DEVICE        = 0x02, /**< Thunderbolt device */
    TB_DEVICE_SWITCH        = 0x03, /**< Thunderbolt switch */
} TB_DEVICE_TYPE;

/**
 * @brief Thunderbolt Connection State
 */
typedef enum _TB_CONNECTION_STATE {
    TB_STATE_DISCONNECTED   = 0,    /**< No device connected */
    TB_STATE_CONNECTING     = 1,    /**< Device connecting */
    TB_STATE_CONNECTED      = 2,    /**< Device connected */
    TB_STATE_AUTHORIZING    = 3,    /**< Waiting for authorization */
    TB_STATE_AUTHORIZED     = 4,    /**< Device authorized */
    TB_STATE_ERROR          = 5,    /**< Connection error */
} TB_CONNECTION_STATE;

/**
 * @brief Thunderbolt Tunnel Type
 */
typedef enum _TB_TUNNEL_TYPE {
    TB_TUNNEL_PCIE          = 0x01, /**< PCIe tunnel */
    TB_TUNNEL_DP            = 0x02, /**< DisplayPort tunnel */
    TB_TUNNEL_USB3          = 0x04, /**< USB 3.x tunnel */
    TB_TUNNEL_USB4          = 0x08, /**< USB4 tunnel (TB4/5, USB4) */
    TB_TUNNEL_P2P           = 0x10, /**< Peer-to-peer tunnel */
    TB_TUNNEL_DMA           = 0x20, /**< Direct memory access tunnel */
} TB_TUNNEL_TYPE;

/**
 * @brief Thunderbolt Controller Capabilities
 */
typedef enum _TB_CAPABILITY {
    TB_CAP_HOTPLUG          = 0x00000001, /**< Hot-plug support */
    TB_CAP_DAISY_CHAIN      = 0x00000002, /**< Daisy-chaining support */
    TB_CAP_POWER_DELIVERY   = 0x00000004, /**< USB-C Power Delivery (TB3/4/5) */
    TB_CAP_DISPLAYPORT      = 0x00000008, /**< DisplayPort tunneling */
    TB_CAP_USB3             = 0x00000010, /**< USB 3.x tunneling (TB3/4/5) */
    TB_CAP_CHARGING         = 0x00000020, /**< Device charging */
    TB_CAP_WAKE             = 0x00000040, /**< Wake-on-Thunderbolt */
    TB_CAP_IOMMU            = 0x00000080, /**< IOMMU/VT-d DMA protection */
    TB_CAP_USB4             = 0x00000100, /**< USB4 protocol support */
    TB_CAP_ASYMMETRIC       = 0x00000200, /**< Asymmetric bandwidth (TB5: 120/40) */
    TB_CAP_DP21             = 0x00000400, /**< DisplayPort 2.1 support (TB5) */
    TB_CAP_PAM3             = 0x00000800, /**< PAM-3 signaling (TB5) */
    TB_CAP_PCIE_GEN4        = 0x00001000, /**< PCIe Gen 4 tunneling (TB4/5) */
    TB_CAP_PCIE_GEN5        = 0x00002000, /**< PCIe Gen 5 tunneling (future) */
    TB_CAP_USB4_V2          = 0x00004000, /**< USB4 v2 support (TB5) */
    TB_CAP_BANDWIDTH_MGMT   = 0x00008000, /**< Advanced bandwidth management */
    TB_CAP_SRIOV            = 0x00010000, /**< SR-IOV (Single Root I/O Virtualization) */
    TB_CAP_ATS              = 0x00020000, /**< Address Translation Services */
    TB_CAP_PRI              = 0x00040000, /**< Page Request Interface */
    TB_CAP_PASID            = 0x00080000, /**< Process Address Space ID */
} TB_CAPABILITY;

/**
 * @brief Thunderbolt Bandwidth Mode (TB5)
 */
typedef enum _TB_BANDWIDTH_MODE {
    TB_BANDWIDTH_SYMMETRIC      = 0,    /**< Symmetric bandwidth (e.g., 80/80 Gbps) */
    TB_BANDWIDTH_ASYMMETRIC     = 1,    /**< Asymmetric bandwidth (e.g., 120/40 Gbps) */
} TB_BANDWIDTH_MODE;

/**
 * @brief Thunderbolt Controller Information
 */
typedef struct _TB_CONTROLLER_INFO {
    UINT16          VendorID;           /**< Vendor ID */
    UINT16          DeviceID;           /**< Device ID */
    TB_GENERATION   Generation;         /**< Thunderbolt generation */
    UINT32          NVMVersion;         /**< NVM (firmware) version */
    UINT8           MaxDepth;           /**< Maximum daisy-chain depth */
    UINT8           MaxPortCount;       /**< Maximum number of ports */
    UINT32          Capabilities;       /**< Controller capabilities */
    TB_SECURITY_LEVEL SecurityLevel;    /**< Current security level */
    CHAR8           ControllerName[64]; /**< Controller name */
    UINT32          MaxBandwidthTX;     /**< Max TX bandwidth (Gbps) */
    UINT32          MaxBandwidthRX;     /**< Max RX bandwidth (Gbps) */
    TB_BANDWIDTH_MODE BandwidthMode;    /**< Bandwidth mode (TB5) */
} TB_CONTROLLER_INFO;

/**
 * @brief Thunderbolt Device Information
 */
typedef struct _TB_DEVICE_INFO {
    UINT64          UniqueID;           /**< Unique device ID */
    UINT16          VendorID;           /**< Vendor ID */
    UINT16          DeviceID;           /**< Device ID */
    CHAR8           DeviceName[64];     /**< Device name */
    TB_DEVICE_TYPE  DeviceType;         /**< Device type */
    UINT8           Depth;              /**< Daisy-chain depth (0 = host) */
    UINT8           Route;              /**< Route string */
    TB_GENERATION   Generation;         /**< Supported Thunderbolt generation */
    UINT32          Capabilities;       /**< Device capabilities */
    BOOLEAN         bAuthorized;        /**< Device authorized */
    BOOLEAN         bKeyPresent;        /**< Encryption key present */
} TB_DEVICE_INFO;

/**
 * @brief Thunderbolt Tunnel Information
 */
typedef struct _TB_TUNNEL_INFO {
    TB_TUNNEL_TYPE  Type;               /**< Tunnel type */
    UINT32          Bandwidth;          /**< Allocated bandwidth (Mbps) */
    UINT8           TunnelID;           /**< Tunnel ID */
    BOOLEAN         bActive;            /**< Tunnel active */
} TB_TUNNEL_INFO;

/**
 * @brief Thunderbolt Registers (NHI - Native Host Interface)
 */
#define TB_REG_RING_CTRL            0x0000  /**< Ring control register */
#define TB_REG_RING_DB              0x0004  /**< Ring doorbell register */
#define TB_REG_RING_SIZE            0x0008  /**< Ring size register */
#define TB_REG_TX_RING_BASE         0x000C  /**< TX ring base address */
#define TB_REG_RX_RING_BASE         0x001C  /**< RX ring base address */
#define TB_REG_CFG                  0x0100  /**< Configuration register */
#define TB_REG_VERSION              0x0104  /**< Version register */
#define TB_REG_SECURITY             0x0108  /**< Security level register */
#define TB_REG_INTR_MASK            0x010C  /**< Interrupt mask register */
#define TB_REG_INTR_STATUS          0x0110  /**< Interrupt status register */

/**
 * @brief Thunderbolt Configuration Space Offsets
 */
#define TB_CFG_DROM                 0x00    /**< Device ROM (DROM) */
#define TB_CFG_SWITCH               0x01    /**< Switch configuration */
#define TB_CFG_PORT                 0x02    /**< Port configuration */
#define TB_CFG_PATH                 0x03    /**< Path configuration */
#define TB_CFG_COUNTERS             0x04    /**< Performance counters */

/**
 * @brief IIOThunderboltController - Thunderbolt Controller Interface
 *
 * This interface represents a Thunderbolt host controller and provides
 * methods for device enumeration, authorization, and tunnel management.
 */
#undef INTERFACE
#define INTERFACE IIOThunderboltController

DECLARE_INTERFACE_(IIOThunderboltController, IIOService)
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

    // IIOThunderboltController methods

    /**
     * @brief Get controller information
     *
     * Retrieves detailed information about the Thunderbolt controller.
     *
     * @param pControllerInfo   Receives controller information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetControllerInfo)(THIS_
        TB_CONTROLLER_INFO *pControllerInfo
        ) PURE;

    /**
     * @brief Enumerate connected devices
     *
     * Scans the Thunderbolt domain and enumerates all connected devices.
     *
     * @param ppDevices         Receives array of device interfaces
     * @param puDeviceCount     On input: max devices; On output: actual count
     *
     * @retval IO_SUCCESS       Enumeration successful
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, EnumerateDevices)(THIS_
        IIOThunderboltDevice **ppDevices,
        UINT32 *puDeviceCount
        ) PURE;

    /**
     * @brief Set security level
     *
     * Changes the Thunderbolt security level.
     *
     * @param SecurityLevel     New security level
     *
     * @retval IO_SUCCESS       Security level set
     * @retval IO_NOT_PERMITTED Insufficient privileges
     */
    STDMETHOD_(IO_RETURN, SetSecurityLevel)(THIS_
        TB_SECURITY_LEVEL SecurityLevel
        ) PURE;

    /**
     * @brief Authorize device
     *
     * Authorizes a Thunderbolt device for operation.
     *
     * @param pDevice           Device to authorize
     * @param bPersistent       TRUE for persistent authorization
     *
     * @retval IO_SUCCESS       Device authorized
     * @retval IO_ERROR         Authorization failed
     */
    STDMETHOD_(IO_RETURN, AuthorizeDevice)(THIS_
        IIOThunderboltDevice *pDevice,
        BOOLEAN bPersistent
        ) PURE;

    /**
     * @brief Deauthorize device
     *
     * Deauthorizes and disconnects a Thunderbolt device.
     *
     * @param pDevice           Device to deauthorize
     *
     * @retval IO_SUCCESS       Device deauthorized
     */
    STDMETHOD_(IO_RETURN, DeauthorizeDevice)(THIS_
        IIOThunderboltDevice *pDevice
        ) PURE;

    /**
     * @brief Create tunnel
     *
     * Creates a tunnel for the specified protocol.
     *
     * @param pDevice           Destination device
     * @param TunnelType        Type of tunnel to create
     * @param uBandwidth        Requested bandwidth (Mbps)
     * @param puTunnelID        Receives tunnel ID
     *
     * @retval IO_SUCCESS       Tunnel created
     * @retval IO_NO_BANDWIDTH  Insufficient bandwidth
     */
    STDMETHOD_(IO_RETURN, CreateTunnel)(THIS_
        IIOThunderboltDevice *pDevice,
        TB_TUNNEL_TYPE TunnelType,
        UINT32 uBandwidth,
        UINT8 *puTunnelID
        ) PURE;

    /**
     * @brief Destroy tunnel
     *
     * Destroys an existing tunnel.
     *
     * @param uTunnelID         Tunnel ID to destroy
     *
     * @retval IO_SUCCESS       Tunnel destroyed
     */
    STDMETHOD_(IO_RETURN, DestroyTunnel)(THIS_
        UINT8 uTunnelID
        ) PURE;

    /**
     * @brief Update firmware
     *
     * Updates the controller firmware from a file.
     *
     * @param pFirmwareData     Firmware image data
     * @param cbSize            Size of firmware image
     *
     * @retval IO_SUCCESS       Firmware updated
     * @retval IO_BAD_MEDIA     Invalid firmware image
     */
    STDMETHOD_(IO_RETURN, UpdateFirmware)(THIS_
        CONST VOID *pFirmwareData,
        UINTN cbSize
        ) PURE;

    /**
     * @brief Set bandwidth mode (TB5/USB4 v2)
     *
     * Switches between symmetric and asymmetric bandwidth modes.
     * TB5: Symmetric (80/80 Gbps) or Asymmetric (120/40 Gbps)
     * USB4 v2: Symmetric (80/80 Gbps) or Asymmetric configurations
     *
     * @param BandwidthMode     Bandwidth mode to set
     *
     * @retval IO_SUCCESS       Bandwidth mode set
     * @retval IO_UNSUPPORTED   Controller doesn't support this feature
     * @retval IO_BUSY          Cannot change while tunnels are active
     */
    STDMETHOD_(IO_RETURN, SetBandwidthMode)(THIS_
        TB_BANDWIDTH_MODE BandwidthMode
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOThunderboltDevice - Thunderbolt Device Interface
 *
 * This interface represents a Thunderbolt device connected to the
 * controller.
 */
#undef INTERFACE
#define INTERFACE IIOThunderboltDevice

DECLARE_INTERFACE_(IIOThunderboltDevice, IIOService)
{
    // IUnknown and IIOService methods inherited...

    /**
     * @brief Get device information
     *
     * Retrieves detailed information about the Thunderbolt device.
     *
     * @param pDeviceInfo       Receives device information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        TB_DEVICE_INFO *pDeviceInfo
        ) PURE;

    /**
     * @brief Get connection state
     *
     * Returns the current connection state of the device.
     *
     * @param pState            Receives connection state
     *
     * @retval IO_SUCCESS       State retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetConnectionState)(THIS_
        TB_CONNECTION_STATE *pState
        ) PURE;

    /**
     * @brief Read DROM (Device ROM)
     *
     * Reads the device ROM which contains device identification and
     * capability information.
     *
     * @param pBuffer           Buffer to receive DROM data
     * @param cbSize            Size of buffer
     *
     * @retval IO_SUCCESS       DROM read successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, ReadDROM)(THIS_
        VOID *pBuffer,
        UINTN cbSize
        ) PURE;

    /**
     * @brief Get active tunnels
     *
     * Retrieves information about active tunnels to this device.
     *
     * @param pTunnels          Receives array of tunnel information
     * @param puTunnelCount     On input: max tunnels; On output: actual count
     *
     * @retval IO_SUCCESS       Tunnels retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetActiveTunnels)(THIS_
        TB_TUNNEL_INFO *pTunnels,
        UINT32 *puTunnelCount
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOThunderboltController methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOThunderboltController_GetControllerInfo(p,a)     (p)->lpVtbl->GetControllerInfo(p,a)
#define IIOThunderboltController_EnumerateDevices(p,a,b)    (p)->lpVtbl->EnumerateDevices(p,a,b)
#define IIOThunderboltController_SetSecurityLevel(p,a)      (p)->lpVtbl->SetSecurityLevel(p,a)
#define IIOThunderboltController_AuthorizeDevice(p,a,b)     (p)->lpVtbl->AuthorizeDevice(p,a,b)
#define IIOThunderboltController_DeauthorizeDevice(p,a)     (p)->lpVtbl->DeauthorizeDevice(p,a)
#define IIOThunderboltController_CreateTunnel(p,a,b,c,d)    (p)->lpVtbl->CreateTunnel(p,a,b,c,d)
#define IIOThunderboltController_DestroyTunnel(p,a)         (p)->lpVtbl->DestroyTunnel(p,a)
#define IIOThunderboltController_UpdateFirmware(p,a,b)      (p)->lpVtbl->UpdateFirmware(p,a,b)

#define IIOThunderboltDevice_GetDeviceInfo(p,a)             (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOThunderboltDevice_GetConnectionState(p,a)        (p)->lpVtbl->GetConnectionState(p,a)
#define IIOThunderboltDevice_ReadDROM(p,a,b)                (p)->lpVtbl->ReadDROM(p,a,b)
#define IIOThunderboltDevice_GetActiveTunnels(p,a,b)        (p)->lpVtbl->GetActiveTunnels(p,a,b)

#endif

/**
 * @brief Initialize Thunderbolt subsystem
 *
 * @retval IO_SUCCESS   Subsystem initialized successfully
 */
IO_RETURN
ThunderboltInitialize(
    VOID
    );

/**
 * @brief Detect and create Thunderbolt controller instances
 *
 * @param ppControllers     Receives array of controller interfaces
 * @param puControllerCount On input: max controllers; On output: actual count
 *
 * @retval IO_SUCCESS   Detection successful
 */
IO_RETURN
ThunderboltDetectControllers(
    IIOThunderboltController **ppControllers,
    UINT32 *puControllerCount
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_THUNDERBOLT_H */
