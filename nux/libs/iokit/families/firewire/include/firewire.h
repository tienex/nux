/**
 * @file firewire.h
 * @brief FireWire (IEEE 1394) Family Interface - Complete IEEE 1394 Support
 *
 * This header defines the FireWire (IEEE 1394, i.LINK) family interface for
 * managing FireWire host controllers, buses, and devices.
 *
 * FireWire is a high-speed serial bus developed by Apple (FireWire), standardized
 * as IEEE 1394, and marketed as i.LINK by Sony. It was widely used for external
 * storage, digital video cameras (DV), and professional audio interfaces.
 *
 * Supports:
 * - IEEE 1394-1995 (FireWire 400): 100/200/400 Mbps, DS-Link encoding
 * - IEEE 1394a-2000: Enhanced arbitration, suspend/resume, power management
 * - IEEE 1394b-2002 (FireWire 800): 800 Mbps, Beta mode (8b/10b), Cat5 support
 * - IEEE 1394-2008: S1600 (1.6 Gbps), S3200 (3.2 Gbps)
 *
 * Physical Layers:
 * - DS-Link (Data/Strobe encoding) - IEEE 1394-1995/a
 * - Beta mode (8b/10b encoding) - IEEE 1394b
 * - Bilingual PHY (supports both DS-Link and Beta)
 *
 * Cable Types:
 * - 4-pin (unpowered, used on DV cameras and laptops)
 * - 6-pin (powered, 8-40V, desktop computers and peripherals)
 * - 9-pin (IEEE 1394b, beta mode, FireWire 800)
 *
 * Device Classes:
 * - SBP-2 (Serial Bus Protocol 2) - Storage devices (hard drives, CD/DVD)
 * - AV/C (Audio/Video Control) - Cameras, VCRs, camcorders
 * - IEC 61883 - Consumer electronics (DV, MPEG2-TS)
 * - IP over 1394 (RFC 2734) - Networking
 * - IIDC (Instrumentation & Industrial Digital Camera)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_FIREWIRE_H
#define IOKIT_FIREWIRE_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOFireWireController interface GUID
 * {F1E2D3C4-B5A6-4978-8D6E-9F0A1B2C3D4E}
 */
DEFINE_GUID(IID_IIOFireWireController,
    0xF1E2D3C4, 0xB5A6, 0x4978, 0x8D, 0x6E, 0x9F, 0x0A, 0x1B, 0x2C, 0x3D, 0x4E);

/**
 * @brief IIOFireWireBus interface GUID
 * {E2F3D4C5-A6B7-4089-9E7F-0A1B2C3D4E5F}
 */
DEFINE_GUID(IID_IIOFireWireBus,
    0xE2F3D4C5, 0xA6B7, 0x4089, 0x9E, 0x7F, 0x0A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F);

/**
 * @brief IIOFireWireDevice interface GUID
 * {D3E4F5C6-B7A8-4190-8F7E-1B2C3D4E5F60}
 */
DEFINE_GUID(IID_IIOFireWireDevice,
    0xD3E4F5C6, 0xB7A8, 0x4190, 0x8F, 0x7E, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F, 0x60);

/**
 * @brief IIOFireWireUnit interface GUID
 * {C4D5E6F7-A8B9-40A1-9E8F-2C3D4E5F6071}
 */
DEFINE_GUID(IID_IIOFireWireUnit,
    0xC4D5E6F7, 0xA8B9, 0x40A1, 0x9E, 0x8F, 0x2C, 0x3D, 0x4E, 0x5F, 0x60, 0x71);

/**
 * @brief IIOFireWireIsochChannel interface GUID
 * {B5C6D7E8-99AA-41B2-8F9E-3D4E5F607182}
 */
DEFINE_GUID(IID_IIOFireWireIsochChannel,
    0xB5C6D7E8, 0x99AA, 0x41B2, 0x8F, 0x9E, 0x3D, 0x4E, 0x5F, 0x60, 0x71, 0x82);

/**
 * @brief IEEE 1394 Version/Generation
 */
typedef enum _FW_VERSION {
    FW_VERSION_UNKNOWN      = 0x0000,
    FW_VERSION_1394_1995    = 0x0100,   /**< IEEE 1394-1995 (S100/S200/S400) */
    FW_VERSION_1394A_2000   = 0x0110,   /**< IEEE 1394a-2000 (enhanced) */
    FW_VERSION_1394B_2002   = 0x0200,   /**< IEEE 1394b-2002 (S800, beta mode) */
    FW_VERSION_1394_2008    = 0x0300,   /**< IEEE 1394-2008 (S1600/S3200) */
} FW_VERSION;

/**
 * @brief FireWire Speed Codes
 *
 * Speed codes as defined in IEEE 1394 specification.
 * Actual speeds are base rate of 24.576 MHz * (2^speed).
 */
typedef enum _FW_SPEED {
    FW_SPEED_100            = 0,        /**< S100: 98.304 Mbps (100 Mbps nominal) */
    FW_SPEED_200            = 1,        /**< S200: 196.608 Mbps (200 Mbps nominal) */
    FW_SPEED_400            = 2,        /**< S400: 393.216 Mbps (400 Mbps nominal) */
    FW_SPEED_800            = 3,        /**< S800: 786.432 Mbps (800 Mbps nominal, 1394b) */
    FW_SPEED_1600           = 4,        /**< S1600: 1572.864 Mbps (1.6 Gbps, 1394-2008) */
    FW_SPEED_3200           = 5,        /**< S3200: 3145.728 Mbps (3.2 Gbps, 1394-2008) */
} FW_SPEED;

/**
 * @brief FireWire Physical Layer Type
 */
typedef enum _FW_PHY_TYPE {
    FW_PHY_DSLINK           = 0,        /**< DS-Link (Data/Strobe) encoding */
    FW_PHY_BETA             = 1,        /**< Beta mode (8b/10b) encoding */
    FW_PHY_BILINGUAL        = 2,        /**< Bilingual (both DS-Link and Beta) */
} FW_PHY_TYPE;

/**
 * @brief FireWire Cable Type
 */
typedef enum _FW_CABLE_TYPE {
    FW_CABLE_4PIN           = 0,        /**< 4-pin (no power) */
    FW_CABLE_6PIN           = 1,        /**< 6-pin (8-40V power) */
    FW_CABLE_9PIN           = 2,        /**< 9-pin (IEEE 1394b) */
} FW_CABLE_TYPE;

/**
 * @brief FireWire Controller Type
 */
typedef enum _FW_CONTROLLER_TYPE {
    FW_CONTROLLER_OHCI      = 1,        /**< OHCI (Open Host Controller Interface) */
    FW_CONTROLLER_LYNX      = 2,        /**< Texas Instruments LYNX */
} FW_CONTROLLER_TYPE;

/**
 * @brief FireWire PHY Chip Types
 *
 * Common PHY (Physical Layer) chips found in FireWire controllers.
 */
typedef enum _FW_PHY_CHIP {
    FW_PHY_UNKNOWN          = 0x0000,

    // Texas Instruments PHYs
    FW_PHY_TI_TSB41AB1      = 0x0001,   /**< TI TSB41AB1 (1-port, 1394a) */
    FW_PHY_TI_TSB41AB2      = 0x0002,   /**< TI TSB41AB2 (3-port, 1394a) */
    FW_PHY_TI_TSB41AB3      = 0x0003,   /**< TI TSB41AB3 (3-port, 1394a, enhanced) */
    FW_PHY_TI_TSB43AB22     = 0x0022,   /**< TI TSB43AB22 (3-port, 1394b, beta mode) */
    FW_PHY_TI_TSB43AB23     = 0x0023,   /**< TI TSB43AB23 (3-port, 1394b, bilingual) */
    FW_PHY_TI_TSB82AA2      = 0x00A2,   /**< TI TSB82AA2 (2-port, 1394b) */

    // Agere PHYs
    FW_PHY_AGERE_FW802      = 0x0100,   /**< Agere FW802 (1394a) */
    FW_PHY_AGERE_FW803      = 0x0101,   /**< Agere FW803 (1394b) */

    // VIA PHYs
    FW_PHY_VIA_VT6306       = 0x0200,   /**< VIA VT6306 PHY */
} FW_PHY_CHIP;

/**
 * @brief FireWire Transaction Type
 */
typedef enum _FW_TRANSACTION_TYPE {
    FW_TRANS_READ_QUADLET_REQUEST       = 0x00, /**< Read single quadlet */
    FW_TRANS_READ_QUADLET_RESPONSE      = 0x01, /**< Read quadlet response */
    FW_TRANS_READ_BLOCK_REQUEST         = 0x04, /**< Read block */
    FW_TRANS_READ_BLOCK_RESPONSE        = 0x05, /**< Read block response */
    FW_TRANS_WRITE_QUADLET_REQUEST      = 0x08, /**< Write single quadlet */
    FW_TRANS_WRITE_QUADLET_RESPONSE     = 0x09, /**< Write quadlet response */
    FW_TRANS_WRITE_BLOCK_REQUEST        = 0x0C, /**< Write block */
    FW_TRANS_WRITE_BLOCK_RESPONSE       = 0x0D, /**< Write block response */
    FW_TRANS_LOCK_REQUEST               = 0x10, /**< Lock (atomic) request */
    FW_TRANS_LOCK_RESPONSE              = 0x11, /**< Lock response */
    FW_TRANS_ISOCHRONOUS                = 0x1F, /**< Isochronous packet */
} FW_TRANSACTION_TYPE;

/**
 * @brief FireWire Transfer Type
 */
typedef enum _FW_TRANSFER_TYPE {
    FW_TRANSFER_ASYNCHRONOUS    = 0,    /**< Asynchronous transfer (guaranteed delivery) */
    FW_TRANSFER_ISOCHRONOUS     = 1,    /**< Isochronous transfer (real-time, best-effort) */
} FW_TRANSFER_TYPE;

/**
 * @brief FireWire Device Class
 */
typedef enum _FW_DEVICE_CLASS {
    FW_CLASS_SBP2           = 0x010483, /**< SBP-2 (Serial Bus Protocol 2) - Storage */
    FW_CLASS_AVC            = 0x00A02D, /**< AV/C (Audio/Video Control) */
    FW_CLASS_IEC61883       = 0x00A02D, /**< IEC 61883 (Consumer electronics) */
    FW_CLASS_IP1394         = 0x000000, /**< IP over 1394 (RFC 2734) */
    FW_CLASS_DV             = 0x00A02D, /**< DV (Digital Video) */
    FW_CLASS_IIDC           = 0x00A02D, /**< IIDC (Industrial Digital Camera) */
} FW_DEVICE_CLASS;

/**
 * @brief FireWire Power Class
 */
typedef enum _FW_POWER_CLASS {
    FW_POWER_SELF_POWERED       = 0,    /**< Device has its own power supply */
    FW_POWER_BUS_POWERED        = 1,    /**< Device powered by FireWire bus */
    FW_POWER_BUS_POWERED_1_5W   = 2,    /**< Bus-powered, up to 1.5W */
    FW_POWER_BUS_POWERED_3W     = 3,    /**< Bus-powered, up to 3W */
    FW_POWER_BUS_POWERED_6W     = 4,    /**< Bus-powered, up to 6W */
    FW_POWER_BUS_POWERED_10W    = 5,    /**< Bus-powered, up to 10W */
} FW_POWER_CLASS;

/**
 * @brief FireWire Topology State
 */
typedef enum _FW_TOPOLOGY_STATE {
    FW_TOPO_INVALID         = 0,        /**< Invalid/unknown topology */
    FW_TOPO_BUILDING        = 1,        /**< Building topology after bus reset */
    FW_TOPO_VALID           = 2,        /**< Valid topology */
} FW_TOPOLOGY_STATE;

/**
 * @brief FireWire Lock Transaction Type (Atomic Operations)
 */
typedef enum _FW_LOCK_TYPE {
    FW_LOCK_MASK_SWAP       = 0x00,     /**< Masked bit swap */
    FW_LOCK_COMPARE_SWAP    = 0x01,     /**< Compare and swap */
    FW_LOCK_FETCH_ADD       = 0x02,     /**< Fetch and add */
    FW_LOCK_LITTLE_ADD      = 0x03,     /**< Little-endian add */
    FW_LOCK_BOUNDED_ADD     = 0x04,     /**< Bounded add */
    FW_LOCK_WRAP_ADD        = 0x05,     /**< Wrap-around add */
} FW_LOCK_TYPE;

/**
 * @brief FireWire Response Code
 */
typedef enum _FW_RESPONSE_CODE {
    FW_RESP_COMPLETE        = 0x00,     /**< Request completed successfully */
    FW_RESP_CONFLICT_ERROR  = 0x04,     /**< Resource conflict */
    FW_RESP_DATA_ERROR      = 0x05,     /**< Data error */
    FW_RESP_TYPE_ERROR      = 0x06,     /**< Type error */
    FW_RESP_ADDRESS_ERROR   = 0x07,     /**< Invalid address */
} FW_RESPONSE_CODE;

/**
 * @brief FireWire 64-bit Address
 *
 * IEEE 1394 uses 64-bit addressing:
 * - Bits 63-48: Node ID (10 bits bus ID + 6 bits physical ID)
 * - Bits 47-0:  48-bit offset within node's address space
 */
typedef struct _FW_ADDRESS {
    UINT16  NodeID;                     /**< Node ID (10-bit bus + 6-bit phy) */
    UINT64  Offset;                     /**< 48-bit offset (use lower 48 bits) */
} FW_ADDRESS;

/**
 * @brief FireWire Node Unique ID (EUI-64)
 *
 * Globally unique 64-bit identifier for each FireWire node.
 * Format: 24-bit vendor ID + 40-bit serial number/device ID.
 */
typedef struct _FW_NODE_UNIQUE_ID {
    UINT64  Value;                      /**< EUI-64 unique identifier */
} FW_NODE_UNIQUE_ID;

/**
 * @brief FireWire CSR (Control and Status Register) Space
 *
 * Standard CSR addresses as defined in IEEE 1394.
 */
#define FW_CSR_STATE_CLEAR              0xFFFFF0000000ULL   /**< State clear register */
#define FW_CSR_STATE_SET                0xFFFFF0000004ULL   /**< State set register */
#define FW_CSR_NODE_IDS                 0xFFFFF0000008ULL   /**< Node IDs register */
#define FW_CSR_RESET_START              0xFFFFF000000CULL   /**< Reset start */
#define FW_CSR_SPLIT_TIMEOUT_HI         0xFFFFF0000018ULL   /**< Split timeout (high) */
#define FW_CSR_SPLIT_TIMEOUT_LO         0xFFFFF000001CULL   /**< Split timeout (low) */
#define FW_CSR_CYCLE_TIME               0xFFFFF0000200ULL   /**< Cycle time register */
#define FW_CSR_BUS_TIME                 0xFFFFF0000204ULL   /**< Bus time register */
#define FW_CSR_BUSY_TIMEOUT             0xFFFFF0000210ULL   /**< Busy timeout */
#define FW_CSR_BUS_MANAGER_ID           0xFFFFF000021CULL   /**< Bus manager ID */
#define FW_CSR_BANDWIDTH_AVAILABLE      0xFFFFF0000220ULL   /**< Bandwidth available */
#define FW_CSR_CHANNELS_AVAILABLE_HI    0xFFFFF0000224ULL   /**< Channels available (high) */
#define FW_CSR_CHANNELS_AVAILABLE_LO    0xFFFFF0000228ULL   /**< Channels available (low) */
#define FW_CSR_CONFIG_ROM               0xFFFFF0000400ULL   /**< Configuration ROM base */

/**
 * @brief FireWire Configuration ROM Header
 */
typedef struct _FW_CONFIG_ROM_HEADER {
    UINT32  InfoLength;                 /**< Info length (quadlets) */
    UINT32  CRCLength;                  /**< CRC length */
    UINT32  ROMCRCValue;                /**< ROM CRC value */
    UINT32  BusInfoBlock[5];            /**< Bus info block */
} FW_CONFIG_ROM_HEADER;

/**
 * @brief FireWire Bus Info Block
 */
typedef struct _FW_BUS_INFO_BLOCK {
    UINT32  BusName;                    /**< Bus name (always 0x31333934 = "1394") */
    UINT32  IrmcCmcIscBmc;              /**< Capabilities flags */
    UINT64  NodeUniqueID;               /**< EUI-64 unique ID */
} FW_BUS_INFO_BLOCK;

/**
 * @brief FireWire Controller Information
 */
typedef struct _FW_CONTROLLER_INFO {
    FW_CONTROLLER_TYPE  Type;           /**< Controller type (OHCI, LYNX) */
    FW_VERSION          Version;        /**< IEEE 1394 version */
    UINT16              VendorID;       /**< PCI Vendor ID */
    UINT16              DeviceID;       /**< PCI Device ID */
    FW_PHY_CHIP         PhyType;        /**< PHY chip type */
    FW_PHY_TYPE         PhyMode;        /**< Physical layer mode */
    FW_SPEED            MaxSpeed;       /**< Maximum supported speed */
    UINT8               NumPorts;       /**< Number of PHY ports */
    UINT32              Capabilities;   /**< Capability flags */
    UINT64              GUID;           /**< Controller GUID (EUI-64) */
    UINT64              MMIOBase;       /**< Memory-mapped I/O base */
    UINT32              MMIOSize;       /**< MMIO region size */
    CHAR8               ControllerName[64];  /**< Controller name */
} FW_CONTROLLER_INFO;

/**
 * @brief FireWire Bus Information
 */
typedef struct _FW_BUS_INFO {
    UINT16              BusID;          /**< Bus ID (10-bit, 0-1023) */
    UINT16              RootNodeID;     /**< Root node ID */
    UINT16              LocalNodeID;    /**< Local (host) node ID */
    UINT16              IsochronousResourceManagerID; /**< IRM node ID */
    UINT16              BusManagerID;   /**< Bus manager node ID */
    UINT32              Generation;     /**< Bus generation number */
    FW_TOPOLOGY_STATE   TopologyState;  /**< Topology state */
    UINT8               NodeCount;      /**< Number of nodes on bus */
    FW_SPEED            BusSpeed;       /**< Current bus speed */
    BOOLEAN             bCycleMaster;   /**< Cycle master present */
    BOOLEAN             bIsochronousResourceManager; /**< IRM present */
    BOOLEAN             bBusManager;    /**< Bus manager present */
} FW_BUS_INFO;

/**
 * @brief FireWire Device Information
 */
typedef struct _FW_DEVICE_INFO {
    UINT16              NodeID;         /**< Node ID on current bus */
    FW_NODE_UNIQUE_ID   UniqueID;       /**< Globally unique ID (EUI-64) */
    UINT32              VendorID;       /**< Vendor ID (24-bit) */
    UINT32              ModelID;        /**< Model ID */
    UINT32              SpecifierID;    /**< Specifier ID (protocol) */
    UINT32              Version;        /**< Version */
    FW_SPEED            MaxSpeed;       /**< Maximum supported speed */
    FW_POWER_CLASS      PowerClass;     /**< Power class */
    FW_DEVICE_CLASS     DeviceClass;    /**< Device class */
    CHAR8               VendorName[64]; /**< Vendor name string */
    CHAR8               ModelName[64];  /**< Model name string */
    UINT32              UnitCount;      /**< Number of function units */
    BOOLEAN             bCycleMasterCapable; /**< Can be cycle master */
    BOOLEAN             bIsochronousResourceManagerCapable; /**< Can be IRM */
    BOOLEAN             bBusManagerCapable; /**< Can be bus manager */
} FW_DEVICE_INFO;

/**
 * @brief FireWire Unit Information
 *
 * A unit represents a logical function within a FireWire device.
 */
typedef struct _FW_UNIT_INFO {
    UINT32              SpecifierID;    /**< Protocol specifier ID */
    UINT32              Version;        /**< Protocol version */
    UINT32              ModelID;        /**< Model ID */
    FW_DEVICE_CLASS     Protocol;       /**< Protocol type (SBP-2, AV/C, etc.) */
    CHAR8               UnitName[64];   /**< Unit name/description */
} FW_UNIT_INFO;

/**
 * @brief FireWire Isochronous Channel Information
 */
typedef struct _FW_ISOCH_CHANNEL_INFO {
    UINT8               ChannelNumber;  /**< Channel number (0-63) */
    UINT32              Bandwidth;      /**< Allocated bandwidth (units) */
    UINT32              MaxPayload;     /**< Maximum payload size */
    FW_SPEED            Speed;          /**< Channel speed */
    BOOLEAN             bAllocated;     /**< Channel allocated */
    BOOLEAN             bBroadcast;     /**< Broadcast channel */
} FW_ISOCH_CHANNEL_INFO;

/**
 * @brief FireWire Capability Flags
 */
#define FW_CAP_ISOCH_RESOURCE_MANAGER   0x00000001  /**< IRM capable */
#define FW_CAP_CYCLE_MASTER             0x00000002  /**< Cycle master capable */
#define FW_CAP_ISOCHRONOUS              0x00000004  /**< Isochronous support */
#define FW_CAP_BUS_MANAGER              0x00000008  /**< Bus manager capable */
#define FW_CAP_POWER_MANAGER            0x00000010  /**< Power manager capable */
#define FW_CAP_1394A                    0x00000020  /**< IEEE 1394a support */
#define FW_CAP_1394B                    0x00000040  /**< IEEE 1394b support */
#define FW_CAP_BETA_MODE                0x00000080  /**< Beta mode (8b/10b) */
#define FW_CAP_CONTENDER                0x00000100  /**< Contender bit set */
#define FW_CAP_LINK_ACTIVE              0x00000200  /**< Link layer active */
#define FW_CAP_S100                     0x00001000  /**< S100 support */
#define FW_CAP_S200                     0x00002000  /**< S200 support */
#define FW_CAP_S400                     0x00004000  /**< S400 support */
#define FW_CAP_S800                     0x00008000  /**< S800 support */
#define FW_CAP_S1600                    0x00010000  /**< S1600 support */
#define FW_CAP_S3200                    0x00020000  /**< S3200 support */

//
// OHCI (Open Host Controller Interface) Register Definitions
//

/**
 * @brief OHCI Register Offsets
 */
#define OHCI_VERSION                    0x000   /**< Version register */
#define OHCI_GUID_ROM                   0x004   /**< GUID ROM address */
#define OHCI_AT_RETRY_ENABLE            0x008   /**< AT retry enable */
#define OHCI_CSR_READ_DATA              0x00C   /**< CSR read data */
#define OHCI_CSR_COMPARE_DATA           0x010   /**< CSR compare data */
#define OHCI_CSR_CONTROL                0x014   /**< CSR control */
#define OHCI_CONFIG_ROM_HDR             0x018   /**< Config ROM header */
#define OHCI_BUS_ID                     0x01C   /**< Bus ID */
#define OHCI_BUS_OPTIONS                0x020   /**< Bus options */
#define OHCI_GUID_HI                    0x024   /**< GUID high 32 bits */
#define OHCI_GUID_LO                    0x028   /**< GUID low 32 bits */
#define OHCI_CONFIG_ROM_MAP             0x034   /**< Config ROM map */
#define OHCI_POST_WRITE_ADDR_LO         0x038   /**< Posted write address low */
#define OHCI_POST_WRITE_ADDR_HI         0x03C   /**< Posted write address high */
#define OHCI_VENDOR_ID                  0x040   /**< Vendor ID */

#define OHCI_HC_CONTROL_SET             0x050   /**< HC control set */
#define OHCI_HC_CONTROL_CLEAR           0x054   /**< HC control clear */
#define OHCI_SELF_ID_BUFFER             0x064   /**< Self-ID buffer pointer */
#define OHCI_SELF_ID_COUNT              0x068   /**< Self-ID count */

#define OHCI_IR_MASK_HI_SET             0x090   /**< Interrupt mask high set */
#define OHCI_IR_MASK_HI_CLEAR           0x094   /**< Interrupt mask high clear */
#define OHCI_IR_MASK_LO_SET             0x098   /**< Interrupt mask low set */
#define OHCI_IR_MASK_LO_CLEAR           0x09C   /**< Interrupt mask low clear */

#define OHCI_INT_EVENT_SET              0x080   /**< Interrupt event set */
#define OHCI_INT_EVENT_CLEAR            0x084   /**< Interrupt event clear */
#define OHCI_INT_MASK_SET               0x088   /**< Interrupt mask set */
#define OHCI_INT_MASK_CLEAR             0x08C   /**< Interrupt mask clear */

#define OHCI_FAIRNESS_CONTROL           0x0DC   /**< Fairness control */
#define OHCI_LINK_CONTROL_SET           0x0E0   /**< Link control set */
#define OHCI_LINK_CONTROL_CLEAR         0x0E4   /**< Link control clear */
#define OHCI_NODE_ID                    0x0E8   /**< Node ID */
#define OHCI_PHY_CONTROL                0x0EC   /**< PHY control */
#define OHCI_ISOCH_CYCLE_TIMER          0x0F0   /**< Isochronous cycle timer */

#define OHCI_AT_CONTEXT_CONTROL_SET     0x180   /**< AT context control set */
#define OHCI_AT_CONTEXT_CONTROL_CLEAR   0x184   /**< AT context control clear */
#define OHCI_AT_COMMAND_PTR             0x18C   /**< AT command pointer */

#define OHCI_AR_CONTEXT_CONTROL_SET     0x1A0   /**< AR context control set */
#define OHCI_AR_CONTEXT_CONTROL_CLEAR   0x1A4   /**< AR context control clear */
#define OHCI_AR_COMMAND_PTR             0x1AC   /**< AR command pointer */

/**
 * @brief OHCI Interrupt Event Bits
 */
#define OHCI_INT_REQ_TX_COMPLETE        0x00000001  /**< AT request complete */
#define OHCI_INT_RESP_TX_COMPLETE       0x00000002  /**< AT response complete */
#define OHCI_INT_ARRQ                   0x00000004  /**< AR request received */
#define OHCI_INT_ARRS                   0x00000008  /**< AR response received */
#define OHCI_INT_RQ_PKT                 0x00000010  /**< Request packet */
#define OHCI_INT_RS_PKT                 0x00000020  /**< Response packet */
#define OHCI_INT_ISOCH_TX               0x00000040  /**< Isochronous TX */
#define OHCI_INT_ISOCH_RX               0x00000080  /**< Isochronous RX */
#define OHCI_INT_POST_WR_ERR            0x00000100  /**< Posted write error */
#define OHCI_INT_LOCK_RESP_ERR          0x00000200  /**< Lock response error */
#define OHCI_INT_SELF_ID_COMPLETE       0x00010000  /**< Self-ID complete */
#define OHCI_INT_BUS_RESET              0x00020000  /**< Bus reset */
#define OHCI_INT_REG_ACCESS_FAIL        0x00040000  /**< Register access failure */
#define OHCI_INT_PHY                    0x00080000  /**< PHY interrupt */
#define OHCI_INT_CYCLE_SYNCH            0x00100000  /**< Cycle synch */
#define OHCI_INT_CYCLE_64_SECONDS       0x00200000  /**< 64-second cycle */
#define OHCI_INT_CYCLE_LOST             0x00400000  /**< Cycle lost */
#define OHCI_INT_CYCLE_INCONSISTENT     0x00800000  /**< Cycle inconsistent */
#define OHCI_INT_UNRECOVERABLE_ERROR    0x01000000  /**< Unrecoverable error */
#define OHCI_INT_CYCLE_TOO_LONG         0x02000000  /**< Cycle too long */
#define OHCI_INT_PHY_REG_RCVD           0x04000000  /**< PHY register received */
#define OHCI_INT_VENDOR_SPECIFIC        0x40000000  /**< Vendor specific */

/**
 * @brief OHCI HC Control Bits
 */
#define OHCI_HC_SOFT_RESET              0x00010000  /**< Soft reset */
#define OHCI_HC_LINK_ENABLE             0x00020000  /**< Link enable */

/**
 * @brief OHCI Context Control Bits
 */
#define OHCI_CTX_RUN                    0x00008000  /**< Context run */
#define OHCI_CTX_WAKE                   0x00001000  /**< Context wake */
#define OHCI_CTX_DEAD                   0x00000800  /**< Context dead */
#define OHCI_CTX_ACTIVE                 0x00000400  /**< Context active */

//
// SBP-2 (Serial Bus Protocol 2) Definitions - For Storage Devices
//

/**
 * @brief SBP-2 Unit Characteristics
 */
#define SBP2_UNIT_SPEC_ID               0x00609E    /**< SBP-2 specifier ID */
#define SBP2_UNIT_SW_VERSION            0x010483    /**< SBP-2 software version */

/**
 * @brief SBP-2 ORB (Operation Request Block) Types
 */
typedef enum _SBP2_ORB_TYPE {
    SBP2_ORB_LOGIN              = 0,    /**< Login ORB */
    SBP2_ORB_QUERY_LOGINS       = 1,    /**< Query logins ORB */
    SBP2_ORB_RECONNECT          = 3,    /**< Reconnect ORB */
    SBP2_ORB_SET_PASSWORD       = 4,    /**< Set password ORB */
    SBP2_ORB_LOGOUT             = 7,    /**< Logout ORB */
    SBP2_ORB_ABORT_TASK         = 0xB,  /**< Abort task ORB */
    SBP2_ORB_ABORT_TASK_SET     = 0xC,  /**< Abort task set ORB */
    SBP2_ORB_LOGICAL_UNIT_RESET = 0xE,  /**< LUN reset ORB */
    SBP2_ORB_TARGET_RESET       = 0xF,  /**< Target reset ORB */
} SBP2_ORB_TYPE;

/**
 * @brief SBP-2 Login ORB
 */
typedef struct _SBP2_LOGIN_ORB {
    UINT64  PasswordAddress;            /**< Password address (or 0) */
    UINT64  LoginResponseAddress;       /**< Login response address */
    UINT32  Options;                    /**< Options (LUN, reconnect, exclusive) */
    UINT32  Reserved;
    UINT64  StatusFIFOAddress;          /**< Status FIFO address */
} SBP2_LOGIN_ORB;

/**
 * @brief SBP-2 Command ORB (for SCSI commands)
 */
typedef struct _SBP2_COMMAND_ORB {
    UINT64  NextORBAddress;             /**< Next ORB address (or NULL) */
    UINT64  DataDescriptor;             /**< Data buffer descriptor */
    UINT32  Options;                    /**< Options (direction, speed, max_payload, page_size, etc.) */
    UINT32  DataSize;                   /**< Data transfer size */
    UINT8   CommandBlock[12];           /**< SCSI command block */
} SBP2_COMMAND_ORB;

/**
 * @brief SBP-2 Status Block
 */
typedef struct _SBP2_STATUS_BLOCK {
    UINT8   Flags;                      /**< Status flags */
    UINT8   SBPStatus;                  /**< SBP status */
    UINT16  ORBOffsetHigh;              /**< ORB offset high */
    UINT32  ORBOffsetLow;               /**< ORB offset low */
    UINT8   StatusData[24];             /**< Status data (SCSI sense, etc.) */
} SBP2_STATUS_BLOCK;

/**
 * @brief SBP-2 LUN (Logical Unit Number)
 */
typedef UINT16 SBP2_LUN;

//
// AV/C (Audio/Video Control) Definitions
//

/**
 * @brief AV/C Unit Specifier ID
 */
#define AVC_UNIT_SPEC_ID                0x00A02D    /**< AV/C specifier ID */

/**
 * @brief AV/C Command Type
 */
typedef enum _AVC_COMMAND_TYPE {
    AVC_CMD_CONTROL             = 0x00, /**< Control command */
    AVC_CMD_STATUS              = 0x01, /**< Status inquiry */
    AVC_CMD_SPECIFIC_INQUIRY    = 0x02, /**< Specific inquiry */
    AVC_CMD_NOTIFY              = 0x03, /**< Notify */
    AVC_CMD_GENERAL_INQUIRY     = 0x04, /**< General inquiry */
} AVC_COMMAND_TYPE;

/**
 * @brief AV/C Response Type
 */
typedef enum _AVC_RESPONSE_TYPE {
    AVC_RESP_NOT_IMPLEMENTED    = 0x08, /**< Not implemented */
    AVC_RESP_ACCEPTED           = 0x09, /**< Accepted */
    AVC_RESP_REJECTED           = 0x0A, /**< Rejected */
    AVC_RESP_IN_TRANSITION      = 0x0B, /**< In transition */
    AVC_RESP_IMPLEMENTED        = 0x0C, /**< Implemented/Stable */
    AVC_RESP_CHANGED            = 0x0D, /**< Changed */
    AVC_RESP_INTERIM            = 0x0F, /**< Interim */
} AVC_RESPONSE_TYPE;

/**
 * @brief AV/C Subunit Type
 */
typedef enum _AVC_SUBUNIT_TYPE {
    AVC_SUBUNIT_VIDEO_MONITOR   = 0x00, /**< Video monitor */
    AVC_SUBUNIT_AUDIO           = 0x01, /**< Audio */
    AVC_SUBUNIT_PRINTER         = 0x02, /**< Printer */
    AVC_SUBUNIT_DISC            = 0x03, /**< Disc recorder/player */
    AVC_SUBUNIT_TAPE_RECORDER   = 0x04, /**< Tape recorder/player */
    AVC_SUBUNIT_TUNER           = 0x05, /**< Tuner */
    AVC_SUBUNIT_CA              = 0x06, /**< Conditional access */
    AVC_SUBUNIT_CAMERA          = 0x07, /**< Camera */
    AVC_SUBUNIT_PANEL           = 0x09, /**< Panel */
    AVC_SUBUNIT_BULLETIN_BOARD  = 0x0A, /**< Bulletin board */
    AVC_SUBUNIT_CAMERA_STORAGE  = 0x0B, /**< Camera storage */
    AVC_SUBUNIT_VENDOR_UNIQUE   = 0x1C, /**< Vendor unique */
    AVC_SUBUNIT_UNIT            = 0x1F, /**< Unit (whole device) */
} AVC_SUBUNIT_TYPE;

/**
 * @brief AV/C Command Frame
 */
typedef struct _AVC_COMMAND_FRAME {
    UINT8   CommandType;                /**< Command/response type */
    UINT8   SubunitType;                /**< Subunit type and ID */
    UINT8   Opcode;                     /**< Operation code */
    UINT8   Operands[509];              /**< Operands (variable length) */
} AVC_COMMAND_FRAME;

//
// Forward declarations
//
DECLARE_INTERFACE_(IIOFireWireController, IIOService);
DECLARE_INTERFACE_(IIOFireWireBus, IIOService);
DECLARE_INTERFACE_(IIOFireWireDevice, IIOService);
DECLARE_INTERFACE_(IIOFireWireUnit, IIOService);
DECLARE_INTERFACE_(IIOFireWireIsochChannel, IIOService);

/**
 * @brief IIOFireWireController - FireWire Host Controller Interface
 *
 * Represents a FireWire (IEEE 1394) host controller, typically an OHCI-compliant
 * controller on a PCI/PCIe bus.
 */
#undef INTERFACE
#define INTERFACE IIOFireWireController

DECLARE_INTERFACE_(IIOFireWireController, IIOService)
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

    // IIOFireWireController methods

    /**
     * @brief Get controller information
     *
     * @param pInfo             Receives controller information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetControllerInfo)(THIS_
        FW_CONTROLLER_INFO *pInfo
        ) PURE;

    /**
     * @brief Reset the FireWire bus
     *
     * Initiates a bus reset, which causes all nodes to re-enumerate
     * and rebuild the topology.
     *
     * @retval IO_SUCCESS       Bus reset initiated
     * @retval IO_ERROR         Failed to reset bus
     */
    STDMETHOD_(IO_RETURN, ResetBus)(THIS) PURE;

    /**
     * @brief Get PHY information
     *
     * @param pPhyType          Receives PHY type
     * @param pCableType        Receives cable type (if detectable)
     * @param pMaxSpeed         Receives maximum speed
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetPhyInfo)(THIS_
        FW_PHY_TYPE *pPhyType,
        FW_CABLE_TYPE *pCableType,
        FW_SPEED *pMaxSpeed
        ) PURE;

    /**
     * @brief Set bus manager node
     *
     * @param NodeID            Node ID to become bus manager
     *
     * @retval IO_SUCCESS       Bus manager set
     */
    STDMETHOD_(IO_RETURN, SetBusManager)(THIS_
        UINT16 NodeID
        ) PURE;

    /**
     * @brief Allocate address range
     *
     * Allocates an address range in the local address space for
     * incoming requests.
     *
     * @param Offset            Desired offset (or 0 for any)
     * @param Length            Length in bytes
     * @param pAllocatedOffset  Receives allocated offset
     *
     * @retval IO_SUCCESS       Address allocated
     * @retval IO_NO_SPACE      Insufficient address space
     */
    STDMETHOD_(IO_RETURN, AllocateAddress)(THIS_
        UINT64 Offset,
        UINT64 Length,
        UINT64 *pAllocatedOffset
        ) PURE;

    /**
     * @brief Deallocate address range
     *
     * @param Offset            Offset to deallocate
     * @param Length            Length in bytes
     *
     * @retval IO_SUCCESS       Address deallocated
     */
    STDMETHOD_(IO_RETURN, DeallocateAddress)(THIS_
        UINT64 Offset,
        UINT64 Length
        ) PURE;

    /**
     * @brief Read quadlet (32-bit) from device
     *
     * Performs an asynchronous read quadlet transaction.
     *
     * @param NodeID            Target node ID
     * @param Offset            Offset within node's address space
     * @param pValue            Receives read value
     *
     * @retval IO_SUCCESS       Read successful
     * @retval IO_TIMEOUT       Transaction timed out
     * @retval IO_ERROR         Transaction failed
     */
    STDMETHOD_(IO_RETURN, ReadQuadlet)(THIS_
        UINT16 NodeID,
        UINT64 Offset,
        UINT32 *pValue
        ) PURE;

    /**
     * @brief Write quadlet (32-bit) to device
     *
     * Performs an asynchronous write quadlet transaction.
     *
     * @param NodeID            Target node ID
     * @param Offset            Offset within node's address space
     * @param Value             Value to write
     *
     * @retval IO_SUCCESS       Write successful
     * @retval IO_TIMEOUT       Transaction timed out
     * @retval IO_ERROR         Transaction failed
     */
    STDMETHOD_(IO_RETURN, WriteQuadlet)(THIS_
        UINT16 NodeID,
        UINT64 Offset,
        UINT32 Value
        ) PURE;

    /**
     * @brief Read block from device
     *
     * Performs an asynchronous read block transaction.
     *
     * @param NodeID            Target node ID
     * @param Offset            Offset within node's address space
     * @param pBuffer           Buffer to receive data
     * @param Length            Number of bytes to read
     *
     * @retval IO_SUCCESS       Read successful
     * @retval IO_TIMEOUT       Transaction timed out
     * @retval IO_ERROR         Transaction failed
     */
    STDMETHOD_(IO_RETURN, ReadBlock)(THIS_
        UINT16 NodeID,
        UINT64 Offset,
        VOID *pBuffer,
        UINT32 Length
        ) PURE;

    /**
     * @brief Write block to device
     *
     * Performs an asynchronous write block transaction.
     *
     * @param NodeID            Target node ID
     * @param Offset            Offset within node's address space
     * @param pBuffer           Buffer containing data to write
     * @param Length            Number of bytes to write
     *
     * @retval IO_SUCCESS       Write successful
     * @retval IO_TIMEOUT       Transaction timed out
     * @retval IO_ERROR         Transaction failed
     */
    STDMETHOD_(IO_RETURN, WriteBlock)(THIS_
        UINT16 NodeID,
        UINT64 Offset,
        CONST VOID *pBuffer,
        UINT32 Length
        ) PURE;

    /**
     * @brief Perform lock transaction (atomic operation)
     *
     * @param NodeID            Target node ID
     * @param Offset            Offset within node's address space
     * @param LockType          Type of lock operation
     * @param ArgValue          Argument value
     * @param DataValue         Data value
     * @param pOldValue         Receives old value
     *
     * @retval IO_SUCCESS       Lock successful
     * @retval IO_TIMEOUT       Transaction timed out
     * @retval IO_ERROR         Transaction failed
     */
    STDMETHOD_(IO_RETURN, Lock)(THIS_
        UINT16 NodeID,
        UINT64 Offset,
        FW_LOCK_TYPE LockType,
        UINT32 ArgValue,
        UINT32 DataValue,
        UINT32 *pOldValue
        ) PURE;

    /**
     * @brief Enable cycle master
     *
     * Enables or disables the cycle master function.
     *
     * @param bEnable           TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS       Cycle master state set
     */
    STDMETHOD_(IO_RETURN, EnableCycleMaster)(THIS_
        BOOLEAN bEnable
        ) PURE;
};

/**
 * @brief IIOFireWireBus - FireWire Bus Interface
 *
 * Represents the FireWire bus and provides bus-level operations such as
 * device enumeration and topology management.
 */
#undef INTERFACE
#define INTERFACE IIOFireWireBus

DECLARE_INTERFACE_(IIOFireWireBus, IIOService)
{
    // IUnknown, IIOService methods inherited...

    /**
     * @brief Enumerate devices on the bus
     *
     * @param ppDevices         Receives array of device interfaces
     * @param puCount           On input: max devices; On output: actual count
     *
     * @retval IO_SUCCESS       Enumeration successful
     */
    STDMETHOD_(IO_RETURN, EnumerateDevices)(THIS_
        IIOFireWireDevice **ppDevices,
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Get topology information
     *
     * @param pTopology         Receives topology state
     *
     * @retval IO_SUCCESS       Topology retrieved
     */
    STDMETHOD_(IO_RETURN, GetTopology)(THIS_
        FW_TOPOLOGY_STATE *pTopology
        ) PURE;

    /**
     * @brief Get root node ID
     *
     * @param pRootNodeID       Receives root node ID
     *
     * @retval IO_SUCCESS       Root node ID retrieved
     */
    STDMETHOD_(IO_RETURN, GetRootNode)(THIS_
        UINT16 *pRootNodeID
        ) PURE;

    /**
     * @brief Get local (host) node ID
     *
     * @param pLocalNodeID      Receives local node ID
     *
     * @retval IO_SUCCESS       Local node ID retrieved
     */
    STDMETHOD_(IO_RETURN, GetLocalNodeID)(THIS_
        UINT16 *pLocalNodeID
        ) PURE;

    /**
     * @brief Get bus generation number
     *
     * The generation number increments on each bus reset.
     *
     * @param pGeneration       Receives generation number
     *
     * @retval IO_SUCCESS       Generation retrieved
     */
    STDMETHOD_(IO_RETURN, GetBusGeneration)(THIS_
        UINT32 *pGeneration
        ) PURE;

    /**
     * @brief Wait for bus reset
     *
     * Blocks until the next bus reset occurs.
     *
     * @param Timeout           Timeout in milliseconds (0 = infinite)
     *
     * @retval IO_SUCCESS       Bus reset occurred
     * @retval IO_TIMEOUT       Timed out
     */
    STDMETHOD_(IO_RETURN, WaitForBusReset)(THIS_
        UINT32 Timeout
        ) PURE;
};

/**
 * @brief IIOFireWireDevice - FireWire Device Interface
 *
 * Represents a FireWire device on the bus.
 */
#undef INTERFACE
#define INTERFACE IIOFireWireDevice

DECLARE_INTERFACE_(IIOFireWireDevice, IIOService)
{
    // IUnknown, IIOService methods inherited...

    /**
     * @brief Get device information
     *
     * @param pInfo             Receives device information
     *
     * @retval IO_SUCCESS       Information retrieved
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        FW_DEVICE_INFO *pInfo
        ) PURE;

    /**
     * @brief Get configuration ROM
     *
     * Reads the device's configuration ROM.
     *
     * @param pBuffer           Buffer to receive ROM data
     * @param cbSize            Size of buffer
     * @param pcbActual         Receives actual ROM size
     *
     * @retval IO_SUCCESS       ROM read successfully
     */
    STDMETHOD_(IO_RETURN, GetROM)(THIS_
        VOID *pBuffer,
        UINTN cbSize,
        UINTN *pcbActual
        ) PURE;

    /**
     * @brief Get unit directory
     *
     * Retrieves a specific unit directory from the configuration ROM.
     *
     * @param UnitIndex         Unit index (0-based)
     * @param pUnitInfo         Receives unit information
     *
     * @retval IO_SUCCESS       Unit directory retrieved
     * @retval IO_NO_DEVICE     Unit index out of range
     */
    STDMETHOD_(IO_RETURN, GetUnitDirectory)(THIS_
        UINT32 UnitIndex,
        FW_UNIT_INFO *pUnitInfo
        ) PURE;

    /**
     * @brief Open a unit
     *
     * Opens a function unit within the device.
     *
     * @param UnitIndex         Unit index
     * @param ppUnit            Receives unit interface
     *
     * @retval IO_SUCCESS       Unit opened
     */
    STDMETHOD_(IO_RETURN, OpenUnit)(THIS_
        UINT32 UnitIndex,
        IIOFireWireUnit **ppUnit
        ) PURE;

    /**
     * @brief Get maximum supported speed
     *
     * @param pSpeed            Receives maximum speed
     *
     * @retval IO_SUCCESS       Speed retrieved
     */
    STDMETHOD_(IO_RETURN, GetSpeed)(THIS_
        FW_SPEED *pSpeed
        ) PURE;
};

/**
 * @brief IIOFireWireUnit - FireWire Function Unit Interface
 *
 * Represents a logical function unit within a FireWire device
 * (e.g., SBP-2 storage, AV/C camera).
 */
#undef INTERFACE
#define INTERFACE IIOFireWireUnit

DECLARE_INTERFACE_(IIOFireWireUnit, IIOService)
{
    // IUnknown, IIOService methods inherited...

    /**
     * @brief Get unit information
     *
     * @param pInfo             Receives unit information
     *
     * @retval IO_SUCCESS       Information retrieved
     */
    STDMETHOD_(IO_RETURN, GetUnitInfo)(THIS_
        FW_UNIT_INFO *pInfo
        ) PURE;

    /**
     * @brief Read from unit address space
     *
     * @param Offset            Offset within unit's address space
     * @param pBuffer           Buffer to receive data
     * @param Length            Number of bytes to read
     *
     * @retval IO_SUCCESS       Read successful
     */
    STDMETHOD_(IO_RETURN, Read)(THIS_
        UINT64 Offset,
        VOID *pBuffer,
        UINT32 Length
        ) PURE;

    /**
     * @brief Write to unit address space
     *
     * @param Offset            Offset within unit's address space
     * @param pBuffer           Buffer containing data
     * @param Length            Number of bytes to write
     *
     * @retval IO_SUCCESS       Write successful
     */
    STDMETHOD_(IO_RETURN, Write)(THIS_
        UINT64 Offset,
        CONST VOID *pBuffer,
        UINT32 Length
        ) PURE;

    /**
     * @brief Get protocol type
     *
     * @param pProtocol         Receives protocol type (SBP-2, AV/C, etc.)
     *
     * @retval IO_SUCCESS       Protocol retrieved
     */
    STDMETHOD_(IO_RETURN, GetProtocol)(THIS_
        FW_DEVICE_CLASS *pProtocol
        ) PURE;
};

/**
 * @brief IIOFireWireIsochChannel - Isochronous Channel Interface
 *
 * Represents an isochronous channel for real-time data transfer
 * (audio, video streaming).
 */
#undef INTERFACE
#define INTERFACE IIOFireWireIsochChannel

DECLARE_INTERFACE_(IIOFireWireIsochChannel, IIOService)
{
    // IUnknown, IIOService methods inherited...

    /**
     * @brief Allocate isochronous channel
     *
     * @param ChannelNumber     Desired channel (0-63, or 0xFF for any)
     * @param Bandwidth         Required bandwidth (allocation units)
     * @param pAllocatedChannel Receives allocated channel number
     *
     * @retval IO_SUCCESS       Channel allocated
     * @retval IO_NO_BANDWIDTH  Insufficient bandwidth
     */
    STDMETHOD_(IO_RETURN, AllocateChannel)(THIS_
        UINT8 ChannelNumber,
        UINT32 Bandwidth,
        UINT8 *pAllocatedChannel
        ) PURE;

    /**
     * @brief Deallocate isochronous channel
     *
     * @param ChannelNumber     Channel to deallocate
     *
     * @retval IO_SUCCESS       Channel deallocated
     */
    STDMETHOD_(IO_RETURN, DeallocateChannel)(THIS_
        UINT8 ChannelNumber
        ) PURE;

    /**
     * @brief Allocate bandwidth
     *
     * @param Bandwidth         Bandwidth to allocate (allocation units)
     *
     * @retval IO_SUCCESS       Bandwidth allocated
     * @retval IO_NO_BANDWIDTH  Insufficient bandwidth
     */
    STDMETHOD_(IO_RETURN, AllocateBandwidth)(THIS_
        UINT32 Bandwidth
        ) PURE;

    /**
     * @brief Start transmit
     *
     * @param ChannelNumber     Channel to transmit on
     * @param pBuffer           Buffer containing data
     * @param Length            Number of bytes
     *
     * @retval IO_SUCCESS       Transmit started
     */
    STDMETHOD_(IO_RETURN, StartTransmit)(THIS_
        UINT8 ChannelNumber,
        CONST VOID *pBuffer,
        UINT32 Length
        ) PURE;

    /**
     * @brief Stop transmit
     *
     * @param ChannelNumber     Channel to stop
     *
     * @retval IO_SUCCESS       Transmit stopped
     */
    STDMETHOD_(IO_RETURN, StopTransmit)(THIS_
        UINT8 ChannelNumber
        ) PURE;

    /**
     * @brief Start receive
     *
     * @param ChannelNumber     Channel to receive from
     * @param pBuffer           Buffer to receive data
     * @param Length            Buffer length
     *
     * @retval IO_SUCCESS       Receive started
     */
    STDMETHOD_(IO_RETURN, StartReceive)(THIS_
        UINT8 ChannelNumber,
        VOID *pBuffer,
        UINT32 Length
        ) PURE;

    /**
     * @brief Stop receive
     *
     * @param ChannelNumber     Channel to stop
     *
     * @retval IO_SUCCESS       Receive stopped
     */
    STDMETHOD_(IO_RETURN, StopReceive)(THIS_
        UINT8 ChannelNumber
        ) PURE;
};

//
// Convenience macros for calling interface methods
//

#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOFireWireController_GetControllerInfo(p,a)        (p)->lpVtbl->GetControllerInfo(p,a)
#define IIOFireWireController_ResetBus(p)                   (p)->lpVtbl->ResetBus(p)
#define IIOFireWireController_GetPhyInfo(p,a,b,c)           (p)->lpVtbl->GetPhyInfo(p,a,b,c)
#define IIOFireWireController_SetBusManager(p,a)            (p)->lpVtbl->SetBusManager(p,a)
#define IIOFireWireController_AllocateAddress(p,a,b,c)      (p)->lpVtbl->AllocateAddress(p,a,b,c)
#define IIOFireWireController_DeallocateAddress(p,a,b)      (p)->lpVtbl->DeallocateAddress(p,a,b)
#define IIOFireWireController_ReadQuadlet(p,a,b,c)          (p)->lpVtbl->ReadQuadlet(p,a,b,c)
#define IIOFireWireController_WriteQuadlet(p,a,b,c)         (p)->lpVtbl->WriteQuadlet(p,a,b,c)
#define IIOFireWireController_ReadBlock(p,a,b,c,d)          (p)->lpVtbl->ReadBlock(p,a,b,c,d)
#define IIOFireWireController_WriteBlock(p,a,b,c,d)         (p)->lpVtbl->WriteBlock(p,a,b,c,d)
#define IIOFireWireController_Lock(p,a,b,c,d,e,f)           (p)->lpVtbl->Lock(p,a,b,c,d,e,f)
#define IIOFireWireController_EnableCycleMaster(p,a)        (p)->lpVtbl->EnableCycleMaster(p,a)

#define IIOFireWireBus_EnumerateDevices(p,a,b)              (p)->lpVtbl->EnumerateDevices(p,a,b)
#define IIOFireWireBus_GetTopology(p,a)                     (p)->lpVtbl->GetTopology(p,a)
#define IIOFireWireBus_GetRootNode(p,a)                     (p)->lpVtbl->GetRootNode(p,a)
#define IIOFireWireBus_GetLocalNodeID(p,a)                  (p)->lpVtbl->GetLocalNodeID(p,a)
#define IIOFireWireBus_GetBusGeneration(p,a)                (p)->lpVtbl->GetBusGeneration(p,a)
#define IIOFireWireBus_WaitForBusReset(p,a)                 (p)->lpVtbl->WaitForBusReset(p,a)

#define IIOFireWireDevice_GetDeviceInfo(p,a)                (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOFireWireDevice_GetROM(p,a,b,c)                   (p)->lpVtbl->GetROM(p,a,b,c)
#define IIOFireWireDevice_GetUnitDirectory(p,a,b)           (p)->lpVtbl->GetUnitDirectory(p,a,b)
#define IIOFireWireDevice_OpenUnit(p,a,b)                   (p)->lpVtbl->OpenUnit(p,a,b)
#define IIOFireWireDevice_GetSpeed(p,a)                     (p)->lpVtbl->GetSpeed(p,a)

#define IIOFireWireUnit_GetUnitInfo(p,a)                    (p)->lpVtbl->GetUnitInfo(p,a)
#define IIOFireWireUnit_Read(p,a,b,c)                       (p)->lpVtbl->Read(p,a,b,c)
#define IIOFireWireUnit_Write(p,a,b,c)                      (p)->lpVtbl->Write(p,a,b,c)
#define IIOFireWireUnit_GetProtocol(p,a)                    (p)->lpVtbl->GetProtocol(p,a)

#define IIOFireWireIsochChannel_AllocateChannel(p,a,b,c)    (p)->lpVtbl->AllocateChannel(p,a,b,c)
#define IIOFireWireIsochChannel_DeallocateChannel(p,a)      (p)->lpVtbl->DeallocateChannel(p,a)
#define IIOFireWireIsochChannel_AllocateBandwidth(p,a)      (p)->lpVtbl->AllocateBandwidth(p,a)
#define IIOFireWireIsochChannel_StartTransmit(p,a,b,c)      (p)->lpVtbl->StartTransmit(p,a,b,c)
#define IIOFireWireIsochChannel_StopTransmit(p,a)           (p)->lpVtbl->StopTransmit(p,a)
#define IIOFireWireIsochChannel_StartReceive(p,a,b,c)       (p)->lpVtbl->StartReceive(p,a,b,c)
#define IIOFireWireIsochChannel_StopReceive(p,a)            (p)->lpVtbl->StopReceive(p,a)

#endif

/**
 * @brief Initialize FireWire subsystem
 *
 * @retval IO_SUCCESS   Subsystem initialized successfully
 */
IO_RETURN
FireWireInitialize(
    VOID
    );

/**
 * @brief Shutdown FireWire subsystem
 *
 * @retval IO_SUCCESS   Subsystem shut down successfully
 */
IO_RETURN
FireWireShutdown(
    VOID
    );

/**
 * @brief Create a FireWire controller instance
 *
 * @param pszName           Controller name
 * @param ppController      Receives controller interface
 *
 * @retval IO_SUCCESS           Controller created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid parameter
 */
IO_RETURN
IOFireWireControllerCreate(
    CONST CHAR8              *pszName,
    IIOFireWireController  **ppController
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_FIREWIRE_H */
