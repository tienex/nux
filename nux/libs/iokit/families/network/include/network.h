/**
 * @file network.h
 * @brief Network Family Interface - Unified Network Device Abstraction
 *
 * This header defines the Network family interface providing a unified abstraction
 * layer for all network devices regardless of underlying bus (PCIe, USB, ISA, etc.)
 * and network type (Ethernet, WiFi, Bluetooth, Cellular).
 *
 * The Network family sits ABOVE bus-specific drivers and provides:
 * - Protocol-agnostic packet transmission and reception
 * - Unified device enumeration and capabilities reporting
 * - Common error handling and statistics collection
 * - Network controller and interface abstraction
 * - Support for advanced features (offloading, VLAN, multicast, WoL)
 *
 * This is one of the most critical device families in IOKit, supporting:
 * - Ethernet (10M/100M/1G/2.5G/5G/10G/25G/40G/100G/200G/400G)
 * - WiFi (802.11a/b/g/n/ac/ax/be - WiFi 1 through WiFi 7)
 * - Bluetooth (1.0 through 5.3)
 * - Cellular (2G/3G/4G LTE/5G)
 * - WAN (T1/E1, DSL, Cable modem, WWAN)
 * - InfiniBand, Fibre Channel
 * - Legacy technologies (Token Ring, FDDI, ATM)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_NETWORK_H
#define IOKIT_NETWORK_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIONetworkController interface GUID
 * {C1D2E3F4-A5B6-4C7D-9E8F-0A1B2C3D4E5F}
 */
DEFINE_GUID(IID_IIONetworkController,
    0xC1D2E3F4, 0xA5B6, 0x4C7D, 0x9E, 0x8F, 0x0A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F);

/**
 * @brief IIONetworkInterface interface GUID
 * {D2E3F4A5-B6C7-4D8E-AF9B-1C2D3E4F5A6B}
 */
DEFINE_GUID(IID_IIONetworkInterface,
    0xD2E3F4A5, 0xB6C7, 0x4D8E, 0xAF, 0x9B, 0x1C, 0x2D, 0x3E, 0x4F, 0x5A, 0x6B);

/**
 * @brief IIONetworkPacket interface GUID
 * {E3F4A5B6-C7D8-4E9F-BA0C-2D3E4F5A6B7C}
 */
DEFINE_GUID(IID_IIONetworkPacket,
    0xE3F4A5B6, 0xC7D8, 0x4E9F, 0xBA, 0x0C, 0x2D, 0x3E, 0x4F, 0x5A, 0x6B, 0x7C);

/**
 * @brief Network Device Types
 *
 * Comprehensive list of network device types supported by the framework.
 */
typedef enum _NETWORK_DEVICE_TYPE {
    NETWORK_TYPE_UNKNOWN        = 0x00,     /**< Unknown device type */
    NETWORK_TYPE_ETHERNET       = 0x01,     /**< Ethernet (wired) */
    NETWORK_TYPE_WIFI           = 0x02,     /**< WiFi (802.11) */
    NETWORK_TYPE_BLUETOOTH      = 0x03,     /**< Bluetooth */
    NETWORK_TYPE_CELLULAR       = 0x04,     /**< Cellular (2G/3G/4G/5G) */
    NETWORK_TYPE_INFINIBAND     = 0x05,     /**< InfiniBand */
    NETWORK_TYPE_FIBRE_CHANNEL  = 0x06,     /**< Fibre Channel */
    NETWORK_TYPE_WAN            = 0x07,     /**< WAN (T1/E1/DSL/Cable) */
    NETWORK_TYPE_WWAN           = 0x08,     /**< Wireless WAN */
    NETWORK_TYPE_TOKEN_RING     = 0x09,     /**< Token Ring (legacy) */
    NETWORK_TYPE_FDDI           = 0x0A,     /**< FDDI (legacy) */
    NETWORK_TYPE_ATM            = 0x0B,     /**< ATM (legacy) */
    NETWORK_TYPE_LOOPBACK       = 0x0C,     /**< Loopback interface */
    NETWORK_TYPE_TUNNEL         = 0x0D,     /**< Tunnel interface (VPN, etc.) */
    NETWORK_TYPE_BRIDGE         = 0x0E,     /**< Bridge interface */
    NETWORK_TYPE_VIRTUAL        = 0x0F,     /**< Virtual network interface */
} NETWORK_DEVICE_TYPE;

/**
 * @brief Network Media Types
 *
 * Specifies the physical media and link speed/mode.
 */
typedef enum _NETWORK_MEDIA_TYPE {
    // Ethernet Media Types
    MEDIA_ETHERNET_AUTO         = 0x0000,   /**< Auto-negotiation */
    MEDIA_ETHERNET_10BASE_T     = 0x0001,   /**< 10 Mbps twisted pair */
    MEDIA_ETHERNET_10BASE_2     = 0x0002,   /**< 10 Mbps thin coax */
    MEDIA_ETHERNET_10BASE_5     = 0x0003,   /**< 10 Mbps thick coax */
    MEDIA_ETHERNET_100BASE_TX   = 0x0004,   /**< 100 Mbps twisted pair */
    MEDIA_ETHERNET_100BASE_FX   = 0x0005,   /**< 100 Mbps fiber */
    MEDIA_ETHERNET_1000BASE_T   = 0x0006,   /**< 1 Gbps twisted pair */
    MEDIA_ETHERNET_1000BASE_SX  = 0x0007,   /**< 1 Gbps short-range fiber */
    MEDIA_ETHERNET_1000BASE_LX  = 0x0008,   /**< 1 Gbps long-range fiber */
    MEDIA_ETHERNET_1000BASE_CX  = 0x0009,   /**< 1 Gbps copper twinax */
    MEDIA_ETHERNET_2500BASE_T   = 0x000A,   /**< 2.5 Gbps twisted pair */
    MEDIA_ETHERNET_5GBASE_T     = 0x000B,   /**< 5 Gbps twisted pair */
    MEDIA_ETHERNET_10GBASE_T    = 0x000C,   /**< 10 Gbps twisted pair */
    MEDIA_ETHERNET_10GBASE_SR   = 0x000D,   /**< 10 Gbps short-range fiber */
    MEDIA_ETHERNET_10GBASE_LR   = 0x000E,   /**< 10 Gbps long-range fiber */
    MEDIA_ETHERNET_10GBASE_ER   = 0x000F,   /**< 10 Gbps extended-range fiber */
    MEDIA_ETHERNET_25GBASE_SR   = 0x0010,   /**< 25 Gbps short-range fiber */
    MEDIA_ETHERNET_40GBASE_SR4  = 0x0011,   /**< 40 Gbps short-range fiber */
    MEDIA_ETHERNET_40GBASE_LR4  = 0x0012,   /**< 40 Gbps long-range fiber */
    MEDIA_ETHERNET_100GBASE_SR4 = 0x0013,   /**< 100 Gbps short-range fiber */
    MEDIA_ETHERNET_100GBASE_LR4 = 0x0014,   /**< 100 Gbps long-range fiber */
    MEDIA_ETHERNET_200GBASE_SR4 = 0x0015,   /**< 200 Gbps short-range fiber */
    MEDIA_ETHERNET_400GBASE_SR8 = 0x0016,   /**< 400 Gbps short-range fiber */

    // WiFi Media Types (frequency bands)
    MEDIA_WIFI_2_4GHZ           = 0x0100,   /**< 2.4 GHz band */
    MEDIA_WIFI_5GHZ             = 0x0101,   /**< 5 GHz band */
    MEDIA_WIFI_6GHZ             = 0x0102,   /**< 6 GHz band (WiFi 6E/7) */
    MEDIA_WIFI_60GHZ            = 0x0103,   /**< 60 GHz band (802.11ad/ay) */

    // Fiber Types
    MEDIA_FIBER_SINGLE_MODE     = 0x0200,   /**< Single-mode fiber */
    MEDIA_FIBER_MULTI_MODE      = 0x0201,   /**< Multi-mode fiber */

    // Cellular Bands (simplified)
    MEDIA_CELLULAR_GSM          = 0x0300,   /**< GSM bands */
    MEDIA_CELLULAR_UMTS         = 0x0301,   /**< UMTS/3G bands */
    MEDIA_CELLULAR_LTE          = 0x0302,   /**< LTE/4G bands */
    MEDIA_CELLULAR_5G_SUB6      = 0x0303,   /**< 5G Sub-6 GHz */
    MEDIA_CELLULAR_5G_MMWAVE    = 0x0304,   /**< 5G mmWave */
} NETWORK_MEDIA_TYPE;

/**
 * @brief Network Link Speeds
 */
typedef enum _NETWORK_SPEED {
    SPEED_UNKNOWN               = 0,        /**< Unknown speed */
    SPEED_10MBPS                = 10,       /**< 10 Megabits/sec */
    SPEED_100MBPS               = 100,      /**< 100 Megabits/sec */
    SPEED_1GBPS                 = 1000,     /**< 1 Gigabit/sec */
    SPEED_2_5GBPS               = 2500,     /**< 2.5 Gigabits/sec */
    SPEED_5GBPS                 = 5000,     /**< 5 Gigabits/sec */
    SPEED_10GBPS                = 10000,    /**< 10 Gigabits/sec */
    SPEED_25GBPS                = 25000,    /**< 25 Gigabits/sec */
    SPEED_40GBPS                = 40000,    /**< 40 Gigabits/sec */
    SPEED_100GBPS               = 100000,   /**< 100 Gigabits/sec */
    SPEED_200GBPS               = 200000,   /**< 200 Gigabits/sec */
    SPEED_400GBPS               = 400000,   /**< 400 Gigabits/sec */
} NETWORK_SPEED;

/**
 * @brief Network Duplex Modes
 */
typedef enum _NETWORK_DUPLEX {
    DUPLEX_UNKNOWN              = 0x00,     /**< Unknown duplex mode */
    DUPLEX_HALF                 = 0x01,     /**< Half duplex */
    DUPLEX_FULL                 = 0x02,     /**< Full duplex */
    DUPLEX_AUTO                 = 0x03,     /**< Auto-negotiation */
} NETWORK_DUPLEX;

/**
 * @brief WiFi Standards (802.11)
 */
typedef enum _WIFI_STANDARD {
    WIFI_STANDARD_NONE          = 0x00,     /**< No WiFi / Not applicable */
    WIFI_STANDARD_80211A        = 0x01,     /**< WiFi 1 - 802.11a (5 GHz) */
    WIFI_STANDARD_80211B        = 0x02,     /**< WiFi 2 - 802.11b (2.4 GHz) */
    WIFI_STANDARD_80211G        = 0x03,     /**< WiFi 3 - 802.11g (2.4 GHz) */
    WIFI_STANDARD_80211N        = 0x04,     /**< WiFi 4 - 802.11n (HT) */
    WIFI_STANDARD_80211AC       = 0x05,     /**< WiFi 5 - 802.11ac (VHT) */
    WIFI_STANDARD_80211AX       = 0x06,     /**< WiFi 6/6E - 802.11ax (HE) */
    WIFI_STANDARD_80211BE       = 0x07,     /**< WiFi 7 - 802.11be (EHT) */
    WIFI_STANDARD_80211AD       = 0x08,     /**< 802.11ad (60 GHz) */
    WIFI_STANDARD_80211AY       = 0x09,     /**< 802.11ay (60 GHz enhanced) */
} WIFI_STANDARD;

/**
 * @brief WiFi Channel Widths
 */
typedef enum _WIFI_CHANNEL_WIDTH {
    WIFI_WIDTH_20MHZ            = 20,       /**< 20 MHz channel */
    WIFI_WIDTH_40MHZ            = 40,       /**< 40 MHz channel */
    WIFI_WIDTH_80MHZ            = 80,       /**< 80 MHz channel */
    WIFI_WIDTH_160MHZ           = 160,      /**< 160 MHz channel */
    WIFI_WIDTH_320MHZ           = 320,      /**< 320 MHz channel (WiFi 7) */
} WIFI_CHANNEL_WIDTH;

/**
 * @brief WiFi Security Modes
 */
typedef enum _WIFI_SECURITY {
    WIFI_SECURITY_NONE          = 0x00,     /**< Open network */
    WIFI_SECURITY_WEP           = 0x01,     /**< WEP (deprecated) */
    WIFI_SECURITY_WPA           = 0x02,     /**< WPA */
    WIFI_SECURITY_WPA2          = 0x03,     /**< WPA2 */
    WIFI_SECURITY_WPA3           = 0x04,     /**< WPA3 */
    WIFI_SECURITY_WPA2_WPA3     = 0x05,     /**< WPA2/WPA3 mixed */
} WIFI_SECURITY;

/**
 * @brief WiFi MIMO Configuration
 */
typedef enum _WIFI_MIMO {
    WIFI_MIMO_1X1               = 0x11,     /**< 1x1 MIMO (1 stream) */
    WIFI_MIMO_2X2               = 0x22,     /**< 2x2 MIMO (2 streams) */
    WIFI_MIMO_3X3               = 0x33,     /**< 3x3 MIMO (3 streams) */
    WIFI_MIMO_4X4               = 0x44,     /**< 4x4 MIMO (4 streams) */
    WIFI_MIMO_8X8               = 0x88,     /**< 8x8 MIMO (8 streams) */
} WIFI_MIMO;

/**
 * @brief Bluetooth Versions
 */
typedef enum _BLUETOOTH_VERSION {
    BT_VERSION_1_0              = 0x10,     /**< Bluetooth 1.0 */
    BT_VERSION_1_1              = 0x11,     /**< Bluetooth 1.1 */
    BT_VERSION_1_2              = 0x12,     /**< Bluetooth 1.2 */
    BT_VERSION_2_0              = 0x20,     /**< Bluetooth 2.0 + EDR */
    BT_VERSION_2_1              = 0x21,     /**< Bluetooth 2.1 + EDR */
    BT_VERSION_3_0              = 0x30,     /**< Bluetooth 3.0 + HS */
    BT_VERSION_4_0              = 0x40,     /**< Bluetooth 4.0 (LE) */
    BT_VERSION_4_1              = 0x41,     /**< Bluetooth 4.1 */
    BT_VERSION_4_2              = 0x42,     /**< Bluetooth 4.2 */
    BT_VERSION_5_0              = 0x50,     /**< Bluetooth 5.0 */
    BT_VERSION_5_1              = 0x51,     /**< Bluetooth 5.1 */
    BT_VERSION_5_2              = 0x52,     /**< Bluetooth 5.2 */
    BT_VERSION_5_3              = 0x53,     /**< Bluetooth 5.3 */
} BLUETOOTH_VERSION;

/**
 * @brief Network Capability Flags (Bitfield)
 *
 * Flags indicating which advanced features the network device supports.
 */
#define NETWORK_CAP_CHECKSUM_IPV4       0x00000001  /**< IPv4 checksum offload */
#define NETWORK_CAP_CHECKSUM_IPV6       0x00000002  /**< IPv6 checksum offload */
#define NETWORK_CAP_CHECKSUM_TCP        0x00000004  /**< TCP checksum offload */
#define NETWORK_CAP_CHECKSUM_UDP        0x00000008  /**< UDP checksum offload */
#define NETWORK_CAP_TSO                 0x00000010  /**< TCP Segmentation Offload */
#define NETWORK_CAP_LRO                 0x00000020  /**< Large Receive Offload */
#define NETWORK_CAP_RSS                 0x00000040  /**< Receive Side Scaling */
#define NETWORK_CAP_VLAN_TAG            0x00000080  /**< VLAN tagging */
#define NETWORK_CAP_VLAN_FILTER         0x00000100  /**< VLAN filtering */
#define NETWORK_CAP_JUMBO_FRAMES        0x00000200  /**< Jumbo frames support */
#define NETWORK_CAP_WAKE_ON_LAN         0x00000400  /**< Wake-on-LAN */
#define NETWORK_CAP_SRIOV               0x00000800  /**< SR-IOV support */
#define NETWORK_CAP_RDMA                0x00001000  /**< RDMA support */
#define NETWORK_CAP_FLOW_CONTROL        0x00002000  /**< 802.3x flow control */
#define NETWORK_CAP_PFC                 0x00004000  /**< Priority Flow Control */
#define NETWORK_CAP_LACP                0x00008000  /**< Link Aggregation (LACP) */
#define NETWORK_CAP_MULTICAST_FILTER    0x00010000  /**< Multicast filtering */
#define NETWORK_CAP_PROMISCUOUS         0x00020000  /**< Promiscuous mode */
#define NETWORK_CAP_MONITOR_MODE        0x00040000  /**< Monitor mode (WiFi) */
#define NETWORK_CAP_AUTO_NEGOTIATE      0x00080000  /**< Auto-negotiation */
#define NETWORK_CAP_FULL_DUPLEX         0x00100000  /**< Full duplex capable */
#define NETWORK_CAP_HALF_DUPLEX         0x00200000  /**< Half duplex capable */
#define NETWORK_CAP_SCATTER_GATHER      0x00400000  /**< Scatter-gather DMA */
#define NETWORK_CAP_INTERRUPT_COALESCE  0x00800000  /**< Interrupt coalescing */
#define NETWORK_CAP_VXLAN               0x01000000  /**< VXLAN offload */
#define NETWORK_CAP_GENEVE              0x02000000  /**< GENEVE offload */
#define NETWORK_CAP_IPSEC_OFFLOAD       0x04000000  /**< IPSec offload */
#define NETWORK_CAP_MACSEC              0x08000000  /**< MACsec support */
#define NETWORK_CAP_PTP                 0x10000000  /**< Precision Time Protocol */

/**
 * @brief Link State Flags
 */
#define LINK_STATE_DOWN                 0x00000000  /**< Link is down */
#define LINK_STATE_UP                   0x00000001  /**< Link is up */
#define LINK_STATE_RUNNING              0x00000002  /**< Interface is running */
#define LINK_STATE_CARRIER              0x00000004  /**< Carrier detected */
#define LINK_STATE_DORMANT              0x00000008  /**< Link is dormant */

/**
 * @brief Packet Protocol Types (EtherType)
 */
typedef enum _NETWORK_PROTOCOL {
    PROTOCOL_IPV4               = 0x0800,   /**< IPv4 */
    PROTOCOL_ARP                = 0x0806,   /**< ARP */
    PROTOCOL_IPV6               = 0x86DD,   /**< IPv6 */
    PROTOCOL_VLAN               = 0x8100,   /**< 802.1Q VLAN */
    PROTOCOL_LLDP               = 0x88CC,   /**< LLDP */
    PROTOCOL_UNKNOWN            = 0xFFFF,   /**< Unknown protocol */
} NETWORK_PROTOCOL;

/**
 * @brief Maximum MAC Address Length
 */
#define MAX_MAC_ADDRESS_LENGTH          8

/**
 * @brief Standard Ethernet MAC Address Length
 */
#define ETHERNET_MAC_ADDRESS_LENGTH     6

/**
 * @brief Maximum MTU Size
 */
#define MAX_MTU_SIZE                    65535

/**
 * @brief Standard Ethernet MTU
 */
#define ETHERNET_MTU                    1500

/**
 * @brief Jumbo Frame MTU
 */
#define JUMBO_MTU                       9000

/**
 * @brief Network Controller Information
 *
 * Comprehensive information structure describing a network controller's
 * characteristics, capabilities, and current state.
 */
typedef struct _NETWORK_CONTROLLER_INFO {
    // Device Type and Media
    NETWORK_DEVICE_TYPE DeviceType;         /**< Network device type */
    NETWORK_MEDIA_TYPE  MediaType;          /**< Media type */
    NETWORK_SPEED       Speed;              /**< Link speed (Mbps) */
    NETWORK_DUPLEX      Duplex;             /**< Duplex mode */

    // MAC Address
    UINT8               MACAddress[MAX_MAC_ADDRESS_LENGTH]; /**< MAC address */
    UINT8               MACAddressLength;   /**< Length of MAC address (6 or 8) */

    // Capabilities
    UINT32              Capabilities;       /**< Capability flags (NETWORK_CAP_*) */
    UINT32              MTU;                /**< Maximum Transmission Unit */
    UINT32              MaxMTU;             /**< Maximum supported MTU */
    UINT32              MinMTU;             /**< Minimum supported MTU */

    // Driver Information
    CHAR8               DriverName[64];     /**< Driver name */
    CHAR8               DriverVersion[16];  /**< Driver version */

    // Hardware Information
    CHAR8               Vendor[40];         /**< Vendor/manufacturer */
    CHAR8               Model[40];          /**< Model/chipset name */
    CHAR8               FirmwareVersion[16];/**< Firmware version */

    // Bus Attachment
    UINT16              VendorID;           /**< PCI Vendor ID (if applicable) */
    UINT16              DeviceID;           /**< PCI Device ID (if applicable) */
    UINT16              SubsystemVendorID;  /**< Subsystem Vendor ID */
    UINT16              SubsystemDeviceID;  /**< Subsystem Device ID */
    UINT8               BusNumber;          /**< Bus number */
    UINT8               DeviceNumber;       /**< Device number */
    UINT8               FunctionNumber;     /**< Function number */

    // Queue Information
    UINT32              NumTXQueues;        /**< Number of TX queues */
    UINT32              NumRXQueues;        /**< Number of RX queues */
    UINT32              TXQueueDepth;       /**< TX queue depth */
    UINT32              RXQueueDepth;       /**< RX queue depth */

    // WiFi-Specific Information
    WIFI_STANDARD       WiFiStandard;       /**< WiFi standard (if WiFi) */
    WIFI_CHANNEL_WIDTH  WiFiChannelWidth;   /**< WiFi channel width */
    WIFI_MIMO           WiFiMIMO;           /**< MIMO configuration */

    // Bluetooth-Specific Information
    BLUETOOTH_VERSION   BTVersion;          /**< Bluetooth version */

    // Link State
    UINT32              LinkState;          /**< Link state flags */
} NETWORK_CONTROLLER_INFO;

/**
 * @brief Network Interface Information
 */
typedef struct _NETWORK_INTERFACE_INFO {
    CHAR8               InterfaceName[32];  /**< Interface name (eth0, wlan0) */
    UINT8               MACAddress[MAX_MAC_ADDRESS_LENGTH]; /**< MAC address */
    UINT8               MACAddressLength;   /**< Length of MAC address */

    // IPv4 Information
    UINT32              IPv4Address;        /**< IPv4 address (network byte order) */
    UINT32              IPv4Netmask;        /**< IPv4 netmask */
    UINT32              IPv4Gateway;        /**< IPv4 gateway */

    // IPv6 Information
    UINT8               IPv6Address[16];    /**< IPv6 address */
    UINT8               IPv6PrefixLength;   /**< IPv6 prefix length */

    // Link State
    UINT32              LinkState;          /**< Link state flags */
    NETWORK_SPEED       Speed;              /**< Current link speed */
    NETWORK_DUPLEX      Duplex;             /**< Current duplex mode */
    UINT32              MTU;                /**< Current MTU */

    // Flags
    BOOLEAN             bUp;                /**< Interface is up */
    BOOLEAN             bRunning;           /**< Interface is running */
    BOOLEAN             bPromiscuous;       /**< Promiscuous mode enabled */
} NETWORK_INTERFACE_INFO;

/**
 * @brief Network Packet Structure
 *
 * Represents a network packet for transmission or reception.
 */
typedef struct _NETWORK_PACKET {
    // Buffer Information
    VOID               *pBuffer;            /**< Packet buffer pointer */
    UINT32              BufferLength;       /**< Total buffer length */
    UINT32              DataLength;         /**< Actual data length */

    // Scatter-Gather Support
    VOID              **ppSGList;           /**< Scatter-gather list (if supported) */
    UINT32             *pSGLengths;         /**< Length of each SG segment */
    UINT32              NumSGEntries;       /**< Number of SG entries */

    // Metadata
    UINT16              VLANTag;            /**< VLAN tag (0 = none) */
    NETWORK_PROTOCOL    Protocol;           /**< Protocol type (EtherType) */
    UINT64              Timestamp;          /**< Timestamp (nanoseconds) */

    // Checksum Information
    BOOLEAN             bChecksumValid;     /**< Checksum verified (RX) */
    BOOLEAN             bChecksumOffload;   /**< Use checksum offload (TX) */
    UINT8               ChecksumStart;      /**< Checksum start offset */
    UINT8               ChecksumOffset;     /**< Checksum field offset */

    // Flags
    UINT32              Flags;              /**< Packet flags */

    // Private driver data
    VOID               *pPrivate;           /**< Private driver data */
} NETWORK_PACKET;

/**
 * @brief Packet Flags
 */
#define PACKET_FLAG_TSO                 0x00000001  /**< TSO enabled */
#define PACKET_FLAG_VLAN                0x00000002  /**< VLAN tag present */
#define PACKET_FLAG_BROADCAST           0x00000004  /**< Broadcast packet */
#define PACKET_FLAG_MULTICAST           0x00000008  /**< Multicast packet */
#define PACKET_FLAG_ERROR               0x00000010  /**< Error in packet (RX) */
#define PACKET_FLAG_TRUNCATED           0x00000020  /**< Packet truncated */

/**
 * @brief Network Statistics
 */
typedef struct _NETWORK_STATS {
    // RX Statistics
    UINT64              RXPackets;          /**< Received packets */
    UINT64              RXBytes;            /**< Received bytes */
    UINT64              RXErrors;           /**< Receive errors */
    UINT64              RXDropped;          /**< Dropped packets (RX) */
    UINT64              RXOverrun;          /**< RX buffer overruns */
    UINT64              RXFrameErrors;      /**< Frame alignment errors */
    UINT64              RXCRCErrors;        /**< CRC errors */
    UINT64              RXMulticast;        /**< Multicast packets received */

    // TX Statistics
    UINT64              TXPackets;          /**< Transmitted packets */
    UINT64              TXBytes;            /**< Transmitted bytes */
    UINT64              TXErrors;           /**< Transmit errors */
    UINT64              TXDropped;          /**< Dropped packets (TX) */
    UINT64              TXCarrierErrors;    /**< Carrier errors */
    UINT64              TXCollisions;       /**< Collisions */
    UINT64              TXAbortedErrors;    /**< Aborted transmissions */
    UINT64              TXFIFOErrors;       /**< FIFO errors */

    // Link State
    UINT32              LinkStateChanges;   /**< Number of link state changes */

    // Queue Depths
    UINT32              TXQueueDepth;       /**< Current TX queue depth */
    UINT32              RXQueueDepth;       /**< Current RX queue depth */
} NETWORK_STATS;

/**
 * @brief WiFi Network Information (for scanning)
 */
typedef struct _WIFI_NETWORK_INFO {
    CHAR8               SSID[33];           /**< Network SSID (null-terminated) */
    UINT8               BSSID[6];           /**< BSSID (MAC address) */
    INT32               SignalStrength;     /**< Signal strength (RSSI in dBm) */
    UINT32              Channel;            /**< Channel number */
    UINT32              Frequency;          /**< Frequency (MHz) */
    WIFI_STANDARD       Standard;           /**< WiFi standard */
    WIFI_SECURITY       Security;           /**< Security mode */
    WIFI_CHANNEL_WIDTH  ChannelWidth;       /**< Channel width */
    UINT8               SignalQuality;      /**< Signal quality (0-100%) */
} WIFI_NETWORK_INFO;

/**
 * @brief Multicast Address Entry
 */
typedef struct _MULTICAST_ADDRESS {
    UINT8               Address[MAX_MAC_ADDRESS_LENGTH];
    UINT8               Length;
} MULTICAST_ADDRESS;

/**
 * @brief Receive Callback Function
 *
 * Called when a packet is received by the network interface.
 *
 * @param pContext      User context pointer
 * @param pPacket       Received packet
 *
 * @retval IO_SUCCESS   Packet processed successfully
 */
typedef IO_RETURN (*PFN_NETWORK_RECEIVE_CALLBACK)(
    VOID *pContext,
    NETWORK_PACKET *pPacket
    );

//
// Forward declarations
//
DECLARE_INTERFACE_(IIONetworkController, IIOService);
DECLARE_INTERFACE_(IIONetworkInterface, IIOService);
DECLARE_INTERFACE_(IIONetworkPacket, IUnknown);

/**
 * @brief IIONetworkController - Network Controller Interface
 *
 * This interface represents a network controller (NIC, WiFi adapter, etc.)
 * and provides methods for controller-level operations.
 */
#undef INTERFACE
#define INTERFACE IIONetworkController

DECLARE_INTERFACE_(IIONetworkController, IIOService)
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

    // IIONetworkController methods

    /**
     * @brief Get controller information
     *
     * Retrieves comprehensive information about the network controller.
     *
     * @param pControllerInfo   Receives controller information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_NOT_READY     Controller not ready
     */
    STDMETHOD_(IO_RETURN, GetControllerInfo)(THIS_
        NETWORK_CONTROLLER_INFO *pControllerInfo
        ) PURE;

    /**
     * @brief Get MAC address
     *
     * Retrieves the MAC address of the network controller.
     *
     * @param pMACAddress       Buffer to receive MAC address
     * @param cbSize            Size of buffer (at least MAX_MAC_ADDRESS_LENGTH)
     *
     * @retval IO_SUCCESS       MAC address retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetMACAddress)(THIS_
        UINT8 *pMACAddress,
        UINTN cbSize
        ) PURE;

    /**
     * @brief Set MAC address
     *
     * Sets the MAC address of the network controller.
     *
     * @param pMACAddress       New MAC address
     * @param cbSize            Size of MAC address (6 or 8 bytes)
     *
     * @retval IO_SUCCESS       MAC address set successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_UNSUPPORTED   MAC address change not supported
     */
    STDMETHOD_(IO_RETURN, SetMACAddress)(THIS_
        CONST UINT8 *pMACAddress,
        UINTN cbSize
        ) PURE;

    /**
     * @brief Get link state
     *
     * Retrieves the current link state (up/down/speed/duplex).
     *
     * @param puLinkState       Receives link state flags
     * @param pSpeed            Receives current speed (may be NULL)
     * @param pDuplex           Receives current duplex (may be NULL)
     *
     * @retval IO_SUCCESS       Link state retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetLinkState)(THIS_
        UINT32 *puLinkState,
        NETWORK_SPEED *pSpeed,
        NETWORK_DUPLEX *pDuplex
        ) PURE;

    /**
     * @brief Set media type
     *
     * Sets the media type (e.g., 1000BASE-T, 10GBASE-SR).
     *
     * @param MediaType         Media type to set
     *
     * @retval IO_SUCCESS       Media type set successfully
     * @retval IO_UNSUPPORTED   Media type not supported
     */
    STDMETHOD_(IO_RETURN, SetMediaType)(THIS_
        NETWORK_MEDIA_TYPE MediaType
        ) PURE;

    /**
     * @brief Set link speed
     *
     * Sets the link speed. Use SPEED_UNKNOWN for auto-negotiation.
     *
     * @param Speed             Desired speed
     *
     * @retval IO_SUCCESS       Speed set successfully
     * @retval IO_UNSUPPORTED   Speed not supported
     */
    STDMETHOD_(IO_RETURN, SetSpeed)(THIS_
        NETWORK_SPEED Speed
        ) PURE;

    /**
     * @brief Set duplex mode
     *
     * Sets the duplex mode.
     *
     * @param Duplex            Desired duplex mode
     *
     * @retval IO_SUCCESS       Duplex set successfully
     * @retval IO_UNSUPPORTED   Duplex mode not supported
     */
    STDMETHOD_(IO_RETURN, SetDuplex)(THIS_
        NETWORK_DUPLEX Duplex
        ) PURE;

    /**
     * @brief Enable promiscuous mode
     *
     * Enables promiscuous mode (receive all packets).
     *
     * @retval IO_SUCCESS       Promiscuous mode enabled
     * @retval IO_UNSUPPORTED   Not supported
     */
    STDMETHOD_(IO_RETURN, EnablePromiscuousMode)(THIS) PURE;

    /**
     * @brief Disable promiscuous mode
     *
     * Disables promiscuous mode.
     *
     * @retval IO_SUCCESS       Promiscuous mode disabled
     */
    STDMETHOD_(IO_RETURN, DisablePromiscuousMode)(THIS) PURE;

    /**
     * @brief Add multicast address
     *
     * Adds a multicast address to the filter list.
     *
     * @param pAddress          Multicast MAC address
     * @param cbSize            Size of address
     *
     * @retval IO_SUCCESS       Address added successfully
     * @retval IO_BAD_ARGUMENT  Invalid address
     * @retval IO_NO_RESOURCES  Filter table full
     */
    STDMETHOD_(IO_RETURN, AddMulticastAddress)(THIS_
        CONST UINT8 *pAddress,
        UINTN cbSize
        ) PURE;

    /**
     * @brief Remove multicast address
     *
     * Removes a multicast address from the filter list.
     *
     * @param pAddress          Multicast MAC address
     * @param cbSize            Size of address
     *
     * @retval IO_SUCCESS       Address removed successfully
     * @retval IO_NO_MATCH      Address not found
     */
    STDMETHOD_(IO_RETURN, RemoveMulticastAddress)(THIS_
        CONST UINT8 *pAddress,
        UINTN cbSize
        ) PURE;

    /**
     * @brief Set VLAN
     *
     * Sets the VLAN tag for untagged packets.
     *
     * @param uVLANTag          VLAN tag (0 = disable)
     *
     * @retval IO_SUCCESS       VLAN set successfully
     * @retval IO_UNSUPPORTED   VLAN not supported
     */
    STDMETHOD_(IO_RETURN, SetVLAN)(THIS_
        UINT16 uVLANTag
        ) PURE;

    /**
     * @brief Get VLAN
     *
     * Gets the current VLAN tag.
     *
     * @param puVLANTag         Receives VLAN tag
     *
     * @retval IO_SUCCESS       VLAN retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetVLAN)(THIS_
        UINT16 *puVLANTag
        ) PURE;

    /**
     * @brief Get statistics
     *
     * Retrieves network statistics.
     *
     * @param pStats            Receives statistics
     * @param bReset            Reset statistics after reading
     *
     * @retval IO_SUCCESS       Statistics retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetStatistics)(THIS_
        NETWORK_STATS *pStats,
        BOOLEAN bReset
        ) PURE;

    /**
     * @brief Reset statistics
     *
     * Resets all statistics counters to zero.
     *
     * @retval IO_SUCCESS       Statistics reset successfully
     */
    STDMETHOD_(IO_RETURN, ResetStatistics)(THIS) PURE;

    /**
     * @brief Get capabilities
     *
     * Retrieves the device capability flags.
     *
     * @param puCapabilities    Receives capability flags
     *
     * @retval IO_SUCCESS       Capabilities retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetCapabilities)(THIS_
        UINT32 *puCapabilities
        ) PURE;

    /**
     * @brief Enable Wake-on-LAN
     *
     * Enables Wake-on-LAN functionality.
     *
     * @retval IO_SUCCESS       WoL enabled successfully
     * @retval IO_UNSUPPORTED   WoL not supported
     */
    STDMETHOD_(IO_RETURN, EnableWakeOnLAN)(THIS) PURE;

    /**
     * @brief Disable Wake-on-LAN
     *
     * Disables Wake-on-LAN functionality.
     *
     * @retval IO_SUCCESS       WoL disabled successfully
     */
    STDMETHOD_(IO_RETURN, DisableWakeOnLAN)(THIS) PURE;

    /**
     * @brief Reset controller
     *
     * Performs a hardware reset of the controller.
     *
     * @retval IO_SUCCESS       Reset completed successfully
     * @retval IO_ERROR         Reset failed
     */
    STDMETHOD_(IO_RETURN, ResetController)(THIS) PURE;

    /**
     * @brief Enable controller
     *
     * Enables the network controller.
     *
     * @retval IO_SUCCESS       Controller enabled
     */
    STDMETHOD_(IO_RETURN, Enable)(THIS) PURE;

    /**
     * @brief Disable controller
     *
     * Disables the network controller.
     *
     * @retval IO_SUCCESS       Controller disabled
     */
    STDMETHOD_(IO_RETURN, Disable)(THIS) PURE;
};

#undef INTERFACE

/**
 * @brief IIONetworkInterface - Network Interface Interface
 *
 * This interface represents a network interface (which can be one of
 * multiple interfaces on a controller) and provides packet I/O operations.
 */
#undef INTERFACE
#define INTERFACE IIONetworkInterface

DECLARE_INTERFACE_(IIONetworkInterface, IIOService)
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

    // IIONetworkInterface methods

    /**
     * @brief Get interface information
     *
     * Retrieves information about the network interface.
     *
     * @param pInterfaceInfo    Receives interface information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetInterfaceInfo)(THIS_
        NETWORK_INTERFACE_INFO *pInterfaceInfo
        ) PURE;

    /**
     * @brief Send packet
     *
     * Transmits a network packet.
     *
     * @param pPacket           Packet to transmit
     *
     * @retval IO_SUCCESS       Packet queued/sent successfully
     * @retval IO_BAD_ARGUMENT  Invalid packet
     * @retval IO_NO_RESOURCES  TX queue full
     * @retval IO_NOT_READY     Interface not ready
     */
    STDMETHOD_(IO_RETURN, SendPacket)(THIS_
        NETWORK_PACKET *pPacket
        ) PURE;

    /**
     * @brief Receive packet
     *
     * Receives a network packet (blocking or non-blocking based on flags).
     *
     * @param pPacket           Receives packet data
     * @param uTimeout          Timeout in milliseconds (0 = non-blocking)
     *
     * @retval IO_SUCCESS       Packet received successfully
     * @retval IO_TIMEOUT       No packet available (timeout)
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, ReceivePacket)(THIS_
        NETWORK_PACKET *pPacket,
        UINT32 uTimeout
        ) PURE;

    /**
     * @brief Register receive callback
     *
     * Registers a callback function to be called when packets are received.
     *
     * @param pfnCallback       Callback function
     * @param pContext          User context pointer
     *
     * @retval IO_SUCCESS       Callback registered successfully
     * @retval IO_BAD_ARGUMENT  Invalid callback
     */
    STDMETHOD_(IO_RETURN, RegisterReceiveCallback)(THIS_
        PFN_NETWORK_RECEIVE_CALLBACK pfnCallback,
        VOID *pContext
        ) PURE;

    /**
     * @brief Set MTU
     *
     * Sets the Maximum Transmission Unit size.
     *
     * @param uMTU              New MTU value
     *
     * @retval IO_SUCCESS       MTU set successfully
     * @retval IO_BAD_ARGUMENT  Invalid MTU
     * @retval IO_UNSUPPORTED   MTU change not supported
     */
    STDMETHOD_(IO_RETURN, SetMTU)(THIS_
        UINT32 uMTU
        ) PURE;

    /**
     * @brief Get MTU
     *
     * Gets the current Maximum Transmission Unit size.
     *
     * @param puMTU             Receives MTU value
     *
     * @retval IO_SUCCESS       MTU retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetMTU)(THIS_
        UINT32 *puMTU
        ) PURE;

    /**
     * @brief Bring interface up
     *
     * Brings the network interface up (ready for traffic).
     *
     * @retval IO_SUCCESS       Interface brought up
     * @retval IO_ERROR         Failed to bring up interface
     */
    STDMETHOD_(IO_RETURN, Up)(THIS) PURE;

    /**
     * @brief Bring interface down
     *
     * Brings the network interface down (no traffic).
     *
     * @retval IO_SUCCESS       Interface brought down
     */
    STDMETHOD_(IO_RETURN, Down)(THIS) PURE;

    /**
     * @brief Get queue depth
     *
     * Gets the current depth of the TX/RX queues.
     *
     * @param puTXDepth         Receives TX queue depth
     * @param puRXDepth         Receives RX queue depth
     *
     * @retval IO_SUCCESS       Queue depths retrieved
     */
    STDMETHOD_(IO_RETURN, GetQueueDepth)(THIS_
        UINT32 *puTXDepth,
        UINT32 *puRXDepth
        ) PURE;

    // WiFi-Specific Methods (return IO_UNSUPPORTED for non-WiFi devices)

    /**
     * @brief Scan for WiFi networks
     *
     * Scans for available WiFi networks.
     *
     * @param pNetworks         Array to receive network information
     * @param uMaxNetworks      Maximum number of networks
     * @param puNumNetworks     Receives actual number found
     *
     * @retval IO_SUCCESS       Scan completed successfully
     * @retval IO_UNSUPPORTED   Not a WiFi device
     * @retval IO_BUSY          Scan already in progress
     */
    STDMETHOD_(IO_RETURN, ScanNetworks)(THIS_
        WIFI_NETWORK_INFO *pNetworks,
        UINT32 uMaxNetworks,
        UINT32 *puNumNetworks
        ) PURE;

    /**
     * @brief Connect to WiFi network
     *
     * Connects to a WiFi network.
     *
     * @param pSSID             Network SSID
     * @param pPassword         Network password (NULL for open)
     * @param Security          Security mode
     *
     * @retval IO_SUCCESS       Connected successfully
     * @retval IO_UNSUPPORTED   Not a WiFi device
     * @retval IO_TIMEOUT       Connection timeout
     * @retval IO_ERROR         Connection failed
     */
    STDMETHOD_(IO_RETURN, Connect)(THIS_
        CONST CHAR8 *pSSID,
        CONST CHAR8 *pPassword,
        WIFI_SECURITY Security
        ) PURE;

    /**
     * @brief Disconnect from WiFi network
     *
     * Disconnects from the current WiFi network.
     *
     * @retval IO_SUCCESS       Disconnected successfully
     * @retval IO_UNSUPPORTED   Not a WiFi device
     */
    STDMETHOD_(IO_RETURN, Disconnect)(THIS) PURE;

    /**
     * @brief Get signal strength
     *
     * Gets the current signal strength (RSSI).
     *
     * @param pRSSI             Receives RSSI in dBm
     *
     * @retval IO_SUCCESS       RSSI retrieved successfully
     * @retval IO_UNSUPPORTED   Not a WiFi device
     * @retval IO_NOT_READY     Not connected
     */
    STDMETHOD_(IO_RETURN, GetSignalStrength)(THIS_
        INT32 *pRSSI
        ) PURE;

    /**
     * @brief Set WiFi channel
     *
     * Sets the WiFi channel.
     *
     * @param uChannel          Channel number
     *
     * @retval IO_SUCCESS       Channel set successfully
     * @retval IO_UNSUPPORTED   Not a WiFi device
     * @retval IO_BAD_ARGUMENT  Invalid channel
     */
    STDMETHOD_(IO_RETURN, SetChannel)(THIS_
        UINT32 uChannel
        ) PURE;

    /**
     * @brief Get WiFi channel
     *
     * Gets the current WiFi channel.
     *
     * @param puChannel         Receives channel number
     *
     * @retval IO_SUCCESS       Channel retrieved successfully
     * @retval IO_UNSUPPORTED   Not a WiFi device
     */
    STDMETHOD_(IO_RETURN, GetChannel)(THIS_
        UINT32 *puChannel
        ) PURE;

    /**
     * @brief Set security mode
     *
     * Sets the WiFi security mode.
     *
     * @param Security          Security mode
     *
     * @retval IO_SUCCESS       Security mode set
     * @retval IO_UNSUPPORTED   Not a WiFi device or mode not supported
     */
    STDMETHOD_(IO_RETURN, SetSecurityMode)(THIS_
        WIFI_SECURITY Security
        ) PURE;

    /**
     * @brief Get BSSID
     *
     * Gets the BSSID (MAC address) of the connected access point.
     *
     * @param pBSSID            Receives BSSID (6 bytes)
     *
     * @retval IO_SUCCESS       BSSID retrieved successfully
     * @retval IO_UNSUPPORTED   Not a WiFi device
     * @retval IO_NOT_READY     Not connected
     */
    STDMETHOD_(IO_RETURN, GetBSSID)(THIS_
        UINT8 *pBSSID
        ) PURE;

    /**
     * @brief Enable monitor mode
     *
     * Enables WiFi monitor mode (promiscuous packet capture).
     *
     * @retval IO_SUCCESS       Monitor mode enabled
     * @retval IO_UNSUPPORTED   Not supported
     */
    STDMETHOD_(IO_RETURN, EnableMonitorMode)(THIS) PURE;

    /**
     * @brief Disable monitor mode
     *
     * Disables WiFi monitor mode.
     *
     * @retval IO_SUCCESS       Monitor mode disabled
     */
    STDMETHOD_(IO_RETURN, DisableMonitorMode)(THIS) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIONetworkController methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIONetworkController_GetControllerInfo(p,a)        (p)->lpVtbl->GetControllerInfo(p,a)
#define IIONetworkController_GetMACAddress(p,a,b)          (p)->lpVtbl->GetMACAddress(p,a,b)
#define IIONetworkController_SetMACAddress(p,a,b)          (p)->lpVtbl->SetMACAddress(p,a,b)
#define IIONetworkController_GetLinkState(p,a,b,c)         (p)->lpVtbl->GetLinkState(p,a,b,c)
#define IIONetworkController_SetMediaType(p,a)             (p)->lpVtbl->SetMediaType(p,a)
#define IIONetworkController_SetSpeed(p,a)                 (p)->lpVtbl->SetSpeed(p,a)
#define IIONetworkController_SetDuplex(p,a)                (p)->lpVtbl->SetDuplex(p,a)
#define IIONetworkController_EnablePromiscuousMode(p)      (p)->lpVtbl->EnablePromiscuousMode(p)
#define IIONetworkController_DisablePromiscuousMode(p)     (p)->lpVtbl->DisablePromiscuousMode(p)
#define IIONetworkController_AddMulticastAddress(p,a,b)    (p)->lpVtbl->AddMulticastAddress(p,a,b)
#define IIONetworkController_RemoveMulticastAddress(p,a,b) (p)->lpVtbl->RemoveMulticastAddress(p,a,b)
#define IIONetworkController_SetVLAN(p,a)                  (p)->lpVtbl->SetVLAN(p,a)
#define IIONetworkController_GetVLAN(p,a)                  (p)->lpVtbl->GetVLAN(p,a)
#define IIONetworkController_GetStatistics(p,a,b)          (p)->lpVtbl->GetStatistics(p,a,b)
#define IIONetworkController_ResetStatistics(p)            (p)->lpVtbl->ResetStatistics(p)
#define IIONetworkController_GetCapabilities(p,a)          (p)->lpVtbl->GetCapabilities(p,a)
#define IIONetworkController_EnableWakeOnLAN(p)            (p)->lpVtbl->EnableWakeOnLAN(p)
#define IIONetworkController_DisableWakeOnLAN(p)           (p)->lpVtbl->DisableWakeOnLAN(p)
#define IIONetworkController_ResetController(p)            (p)->lpVtbl->ResetController(p)
#define IIONetworkController_Enable(p)                     (p)->lpVtbl->Enable(p)
#define IIONetworkController_Disable(p)                    (p)->lpVtbl->Disable(p)

#define IIONetworkInterface_GetInterfaceInfo(p,a)          (p)->lpVtbl->GetInterfaceInfo(p,a)
#define IIONetworkInterface_SendPacket(p,a)                (p)->lpVtbl->SendPacket(p,a)
#define IIONetworkInterface_ReceivePacket(p,a,b)           (p)->lpVtbl->ReceivePacket(p,a,b)
#define IIONetworkInterface_RegisterReceiveCallback(p,a,b) (p)->lpVtbl->RegisterReceiveCallback(p,a,b)
#define IIONetworkInterface_SetMTU(p,a)                    (p)->lpVtbl->SetMTU(p,a)
#define IIONetworkInterface_GetMTU(p,a)                    (p)->lpVtbl->GetMTU(p,a)
#define IIONetworkInterface_Up(p)                          (p)->lpVtbl->Up(p)
#define IIONetworkInterface_Down(p)                        (p)->lpVtbl->Down(p)
#define IIONetworkInterface_GetQueueDepth(p,a,b)           (p)->lpVtbl->GetQueueDepth(p,a,b)
#define IIONetworkInterface_ScanNetworks(p,a,b,c)          (p)->lpVtbl->ScanNetworks(p,a,b,c)
#define IIONetworkInterface_Connect(p,a,b,c)               (p)->lpVtbl->Connect(p,a,b,c)
#define IIONetworkInterface_Disconnect(p)                  (p)->lpVtbl->Disconnect(p)
#define IIONetworkInterface_GetSignalStrength(p,a)         (p)->lpVtbl->GetSignalStrength(p,a)
#define IIONetworkInterface_SetChannel(p,a)                (p)->lpVtbl->SetChannel(p,a)
#define IIONetworkInterface_GetChannel(p,a)                (p)->lpVtbl->GetChannel(p,a)
#define IIONetworkInterface_SetSecurityMode(p,a)           (p)->lpVtbl->SetSecurityMode(p,a)
#define IIONetworkInterface_GetBSSID(p,a)                  (p)->lpVtbl->GetBSSID(p,a)
#define IIONetworkInterface_EnableMonitorMode(p)           (p)->lpVtbl->EnableMonitorMode(p)
#define IIONetworkInterface_DisableMonitorMode(p)          (p)->lpVtbl->DisableMonitorMode(p)

#endif

/**
 * @brief Initialize Network family subsystem
 *
 * Initializes the network abstraction layer and registers it with IOKit.
 *
 * @retval IO_SUCCESS   Initialization successful
 * @retval IO_ERROR     Initialization failed
 */
IO_RETURN
NetworkInitialize(
    VOID
    );

/**
 * @brief Shutdown Network family subsystem
 *
 * Shuts down the network abstraction layer and releases resources.
 *
 * @retval IO_SUCCESS   Shutdown successful
 */
IO_RETURN
NetworkShutdown(
    VOID
    );

/**
 * @brief Create a network controller instance
 *
 * Creates a network controller interface wrapping a bus-specific device.
 *
 * @param pBusDevice        Bus-specific device (PCIe, USB, etc.)
 * @param DeviceType        Network device type
 * @param ppController      Receives network controller interface
 *
 * @retval IO_SUCCESS           Controller created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid argument
 */
IO_RETURN
NetworkControllerCreate(
    IIOService *pBusDevice,
    NETWORK_DEVICE_TYPE DeviceType,
    IIONetworkController **ppController
    );

/**
 * @brief Create a network interface instance
 *
 * Creates a network interface attached to a controller.
 *
 * @param pController       Parent controller
 * @param pszInterfaceName  Interface name (e.g., "eth0")
 * @param ppInterface       Receives network interface
 *
 * @retval IO_SUCCESS           Interface created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid argument
 */
IO_RETURN
NetworkInterfaceCreate(
    IIONetworkController *pController,
    CONST CHAR8 *pszInterfaceName,
    IIONetworkInterface **ppInterface
    );

/**
 * @brief Detect network device type
 *
 * Helper function to detect the network device type by examining
 * PCI IDs and other characteristics.
 *
 * @param pDevice           Device to examine
 * @param pDeviceType       Receives detected device type
 *
 * @retval IO_SUCCESS       Device type detected successfully
 * @retval IO_NO_MATCH      Device type could not be determined
 * @retval IO_BAD_ARGUMENT  Invalid argument
 */
IO_RETURN
NetworkDetectDeviceType(
    IIOService *pDevice,
    NETWORK_DEVICE_TYPE *pDeviceType
    );

/**
 * @brief Get network device name from PCI IDs
 *
 * Looks up the device name based on PCI Vendor ID and Device ID.
 *
 * @param uVendorID         PCI Vendor ID
 * @param uDeviceID         PCI Device ID
 * @param pszName           Buffer to receive device name
 * @param cbSize            Size of buffer
 *
 * @retval IO_SUCCESS       Device name found
 * @retval IO_NO_MATCH      Device not in database
 * @retval IO_BAD_ARGUMENT  Invalid argument
 */
IO_RETURN
NetworkGetDeviceName(
    UINT16 uVendorID,
    UINT16 uDeviceID,
    CHAR8 *pszName,
    UINTN cbSize
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_NETWORK_H */
