/**
 * @file rdma.h
 * @brief RDMA Family Interface - Remote Direct Memory Access and InfiniBand
 *
 * This header defines the RDMA family interface for high-performance interconnects
 * including InfiniBand, RoCE, iWARP, and OmniPath. Provides comprehensive support
 * for RDMA verbs, queue pairs, memory registration, and all RDMA operations.
 *
 * Supports:
 * - InfiniBand (SDR/DDR/QDR/FDR/EDR/HDR/NDR/XDR - 2.5 to 250 Gbps)
 * - RoCE v1/v2 (RDMA over Converged Ethernet)
 * - iWARP (RDMA over TCP/IP)
 * - Intel OmniPath Architecture
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_RDMA_H
#define IOKIT_RDMA_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIORDMADevice interface GUID
 * {C7D8E9F0-1A2B-3C4D-5E6F-7A8B9C0D1E2F}
 */
DEFINE_GUID(IID_IIORDMADevice,
    0xC7D8E9F0, 0x1A2B, 0x3C4D, 0x5E, 0x6F, 0x7A, 0x8B, 0x9C, 0x0D, 0x1E, 0x2F);

/**
 * @brief IIORDMAConnection interface GUID
 * {D8E9F0A1-2B3C-4D5E-6F7A-8B9C0D1E2F30}
 */
DEFINE_GUID(IID_IIORDMAConnection,
    0xD8E9F0A1, 0x2B3C, 0x4D5E, 0x6F, 0x7A, 0x8B, 0x9C, 0x0D, 0x1E, 0x2F, 0x30);

/**
 * @brief IIORDMAMemoryRegion interface GUID
 * {E9F0A1B2-3C4D-5E6F-7A8B-9C0D1E2F3041}
 */
DEFINE_GUID(IID_IIORDMAMemoryRegion,
    0xE9F0A1B2, 0x3C4D, 0x5E6F, 0x7A, 0x8B, 0x9C, 0x0D, 0x1E, 0x2F, 0x30, 0x41);

/*============================================================================
 * RDMA Transport Types
 *============================================================================*/

/**
 * @brief RDMA transport type
 */
typedef enum _RDMA_TRANSPORT_TYPE {
    RDMA_TRANSPORT_IB           = 0,    /**< InfiniBand native transport */
    RDMA_TRANSPORT_ROCE_V1      = 1,    /**< RoCE version 1 (L2 only) */
    RDMA_TRANSPORT_ROCE_V2      = 2,    /**< RoCE version 2 (UDP/IP) */
    RDMA_TRANSPORT_IWARP        = 3,    /**< iWARP (RDMA over TCP/IP) */
    RDMA_TRANSPORT_OMNIPATH     = 4,    /**< Intel OmniPath Architecture */
    RDMA_TRANSPORT_UNKNOWN      = 0xFF
} RDMA_TRANSPORT_TYPE;

/*============================================================================
 * InfiniBand Link Speeds and Widths
 *============================================================================*/

/**
 * @brief InfiniBand link speed
 */
typedef enum _IB_LINK_SPEED {
    IB_SPEED_SDR    = 0x01,     /**< Single Data Rate - 2.5 Gbps */
    IB_SPEED_DDR    = 0x02,     /**< Double Data Rate - 5 Gbps */
    IB_SPEED_QDR    = 0x04,     /**< Quad Data Rate - 10 Gbps */
    IB_SPEED_FDR10  = 0x08,     /**< Fourteen Data Rate 10 - 10.3125 Gbps */
    IB_SPEED_FDR    = 0x10,     /**< Fourteen Data Rate - 14.0625 Gbps */
    IB_SPEED_EDR    = 0x20,     /**< Enhanced Data Rate - 25.78125 Gbps */
    IB_SPEED_HDR    = 0x40,     /**< High Data Rate - 50 Gbps */
    IB_SPEED_NDR    = 0x80,     /**< Next Data Rate - 100 Gbps */
    IB_SPEED_XDR    = 0x100,    /**< eXtended Data Rate - 250 Gbps */
} IB_LINK_SPEED;

/**
 * @brief InfiniBand link width
 */
typedef enum _IB_LINK_WIDTH {
    IB_WIDTH_1X     = 0x01,     /**< 1x link (1 lane) */
    IB_WIDTH_4X     = 0x02,     /**< 4x link (4 lanes) */
    IB_WIDTH_8X     = 0x04,     /**< 8x link (8 lanes) */
    IB_WIDTH_12X    = 0x08,     /**< 12x link (12 lanes) */
} IB_LINK_WIDTH;

/**
 * @brief InfiniBand port state
 */
typedef enum _IB_PORT_STATE {
    IB_PORT_DOWN        = 1,    /**< Port is down */
    IB_PORT_INIT        = 2,    /**< Port is initializing */
    IB_PORT_ARMED       = 3,    /**< Port is armed */
    IB_PORT_ACTIVE      = 4,    /**< Port is active */
    IB_PORT_ACTIVE_DEFER = 5    /**< Port active with deferred link training */
} IB_PORT_STATE;

/**
 * @brief InfiniBand MTU size
 */
typedef enum _IB_MTU {
    IB_MTU_256      = 1,        /**< 256 bytes */
    IB_MTU_512      = 2,        /**< 512 bytes */
    IB_MTU_1024     = 3,        /**< 1024 bytes */
    IB_MTU_2048     = 4,        /**< 2048 bytes */
    IB_MTU_4096     = 5,        /**< 4096 bytes */
} IB_MTU;

/*============================================================================
 * RDMA Queue Pair Types and States
 *============================================================================*/

/**
 * @brief Queue Pair (QP) type
 */
typedef enum _RDMA_QP_TYPE {
    RDMA_QP_RC      = 0,        /**< Reliable Connected */
    RDMA_QP_UC      = 1,        /**< Unreliable Connected */
    RDMA_QP_UD      = 2,        /**< Unreliable Datagram */
    RDMA_QP_RAW     = 3,        /**< Raw packet */
    RDMA_QP_XRC_SEND = 4,       /**< eXtended Reliable Connection (send) */
    RDMA_QP_XRC_RECV = 5,       /**< eXtended Reliable Connection (recv) */
} RDMA_QP_TYPE;

/**
 * @brief Queue Pair state
 */
typedef enum _RDMA_QP_STATE {
    RDMA_QPS_RESET  = 0,        /**< Reset state */
    RDMA_QPS_INIT   = 1,        /**< Initialized state */
    RDMA_QPS_RTR    = 2,        /**< Ready To Receive */
    RDMA_QPS_RTS    = 3,        /**< Ready To Send */
    RDMA_QPS_SQD    = 4,        /**< Send Queue Drained */
    RDMA_QPS_SQE    = 5,        /**< Send Queue Error */
    RDMA_QPS_ERR    = 6,        /**< Error state */
} RDMA_QP_STATE;

/*============================================================================
 * RDMA Work Request Opcodes
 *============================================================================*/

/**
 * @brief Work Request opcode
 */
typedef enum _RDMA_WR_OPCODE {
    RDMA_WR_SEND            = 0,    /**< Send operation */
    RDMA_WR_SEND_WITH_IMM   = 1,    /**< Send with immediate data */
    RDMA_WR_RDMA_WRITE      = 2,    /**< RDMA Write */
    RDMA_WR_RDMA_WRITE_WITH_IMM = 3, /**< RDMA Write with immediate */
    RDMA_WR_RDMA_READ       = 4,    /**< RDMA Read */
    RDMA_WR_ATOMIC_CMP_AND_SWP = 5, /**< Atomic Compare and Swap */
    RDMA_WR_ATOMIC_FETCH_AND_ADD = 6, /**< Atomic Fetch and Add */
    RDMA_WR_SEND_WITH_INV   = 7,    /**< Send with invalidate */
    RDMA_WR_LOCAL_INV       = 8,    /**< Local invalidate */
    RDMA_WR_FAST_REG_MR     = 9,    /**< Fast register memory region */
    RDMA_WR_BIND_MW         = 10,   /**< Bind memory window */
    RDMA_WR_RECV            = 128,  /**< Receive (posted to RQ) */
} RDMA_WR_OPCODE;

/**
 * @brief Work Completion status
 */
typedef enum _RDMA_WC_STATUS {
    RDMA_WC_SUCCESS         = 0,    /**< Operation completed successfully */
    RDMA_WC_LOC_LEN_ERR     = 1,    /**< Local length error */
    RDMA_WC_LOC_QP_OP_ERR   = 2,    /**< Local QP operation error */
    RDMA_WC_LOC_EEC_OP_ERR  = 3,    /**< Local EE context operation error */
    RDMA_WC_LOC_PROT_ERR    = 4,    /**< Local protection error */
    RDMA_WC_WR_FLUSH_ERR    = 5,    /**< Work request flushed error */
    RDMA_WC_MW_BIND_ERR     = 6,    /**< Memory window bind error */
    RDMA_WC_BAD_RESP_ERR    = 7,    /**< Bad response error */
    RDMA_WC_LOC_ACCESS_ERR  = 8,    /**< Local access error */
    RDMA_WC_REM_INV_REQ_ERR = 9,    /**< Remote invalid request error */
    RDMA_WC_REM_ACCESS_ERR  = 10,   /**< Remote access error */
    RDMA_WC_REM_OP_ERR      = 11,   /**< Remote operation error */
    RDMA_WC_RETRY_EXC_ERR   = 12,   /**< Retry counter exceeded */
    RDMA_WC_RNR_RETRY_EXC_ERR = 13, /**< RNR retry counter exceeded */
    RDMA_WC_LOC_RDD_VIOL_ERR = 14,  /**< Local RDD violation error */
    RDMA_WC_REM_INV_RD_REQ_ERR = 15, /**< Remote invalid RD request */
    RDMA_WC_REM_ABORT_ERR   = 16,   /**< Remote aborted error */
    RDMA_WC_INV_EECN_ERR    = 17,   /**< Invalid EE context number */
    RDMA_WC_INV_EEC_STATE_ERR = 18, /**< Invalid EE context state */
    RDMA_WC_FATAL_ERR       = 19,   /**< Fatal error */
    RDMA_WC_RESP_TIMEOUT_ERR = 20,  /**< Response timeout error */
    RDMA_WC_GENERAL_ERR     = 21,   /**< General error */
} RDMA_WC_STATUS;

/*============================================================================
 * RDMA Memory Access Flags
 *============================================================================*/

/**
 * @brief Memory access flags for MR/MW
 */
#define RDMA_ACCESS_LOCAL_WRITE     (1 << 0)    /**< Local write access */
#define RDMA_ACCESS_REMOTE_WRITE    (1 << 1)    /**< Remote write access */
#define RDMA_ACCESS_REMOTE_READ     (1 << 2)    /**< Remote read access */
#define RDMA_ACCESS_REMOTE_ATOMIC   (1 << 3)    /**< Remote atomic access */
#define RDMA_ACCESS_MW_BIND         (1 << 4)    /**< Memory window bind */
#define RDMA_ACCESS_ZERO_BASED      (1 << 5)    /**< Zero-based virtual address */
#define RDMA_ACCESS_ON_DEMAND       (1 << 6)    /**< On-demand paging */

/**
 * @brief Send flags for work requests
 */
#define RDMA_SEND_FENCE             (1 << 0)    /**< Fence operation */
#define RDMA_SEND_SIGNALED          (1 << 1)    /**< Generate completion */
#define RDMA_SEND_SOLICITED         (1 << 2)    /**< Solicited event */
#define RDMA_SEND_INLINE            (1 << 3)    /**< Inline data */
#define RDMA_SEND_IP_CSUM           (1 << 4)    /**< IP checksum offload */

/*============================================================================
 * RDMA Device Capabilities
 *============================================================================*/

/**
 * @brief Device capability flags
 */
#define RDMA_CAP_RESIZE_MAX_WR          (1ULL << 0)     /**< Resize max WR */
#define RDMA_CAP_BAD_PKEY_CNTR          (1ULL << 1)     /**< Bad P_Key counter */
#define RDMA_CAP_BAD_QKEY_CNTR          (1ULL << 2)     /**< Bad Q_Key counter */
#define RDMA_CAP_RAW_MULTI              (1ULL << 3)     /**< Raw multicast */
#define RDMA_CAP_AUTO_PATH_MIG          (1ULL << 4)     /**< Automatic path migration */
#define RDMA_CAP_CHANGE_PHY_PORT        (1ULL << 5)     /**< Change physical port */
#define RDMA_CAP_UD_AV_PORT_ENFORCE     (1ULL << 6)     /**< UD AV port enforce */
#define RDMA_CAP_CURR_QP_STATE_MOD      (1ULL << 7)     /**< Current QP state modify */
#define RDMA_CAP_SHUTDOWN_PORT          (1ULL << 8)     /**< Shutdown port */
#define RDMA_CAP_INIT_TYPE              (1ULL << 9)     /**< Init type */
#define RDMA_CAP_PORT_ACTIVE_EVENT      (1ULL << 10)    /**< Port active event */
#define RDMA_CAP_SYS_IMAGE_GUID         (1ULL << 11)    /**< System image GUID */
#define RDMA_CAP_RC_RNR_NAK_GEN         (1ULL << 12)    /**< RC RNR NAK generation */
#define RDMA_CAP_SRQ_RESIZE             (1ULL << 13)    /**< SRQ resize */
#define RDMA_CAP_N_NOTIFY_CQ            (1ULL << 14)    /**< N notify CQ */
#define RDMA_CAP_MEM_WINDOW             (1ULL << 15)    /**< Memory windows */
#define RDMA_CAP_UD_IP_CSUM             (1ULL << 16)    /**< UD IP checksum */
#define RDMA_CAP_UD_TSO                 (1ULL << 17)    /**< UD TCP segmentation */
#define RDMA_CAP_XRC                    (1ULL << 18)    /**< XRC support */
#define RDMA_CAP_MEM_MGMT_EXTENSIONS    (1ULL << 19)    /**< Memory management extensions */
#define RDMA_CAP_BLOCK_MULTICAST_LOOPBACK (1ULL << 20) /**< Block multicast loopback */
#define RDMA_CAP_MEM_WINDOW_TYPE_2A     (1ULL << 21)    /**< Memory window type 2A */
#define RDMA_CAP_MEM_WINDOW_TYPE_2B     (1ULL << 22)    /**< Memory window type 2B */
#define RDMA_CAP_RC_IP_CSUM             (1ULL << 23)    /**< RC IP checksum */
#define RDMA_CAP_RAW_IP_CSUM            (1ULL << 24)    /**< Raw IP checksum */
#define RDMA_CAP_MANAGED_FLOW_STEERING  (1ULL << 25)    /**< Managed flow steering */
#define RDMA_CAP_SIGNATURE_HANDOVER     (1ULL << 26)    /**< Signature handover */
#define RDMA_CAP_ON_DEMAND_PAGING       (1ULL << 27)    /**< On-demand paging */

/*============================================================================
 * InfiniBand Addressing
 *============================================================================*/

/**
 * @brief InfiniBand GID (Global Identifier) - 128-bit
 */
typedef struct _RDMA_GID {
    union {
        UINT8   Raw[16];            /**< Raw bytes */
        struct {
            UINT64  SubnetPrefix;   /**< Subnet prefix (64-bit) */
            UINT64  InterfaceID;    /**< Interface ID (64-bit) */
        };
    };
} RDMA_GID;

/**
 * @brief InfiniBand addressing
 */
typedef struct _IB_ADDRESS {
    UINT16      LID;                /**< Local Identifier (16-bit) */
    RDMA_GID    GID;                /**< Global Identifier (128-bit) */
    UINT8       ServiceLevel;       /**< Service Level (0-15) */
    UINT8       SourcePathBits;     /**< Source path bits */
} IB_ADDRESS;

/*============================================================================
 * RDMA Scatter-Gather Element
 *============================================================================*/

/**
 * @brief Scatter-Gather Element
 */
typedef struct _RDMA_SGE {
    UINT64      Address;            /**< Virtual address */
    UINT32      Length;             /**< Length in bytes */
    UINT32      LKey;               /**< Local key */
} RDMA_SGE;

/*============================================================================
 * RDMA Work Request
 *============================================================================*/

/**
 * @brief Work Request
 */
typedef struct _RDMA_WR {
    UINT64              WorkRequestID;      /**< User-defined WR ID */
    RDMA_WR_OPCODE      Opcode;             /**< Operation code */
    UINT32              Flags;              /**< Send flags */
    UINT32              ImmediateData;      /**< Immediate data (if applicable) */

    // Scatter-Gather list
    RDMA_SGE           *pSGList;            /**< Scatter-gather list */
    UINT32              NumSGE;             /**< Number of SGEs */

    // RDMA-specific fields
    UINT64              RemoteAddress;      /**< Remote address (RDMA ops) */
    UINT32              RemoteKey;          /**< Remote key (RDMA ops) */

    // Atomic-specific fields
    UINT64              CompareAdd;         /**< Compare operand or add value */
    UINT64              Swap;               /**< Swap operand */

    struct _RDMA_WR    *pNext;              /**< Next WR in chain */
} RDMA_WR;

/*============================================================================
 * RDMA Work Completion
 *============================================================================*/

/**
 * @brief Work Completion
 */
typedef struct _RDMA_WC {
    UINT64              WorkRequestID;      /**< Work request ID */
    RDMA_WC_STATUS      Status;             /**< Completion status */
    RDMA_WR_OPCODE      Opcode;             /**< Operation that completed */
    UINT32              ByteLen;            /**< Bytes transferred */
    UINT32              ImmediateData;      /**< Immediate data (if present) */
    UINT32              QueuePairNum;       /**< Remote QP number (UD) */
    UINT32              SourceQP;           /**< Source QP (UD receive) */
    UINT16              SourceLID;          /**< Source LID (UD receive) */
    UINT8               ServiceLevel;       /**< Service level */
    UINT8               PortNum;            /**< Port number */
    UINT32              VendorError;        /**< Vendor-specific error code */
} RDMA_WC;

/*============================================================================
 * RDMA Device and Port Information
 *============================================================================*/

/**
 * @brief RDMA device information
 */
typedef struct _RDMA_DEVICE_INFO {
    CHAR8                   DeviceName[64];     /**< Device name */
    CHAR8                   VendorName[64];     /**< Vendor name */
    CHAR8                   ModelName[128];     /**< Model name */
    UINT64                  NodeGUID;           /**< Node GUID */
    UINT64                  SystemImageGUID;    /**< System image GUID */
    RDMA_TRANSPORT_TYPE     TransportType;      /**< Transport type */

    // Hardware revision
    UINT16                  VendorID;           /**< PCI vendor ID */
    UINT16                  DeviceID;           /**< PCI device ID */
    UINT32                  HWVersion;          /**< Hardware version */
    UINT32                  FWVersion;          /**< Firmware version */

    // Port configuration
    UINT8                   PhysPortCount;      /**< Number of physical ports */

    // Device capabilities
    UINT64                  DeviceCapFlags;     /**< Device capability flags */
    UINT32                  MaxQP;              /**< Maximum queue pairs */
    UINT32                  MaxQPWR;            /**< Maximum WR per QP */
    UINT32                  MaxSGE;             /**< Maximum SGE per WR */
    UINT32                  MaxSGERD;           /**< Maximum SGE for RDMA read */
    UINT32                  MaxCQ;              /**< Maximum completion queues */
    UINT32                  MaxCQE;             /**< Maximum CQE per CQ */
    UINT32                  MaxMR;              /**< Maximum memory regions */
    UINT32                  MaxPD;              /**< Maximum protection domains */
    UINT32                  MaxQPRDAtom;        /**< Max outstanding RDMA read/atomic */
    UINT32                  MaxEERDAtom;        /**< Max EE outstanding RDMA read/atomic */
    UINT32                  MaxResRDAtom;       /**< Max resources for RDMA read/atomic */
    UINT32                  MaxQPInitRDAtom;    /**< Max QP initiator RDMA read/atomic */
    UINT32                  MaxEEInitRDAtom;    /**< Max EE initiator RDMA read/atomic */
    UINT32                  MaxEE;              /**< Maximum EE contexts */
    UINT32                  MaxRDD;             /**< Maximum RD domains */
    UINT32                  MaxMW;              /**< Maximum memory windows */
    UINT32                  MaxMulticast;       /**< Maximum multicast groups */
    UINT32                  MaxMCastQPAttach;   /**< Max QPs per multicast group */
    UINT32                  MaxTotalMCastQPAttach; /**< Max total multicast QP attaches */
    UINT32                  MaxAH;              /**< Maximum address handles */
    UINT32                  MaxFMR;             /**< Maximum FMRs */
    UINT32                  MaxMapPerFMR;       /**< Max map operations per FMR */
    UINT32                  MaxSRQ;             /**< Maximum SRQs */
    UINT32                  MaxSRQWR;           /**< Maximum WR per SRQ */
    UINT32                  MaxSRQSGE;          /**< Maximum SGE per SRQ WR */
    UINT32                  MaxPKeys;           /**< Maximum P_Keys */
    UINT32                  LocalCAACKDelay;    /**< Local CA ACK delay */
    UINT32                  PageSizeCapability; /**< Page size capability mask */
    UINT32                  AtomicCapability;   /**< Atomic operation capability */
} RDMA_DEVICE_INFO;

/**
 * @brief RDMA port information
 */
typedef struct _RDMA_PORT_INFO {
    IB_PORT_STATE       State;              /**< Port state */
    IB_MTU              MaxMTU;             /**< Maximum MTU */
    IB_MTU              ActiveMTU;          /**< Active MTU */
    UINT32              GIDTableLen;        /**< GID table length */
    UINT32              MaxMsgSize;         /**< Maximum message size */
    UINT32              BadPKeyCounter;     /**< Bad P_Key counter */
    UINT32              QKeyViolCounter;    /**< Q_Key violation counter */
    UINT16              PKeyTableLen;       /**< P_Key table length */
    UINT16              LID;                /**< Local Identifier */
    UINT16              SubnetMgrLID;       /**< Subnet Manager LID */
    UINT8               SubnetMgrSL;        /**< Subnet Manager SL */
    UINT8               SubnetTimeout;      /**< Subnet timeout */

    // Link properties
    IB_LINK_SPEED       ActiveSpeed;        /**< Active link speed */
    IB_LINK_WIDTH       ActiveWidth;        /**< Active link width */
    UINT8               PhysicalState;      /**< Physical state */

    // Capabilities
    BOOLEAN             bIsSubnetMgr;       /**< Is subnet manager */
    BOOLEAN             bIsSubnetMgrDisabled; /**< Subnet manager disabled */

    RDMA_GID            GID;                /**< Port GID */
} RDMA_PORT_INFO;

/**
 * @brief Queue Pair information
 */
typedef struct _RDMA_QP_INFO {
    UINT32              QPNum;              /**< Queue pair number */
    RDMA_QP_TYPE        Type;               /**< Queue pair type */
    RDMA_QP_STATE       State;              /**< Current state */
    UINT32              SendQueueDepth;     /**< Send queue depth */
    UINT32              RecvQueueDepth;     /**< Receive queue depth */
    UINT32              MaxInlineSend;      /**< Maximum inline send size */
    UINT32              MaxSendSGE;         /**< Maximum send SGEs */
    UINT32              MaxRecvSGE;         /**< Maximum receive SGEs */
} RDMA_QP_INFO;

/**
 * @brief Completion Queue information
 */
typedef struct _RDMA_CQ_INFO {
    UINT32              CQNum;              /**< CQ number */
    UINT32              NumCQE;             /**< Number of CQEs */
    UINT32              Vector;             /**< Interrupt vector */
} RDMA_CQ_INFO;

/**
 * @brief Memory Region information
 */
typedef struct _RDMA_MR_INFO {
    UINT64              Address;            /**< Virtual address */
    UINT64              Length;             /**< Length in bytes */
    UINT32              LKey;               /**< Local key */
    UINT32              RKey;               /**< Remote key */
    UINT32              AccessFlags;        /**< Access permissions */
    UINT32              PDHandle;           /**< Protection domain handle */
} RDMA_MR_INFO;

/*============================================================================
 * InfiniBand Management Structures
 *============================================================================*/

/**
 * @brief InfiniBand Management Datagram (MAD)
 */
typedef struct _IB_MAD {
    UINT8   BaseVersion;        /**< Base version */
    UINT8   MgmtClass;          /**< Management class */
    UINT8   ClassVersion;       /**< Class version */
    UINT8   Method;             /**< Method */
    UINT16  Status;             /**< Status */
    UINT16  ClassSpecific;      /**< Class-specific */
    UINT64  TransactionID;      /**< Transaction ID */
    UINT16  AttributeID;        /**< Attribute ID */
    UINT16  Reserved;           /**< Reserved */
    UINT32  AttributeModifier;  /**< Attribute modifier */
    UINT8   Data[232];          /**< MAD data */
} IB_MAD;

/**
 * @brief InfiniBand management classes
 */
#define IB_MGMT_CLASS_SUBN_LID_ROUTED   0x01    /**< Subnet management (LID-routed) */
#define IB_MGMT_CLASS_SUBN_DIRECTED     0x81    /**< Subnet management (directed) */
#define IB_MGMT_CLASS_SUBN_ADM          0x03    /**< Subnet administration */
#define IB_MGMT_CLASS_PERF_MGMT         0x04    /**< Performance management */
#define IB_MGMT_CLASS_BM                0x05    /**< Baseboard management */
#define IB_MGMT_CLASS_DEVICE_MGMT       0x06    /**< Device management */
#define IB_MGMT_CLASS_CM                0x07    /**< Communication management */
#define IB_MGMT_CLASS_SNMP              0x08    /**< SNMP */
#define IB_MGMT_CLASS_VENDOR_RANGE1     0x09    /**< Vendor-specific (range 1) */
#define IB_MGMT_CLASS_VENDOR_RANGE2     0x0F    /**< Vendor-specific (range 2) */

/**
 * @brief Global Routing Header (GRH)
 */
typedef struct _IB_GRH {
    UINT32      IPVersion:4;        /**< IP version (6) */
    UINT32      TrafficClass:8;     /**< Traffic class */
    UINT32      FlowLabel:20;       /**< Flow label */
    UINT16      PayloadLength;      /**< Payload length */
    UINT8       NextHeader;         /**< Next header */
    UINT8       HopLimit;           /**< Hop limit */
    RDMA_GID    SourceGID;          /**< Source GID */
    RDMA_GID    DestGID;            /**< Destination GID */
} IB_GRH;

/**
 * @brief Base Transport Header (BTH)
 */
typedef struct _IB_BTH {
    UINT8       OpCode;             /**< Opcode */
    UINT8       Flags;              /**< Flags (SE, M, Pad, Ver) */
    UINT16      PartitionKey;       /**< Partition key */
    UINT32      DestQP:24;          /**< Destination QP */
    UINT32      AckReq:1;           /**< Acknowledgment request */
    UINT32      Reserved:7;         /**< Reserved */
    UINT32      PSN:24;             /**< Packet sequence number */
} IB_BTH;

/*============================================================================
 * Vendor-Specific Definitions
 *============================================================================*/

/**
 * @brief Known RDMA vendors
 */
#define RDMA_VENDOR_MELLANOX        0x15B3  /**< Mellanox/NVIDIA */
#define RDMA_VENDOR_INTEL           0x8086  /**< Intel */
#define RDMA_VENDOR_CHELSIO         0x1425  /**< Chelsio */
#define RDMA_VENDOR_BROADCOM        0x14E4  /**< Broadcom */
#define RDMA_VENDOR_QLOGIC          0x1077  /**< QLogic (Cavium) */
#define RDMA_VENDOR_CISCO           0x1137  /**< Cisco */

/*============================================================================
 * Interface Definitions
 *============================================================================*/

/**
 * @brief IIORDMADevice - RDMA Device/HCA Interface
 *
 * This interface represents an RDMA-capable device (HCA, RNIC, etc.) and
 * provides methods for device management, queue pair creation, memory
 * registration, and RDMA operations.
 */
#undef INTERFACE
#define INTERFACE IIORDMADevice

DECLARE_INTERFACE_(IIORDMADevice, IIOService)
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

    // IIORDMADevice methods

    /**
     * @brief Get device information
     *
     * @param pInfo         Receives device information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        RDMA_DEVICE_INFO *pInfo
        ) PURE;

    /**
     * @brief Get port information
     *
     * @param uPortNum      Port number (1-based)
     * @param pInfo         Receives port information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid port number
     */
    STDMETHOD_(IO_RETURN, GetPortInfo)(THIS_
        UINT8 uPortNum,
        RDMA_PORT_INFO *pInfo
        ) PURE;

    /**
     * @brief Query port attributes
     *
     * @param uPortNum      Port number (1-based)
     * @param pInfo         Receives port information
     *
     * @retval IO_SUCCESS       Query successful
     * @retval IO_BAD_ARGUMENT  Invalid port number
     */
    STDMETHOD_(IO_RETURN, QueryPort)(THIS_
        UINT8 uPortNum,
        RDMA_PORT_INFO *pInfo
        ) PURE;

    /**
     * @brief Allocate Protection Domain
     *
     * @param puPDHandle    Receives PD handle
     *
     * @retval IO_SUCCESS       PD allocated successfully
     * @retval IO_NO_RESOURCES  No resources available
     */
    STDMETHOD_(IO_RETURN, AllocPD)(THIS_
        UINT32 *puPDHandle
        ) PURE;

    /**
     * @brief Deallocate Protection Domain
     *
     * @param uPDHandle     PD handle to deallocate
     *
     * @retval IO_SUCCESS       PD deallocated successfully
     * @retval IO_BAD_ARGUMENT  Invalid PD handle
     */
    STDMETHOD_(IO_RETURN, DeallocPD)(THIS_
        UINT32 uPDHandle
        ) PURE;

    /**
     * @brief Create Queue Pair
     *
     * @param uPDHandle     Protection domain handle
     * @param Type          Queue pair type
     * @param uSendDepth    Send queue depth
     * @param uRecvDepth    Receive queue depth
     * @param ppQP          Receives QP interface
     *
     * @retval IO_SUCCESS       QP created successfully
     * @retval IO_NO_RESOURCES  Insufficient resources
     */
    STDMETHOD_(IO_RETURN, CreateQP)(THIS_
        UINT32 uPDHandle,
        RDMA_QP_TYPE Type,
        UINT32 uSendDepth,
        UINT32 uRecvDepth,
        IIORDMAConnection **ppQP
        ) PURE;

    /**
     * @brief Destroy Queue Pair
     *
     * @param pQP           Queue pair to destroy
     *
     * @retval IO_SUCCESS       QP destroyed successfully
     */
    STDMETHOD_(IO_RETURN, DestroyQP)(THIS_
        IIORDMAConnection *pQP
        ) PURE;

    /**
     * @brief Modify Queue Pair state
     *
     * @param pQP           Queue pair to modify
     * @param NewState      New state
     * @param pInfo         State-specific parameters
     *
     * @retval IO_SUCCESS       QP modified successfully
     * @retval IO_BAD_ARGUMENT  Invalid state transition
     */
    STDMETHOD_(IO_RETURN, ModifyQP)(THIS_
        IIORDMAConnection *pQP,
        RDMA_QP_STATE NewState,
        CONST VOID *pInfo
        ) PURE;

    /**
     * @brief Query Queue Pair
     *
     * @param pQP           Queue pair to query
     * @param pInfo         Receives QP information
     *
     * @retval IO_SUCCESS       Query successful
     */
    STDMETHOD_(IO_RETURN, QueryQP)(THIS_
        IIORDMAConnection *pQP,
        RDMA_QP_INFO *pInfo
        ) PURE;

    /**
     * @brief Create Completion Queue
     *
     * @param uNumCQE       Number of CQ entries
     * @param puCQHandle    Receives CQ handle
     *
     * @retval IO_SUCCESS       CQ created successfully
     * @retval IO_NO_RESOURCES  Insufficient resources
     */
    STDMETHOD_(IO_RETURN, CreateCQ)(THIS_
        UINT32 uNumCQE,
        UINT32 *puCQHandle
        ) PURE;

    /**
     * @brief Destroy Completion Queue
     *
     * @param uCQHandle     CQ handle to destroy
     *
     * @retval IO_SUCCESS       CQ destroyed successfully
     */
    STDMETHOD_(IO_RETURN, DestroyCQ)(THIS_
        UINT32 uCQHandle
        ) PURE;

    /**
     * @brief Poll Completion Queue
     *
     * @param uCQHandle     CQ handle to poll
     * @param pWC           Array to receive completions
     * @param uMaxEntries   Maximum entries to poll
     * @param puNumEntries  Receives actual number polled
     *
     * @retval IO_SUCCESS       Poll successful
     * @retval IO_BAD_ARGUMENT  Invalid CQ handle
     */
    STDMETHOD_(IO_RETURN, PollCQ)(THIS_
        UINT32 uCQHandle,
        RDMA_WC *pWC,
        UINT32 uMaxEntries,
        UINT32 *puNumEntries
        ) PURE;

    /**
     * @brief Register memory region
     *
     * @param uPDHandle     Protection domain handle
     * @param pAddress      Virtual address
     * @param cbLength      Length in bytes
     * @param uAccessFlags  Access permissions
     * @param ppMR          Receives MR interface
     *
     * @retval IO_SUCCESS       Memory registered successfully
     * @retval IO_NO_RESOURCES  Insufficient resources
     */
    STDMETHOD_(IO_RETURN, RegisterMemory)(THIS_
        UINT32 uPDHandle,
        VOID *pAddress,
        UINT64 cbLength,
        UINT32 uAccessFlags,
        IIORDMAMemoryRegion **ppMR
        ) PURE;

    /**
     * @brief Deregister memory region
     *
     * @param pMR           Memory region to deregister
     *
     * @retval IO_SUCCESS       Memory deregistered successfully
     */
    STDMETHOD_(IO_RETURN, DeregisterMemory)(THIS_
        IIORDMAMemoryRegion *pMR
        ) PURE;

    /**
     * @brief Create Memory Window
     *
     * @param uPDHandle     Protection domain handle
     * @param puMWHandle    Receives MW handle
     *
     * @retval IO_SUCCESS       MW created successfully
     * @retval IO_NO_RESOURCES  Insufficient resources
     */
    STDMETHOD_(IO_RETURN, CreateMW)(THIS_
        UINT32 uPDHandle,
        UINT32 *puMWHandle
        ) PURE;

    /**
     * @brief Bind Memory Window
     *
     * @param uMWHandle     Memory window handle
     * @param pMR           Memory region to bind
     * @param uOffset       Offset within MR
     * @param cbLength      Length to bind
     * @param uAccessFlags  Access permissions
     *
     * @retval IO_SUCCESS       MW bound successfully
     */
    STDMETHOD_(IO_RETURN, BindMW)(THIS_
        UINT32 uMWHandle,
        IIORDMAMemoryRegion *pMR,
        UINT64 uOffset,
        UINT64 cbLength,
        UINT32 uAccessFlags
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIORDMAConnection - RDMA Connection/Queue Pair Interface
 *
 * This interface represents an RDMA queue pair and provides methods for
 * posting send/receive operations, RDMA read/write, and atomic operations.
 */
#undef INTERFACE
#define INTERFACE IIORDMAConnection

DECLARE_INTERFACE_(IIORDMAConnection, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)(THIS_
        REFIID riid,
        void **ppvObject
        ) PURE;

    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IIORDMAConnection methods

    /**
     * @brief Post send work request
     *
     * @param pWR           Work request to post
     * @param ppBadWR       Receives pointer to bad WR on error
     *
     * @retval IO_SUCCESS       WR posted successfully
     * @retval IO_BAD_ARGUMENT  Invalid WR
     */
    STDMETHOD_(IO_RETURN, PostSend)(THIS_
        RDMA_WR *pWR,
        RDMA_WR **ppBadWR
        ) PURE;

    /**
     * @brief Post receive work request
     *
     * @param pWR           Work request to post
     * @param ppBadWR       Receives pointer to bad WR on error
     *
     * @retval IO_SUCCESS       WR posted successfully
     * @retval IO_BAD_ARGUMENT  Invalid WR
     */
    STDMETHOD_(IO_RETURN, PostReceive)(THIS_
        RDMA_WR *pWR,
        RDMA_WR **ppBadWR
        ) PURE;

    /**
     * @brief Post RDMA Read operation
     *
     * @param uRemoteAddr   Remote address
     * @param uRemoteKey    Remote key
     * @param pLocalSGE     Local scatter-gather element
     * @param uWorkRequestID Work request ID
     *
     * @retval IO_SUCCESS       Read posted successfully
     */
    STDMETHOD_(IO_RETURN, PostRead)(THIS_
        UINT64 uRemoteAddr,
        UINT32 uRemoteKey,
        RDMA_SGE *pLocalSGE,
        UINT64 uWorkRequestID
        ) PURE;

    /**
     * @brief Post RDMA Write operation
     *
     * @param uRemoteAddr   Remote address
     * @param uRemoteKey    Remote key
     * @param pLocalSGE     Local scatter-gather element
     * @param uNumSGE       Number of SGEs
     * @param uWorkRequestID Work request ID
     *
     * @retval IO_SUCCESS       Write posted successfully
     */
    STDMETHOD_(IO_RETURN, PostWrite)(THIS_
        UINT64 uRemoteAddr,
        UINT32 uRemoteKey,
        RDMA_SGE *pLocalSGE,
        UINT32 uNumSGE,
        UINT64 uWorkRequestID
        ) PURE;

    /**
     * @brief Post Atomic operation
     *
     * @param uRemoteAddr   Remote address
     * @param uRemoteKey    Remote key
     * @param uCompareAdd   Compare or add operand
     * @param uSwap         Swap operand (CAS only)
     * @param pResultSGE    SGE for result
     * @param bIsCompareSwap TRUE for CAS, FALSE for FetchAdd
     * @param uWorkRequestID Work request ID
     *
     * @retval IO_SUCCESS       Atomic posted successfully
     * @retval IO_UNSUPPORTED   Atomic not supported
     */
    STDMETHOD_(IO_RETURN, PostAtomic)(THIS_
        UINT64 uRemoteAddr,
        UINT32 uRemoteKey,
        UINT64 uCompareAdd,
        UINT64 uSwap,
        RDMA_SGE *pResultSGE,
        BOOLEAN bIsCompareSwap,
        UINT64 uWorkRequestID
        ) PURE;

    /**
     * @brief Get queue pair state
     *
     * @param pState        Receives current state
     *
     * @retval IO_SUCCESS       State retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetState)(THIS_
        RDMA_QP_STATE *pState
        ) PURE;

    /**
     * @brief Modify queue pair state
     *
     * @param NewState      New state
     * @param pInfo         State-specific parameters
     *
     * @retval IO_SUCCESS       State modified successfully
     */
    STDMETHOD_(IO_RETURN, ModifyState)(THIS_
        RDMA_QP_STATE NewState,
        CONST VOID *pInfo
        ) PURE;

    /**
     * @brief Get queue pair information
     *
     * @param pInfo         Receives QP information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetInfo)(THIS_
        RDMA_QP_INFO *pInfo
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIORDMAMemoryRegion - Registered Memory Region Interface
 *
 * This interface represents a registered RDMA memory region.
 */
#undef INTERFACE
#define INTERFACE IIORDMAMemoryRegion

DECLARE_INTERFACE_(IIORDMAMemoryRegion, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)(THIS_
        REFIID riid,
        void **ppvObject
        ) PURE;

    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IIORDMAMemoryRegion methods

    /**
     * @brief Get memory region information
     *
     * @param pInfo         Receives MR information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetInfo)(THIS_
        RDMA_MR_INFO *pInfo
        ) PURE;

    /**
     * @brief Get local key
     *
     * @param puLKey        Receives local key
     *
     * @retval IO_SUCCESS       Key retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetLKey)(THIS_
        UINT32 *puLKey
        ) PURE;

    /**
     * @brief Get remote key
     *
     * @param puRKey        Receives remote key
     *
     * @retval IO_SUCCESS       Key retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetRKey)(THIS_
        UINT32 *puRKey
        ) PURE;
};

#undef INTERFACE

/*============================================================================
 * Convenience Macros
 *============================================================================*/

#if !defined(__cplusplus) || defined(CINTERFACE)

// IIORDMADevice macros
#define IIORDMADevice_GetDeviceInfo(p,a)            (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIORDMADevice_GetPortInfo(p,a,b)            (p)->lpVtbl->GetPortInfo(p,a,b)
#define IIORDMADevice_QueryPort(p,a,b)              (p)->lpVtbl->QueryPort(p,a,b)
#define IIORDMADevice_AllocPD(p,a)                  (p)->lpVtbl->AllocPD(p,a)
#define IIORDMADevice_DeallocPD(p,a)                (p)->lpVtbl->DeallocPD(p,a)
#define IIORDMADevice_CreateQP(p,a,b,c,d,e)         (p)->lpVtbl->CreateQP(p,a,b,c,d,e)
#define IIORDMADevice_DestroyQP(p,a)                (p)->lpVtbl->DestroyQP(p,a)
#define IIORDMADevice_ModifyQP(p,a,b,c)             (p)->lpVtbl->ModifyQP(p,a,b,c)
#define IIORDMADevice_QueryQP(p,a,b)                (p)->lpVtbl->QueryQP(p,a,b)
#define IIORDMADevice_CreateCQ(p,a,b)               (p)->lpVtbl->CreateCQ(p,a,b)
#define IIORDMADevice_DestroyCQ(p,a)                (p)->lpVtbl->DestroyCQ(p,a)
#define IIORDMADevice_PollCQ(p,a,b,c,d)             (p)->lpVtbl->PollCQ(p,a,b,c,d)
#define IIORDMADevice_RegisterMemory(p,a,b,c,d,e)   (p)->lpVtbl->RegisterMemory(p,a,b,c,d,e)
#define IIORDMADevice_DeregisterMemory(p,a)         (p)->lpVtbl->DeregisterMemory(p,a)
#define IIORDMADevice_CreateMW(p,a,b)               (p)->lpVtbl->CreateMW(p,a,b)
#define IIORDMADevice_BindMW(p,a,b,c,d,e)           (p)->lpVtbl->BindMW(p,a,b,c,d,e)

// IIORDMAConnection macros
#define IIORDMAConnection_PostSend(p,a,b)           (p)->lpVtbl->PostSend(p,a,b)
#define IIORDMAConnection_PostReceive(p,a,b)        (p)->lpVtbl->PostReceive(p,a,b)
#define IIORDMAConnection_PostRead(p,a,b,c,d)       (p)->lpVtbl->PostRead(p,a,b,c,d)
#define IIORDMAConnection_PostWrite(p,a,b,c,d,e)    (p)->lpVtbl->PostWrite(p,a,b,c,d,e)
#define IIORDMAConnection_PostAtomic(p,a,b,c,d,e,f,g) (p)->lpVtbl->PostAtomic(p,a,b,c,d,e,f,g)
#define IIORDMAConnection_GetState(p,a)             (p)->lpVtbl->GetState(p,a)
#define IIORDMAConnection_ModifyState(p,a,b)        (p)->lpVtbl->ModifyState(p,a,b)
#define IIORDMAConnection_GetInfo(p,a)              (p)->lpVtbl->GetInfo(p,a)

// IIORDMAMemoryRegion macros
#define IIORDMAMemoryRegion_GetInfo(p,a)            (p)->lpVtbl->GetInfo(p,a)
#define IIORDMAMemoryRegion_GetLKey(p,a)            (p)->lpVtbl->GetLKey(p,a)
#define IIORDMAMemoryRegion_GetRKey(p,a)            (p)->lpVtbl->GetRKey(p,a)

#endif

/*============================================================================
 * Helper Functions
 *============================================================================*/

/**
 * @brief Create RDMA device instance
 *
 * @param pPCIDevice    PCI device interface
 * @param ppDevice      Receives RDMA device interface
 *
 * @retval IO_SUCCESS   Device created successfully
 * @retval IO_NO_MEMORY Insufficient memory
 */
IO_RETURN
RDMADeviceCreate(
    IIOService *pPCIDevice,
    IIORDMADevice **ppDevice
    );

/**
 * @brief Get link speed in Gbps
 *
 * @param Speed         Link speed enum value
 *
 * @return Speed in Gbps, or 0 if unknown
 */
UINT32
RDMAGetLinkSpeedGbps(
    IB_LINK_SPEED Speed
    );

/**
 * @brief Get link width as number of lanes
 *
 * @param Width         Link width enum value
 *
 * @return Number of lanes, or 0 if unknown
 */
UINT32
RDMAGetLinkWidthLanes(
    IB_LINK_WIDTH Width
    );

/**
 * @brief Get MTU size in bytes
 *
 * @param MTU           MTU enum value
 *
 * @return MTU size in bytes
 */
UINT32
RDMAGetMTUSize(
    IB_MTU MTU
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_RDMA_H */
