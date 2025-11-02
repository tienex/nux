/**
 * @file virt.h
 * @brief Virtualization Family Interface - SR-IOV, virtio, and Paravirtualization
 *
 * This header defines the virtualization family interface for managing
 * virtualized devices, SR-IOV (Single Root I/O Virtualization), virtio
 * devices, and paravirtualized drivers for VMware, Hyper-V, and Xen.
 *
 * Supports:
 * - SR-IOV (Physical/Virtual Functions, VF migration, VFIO)
 * - virtio (net, blk, scsi, balloon, console, gpu, fs, vsock, crypto, mem, pmem, iommu)
 * - VMware paravirtualization (VMXNET3, PVSCSI, VMCI, Balloon)
 * - Hyper-V paravirtualization (VMBus, NetVSC, StorVSC, utilities)
 * - Xen paravirtualization (XenBus, netfront, blkfront, grant tables, event channels)
 * - IOMMU virtualization (Intel VT-d, AMD-Vi, PASID, PRI, ATS)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_VIRT_H
#define IOKIT_VIRT_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOVirtDevice interface GUID
 * {F1A2B3C4-D5E6-4F7A-8B9C-0D1E2F3A4B5C}
 */
DEFINE_GUID(IID_IIOVirtDevice,
    0xF1A2B3C4, 0xD5E6, 0x4F7A, 0x8B, 0x9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C);

/**
 * @brief IIOSRIOVPhysicalFunction interface GUID
 * {A1B2C3D4-E5F6-4A7B-8C9D-0E1F2A3B4C5D}
 */
DEFINE_GUID(IID_IIOSRIOVPhysicalFunction,
    0xA1B2C3D4, 0xE5F6, 0x4A7B, 0x8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D);

/**
 * @brief IIOSRIOVVirtualFunction interface GUID
 * {B2C3D4E5-F6A7-4B8C-9D0E-1F2A3B4C5D6E}
 */
DEFINE_GUID(IID_IIOSRIOVVirtualFunction,
    0xB2C3D4E5, 0xF6A7, 0x4B8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E);

/**
 * @brief IIOVirtioDevice interface GUID
 * {C3D4E5F6-A7B8-4C9D-0E1F-2A3B4C5D6E7F}
 */
DEFINE_GUID(IID_IIOVirtioDevice,
    0xC3D4E5F6, 0xA7B8, 0x4C9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F);

/**
 * @brief IIOVMBusDevice interface GUID
 * {D4E5F6A7-B8C9-4D0E-1F2A-3B4C5D6E7F8A}
 */
DEFINE_GUID(IID_IIOVMBusDevice,
    0xD4E5F6A7, 0xB8C9, 0x4D0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A);

/**
 * @brief IIOXenBusDevice interface GUID
 * {E5F6A7B8-C9D0-4E1F-2A3B-4C5D6E7F8A9B}
 */
DEFINE_GUID(IID_IIOXenBusDevice,
    0xE5F6A7B8, 0xC9D0, 0x4E1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B);

//
// ============================================================================
// SR-IOV (Single Root I/O Virtualization) Support
// ============================================================================
//

/**
 * @brief SR-IOV capability registers (PCIe Extended Capability ID 0x10)
 */
#define PCIE_EXT_CAP_SRIOV_ID           0x0010

/**
 * @brief SR-IOV capability structure offsets
 */
#define SRIOV_CAP_CAPABILITIES          0x04    /**< SR-IOV capabilities */
#define SRIOV_CAP_CONTROL               0x08    /**< SR-IOV control */
#define SRIOV_CAP_STATUS                0x0A    /**< SR-IOV status */
#define SRIOV_CAP_INITIAL_VFS           0x0C    /**< Initial VFs */
#define SRIOV_CAP_TOTAL_VFS             0x0E    /**< Total VFs */
#define SRIOV_CAP_NUM_VFS               0x10    /**< Number of VFs */
#define SRIOV_CAP_FUNC_DEPENDENCY       0x12    /**< Function dependency link */
#define SRIOV_CAP_FIRST_VF_OFFSET       0x14    /**< First VF offset */
#define SRIOV_CAP_VF_STRIDE             0x16    /**< VF stride */
#define SRIOV_CAP_VF_DEVICE_ID          0x1A    /**< VF device ID */
#define SRIOV_CAP_SUPPORTED_PAGE_SIZES  0x1C    /**< Supported page sizes */
#define SRIOV_CAP_SYSTEM_PAGE_SIZE      0x20    /**< System page size */
#define SRIOV_CAP_VF_BAR0               0x24    /**< VF BAR 0 */
#define SRIOV_CAP_VF_BAR1               0x28    /**< VF BAR 1 */
#define SRIOV_CAP_VF_BAR2               0x2C    /**< VF BAR 2 */
#define SRIOV_CAP_VF_BAR3               0x30    /**< VF BAR 3 */
#define SRIOV_CAP_VF_BAR4               0x34    /**< VF BAR 4 */
#define SRIOV_CAP_VF_BAR5               0x38    /**< VF BAR 5 */
#define SRIOV_CAP_VF_MIGRATION_STATE    0x3C    /**< VF migration state array offset */

/**
 * @brief SR-IOV control register bits
 */
#define SRIOV_CTRL_VF_ENABLE            0x0001  /**< VF enable */
#define SRIOV_CTRL_VF_MIGRATION_ENABLE  0x0002  /**< VF migration enable */
#define SRIOV_CTRL_VF_MIGRATION_INT_EN  0x0004  /**< VF migration interrupt enable */
#define SRIOV_CTRL_VF_MSE               0x0008  /**< VF memory space enable */
#define SRIOV_CTRL_ARI_CAPABLE          0x0010  /**< ARI capable hierarchy */

/**
 * @brief SR-IOV status register bits
 */
#define SRIOV_STATUS_VF_MIGRATION       0x0001  /**< VF migration status */

/**
 * @brief VF migration state
 */
typedef enum _VF_MIGRATION_STATE {
    VF_MIGRATION_INACTIVE   = 0,    /**< Not migrating */
    VF_MIGRATION_ACTIVE     = 1,    /**< Migration in progress */
    VF_MIGRATION_SUSPENDED  = 2,    /**< Migration suspended */
    VF_MIGRATION_ERROR      = 3,    /**< Migration error */
} VF_MIGRATION_STATE;

/**
 * @brief SR-IOV capability information
 */
typedef struct _SRIOV_CAPABILITY {
    UINT16  CapVersion;             /**< Capability version */
    UINT16  TotalVFs;               /**< Total VFs */
    UINT16  InitialVFs;             /**< Initial VFs */
    UINT16  NumVFs;                 /**< Number of VFs */
    UINT16  FirstVFOffset;          /**< First VF offset */
    UINT16  VFStride;               /**< VF stride */
    UINT16  VFDeviceID;             /**< VF device ID */
    UINT32  SupportedPageSizes;     /**< Supported page sizes (bitmask) */
    UINT32  SystemPageSize;         /**< System page size */
    UINT16  FunctionDependencyLink; /**< Function dependency link */
    BOOLEAN bVFMigrationSupported;  /**< VF migration capable */
    BOOLEAN bARICapable;            /**< ARI capable */
} SRIOV_CAPABILITY;

/**
 * @brief SR-IOV Physical Function information
 */
typedef struct _SRIOV_PF_INFO {
    UINT16  VendorID;               /**< PF vendor ID */
    UINT16  DeviceID;               /**< PF device ID */
    UINT16  TotalVFs;               /**< Total VFs */
    UINT16  ActiveVFs;              /**< Active VFs */
    UINT16  FirstVFOffset;          /**< First VF offset */
    UINT16  VFStride;               /**< VF stride */
    UINT16  VFDeviceID;             /**< VF device ID */
    UINT8   Bus;                    /**< PCI bus */
    UINT8   Device;                 /**< PCI device */
    UINT8   Function;               /**< PCI function */
    BOOLEAN bEnabled;               /**< SR-IOV enabled */
} SRIOV_PF_INFO;

/**
 * @brief SR-IOV Virtual Function information
 */
typedef struct _SRIOV_VF_INFO {
    UINT16              VendorID;       /**< VF vendor ID */
    UINT16              DeviceID;       /**< VF device ID */
    UINT16              VFIndex;        /**< VF index (0-based) */
    UINT8               Bus;            /**< PCI bus */
    UINT8               Device;         /**< PCI device */
    UINT8               Function;       /**< PCI function */
    IIOService         *pPF;            /**< Parent PF */
    VF_MIGRATION_STATE  MigrationState; /**< Migration state */
    UINT32              IOMMUGroup;     /**< IOMMU group ID */
    BOOLEAN             bAssigned;      /**< Assigned to guest */
} SRIOV_VF_INFO;

/**
 * @brief VFIO (Virtual Function I/O) information
 */
typedef struct _VFIO_INFO {
    UINT32  ContainerFD;            /**< VFIO container file descriptor */
    UINT32  GroupID;                /**< IOMMU group ID */
    UINT32  DeviceFD;               /**< VFIO device file descriptor */
    UINT64  IOVABase;               /**< IOVA base address */
    UINT64  IOVASize;               /**< IOVA size */
    BOOLEAN bDMAMapping;            /**< DMA mapping enabled */
} VFIO_INFO;

//
// ============================================================================
// virtio (Virtual I/O Device) Support
// ============================================================================
//

/**
 * @brief virtio vendor ID (Red Hat)
 */
#define VIRTIO_VENDOR_ID                0x1AF4

/**
 * @brief virtio device type enumeration
 */
typedef enum _VIRTIO_DEVICE_TYPE {
    VIRTIO_DEV_RESERVED         = 0,    /**< Reserved */
    VIRTIO_DEV_NET              = 1,    /**< Network card */
    VIRTIO_DEV_BLOCK            = 2,    /**< Block device */
    VIRTIO_DEV_CONSOLE          = 3,    /**< Console */
    VIRTIO_DEV_RNG              = 4,    /**< Entropy source */
    VIRTIO_DEV_BALLOON          = 5,    /**< Memory balloon */
    VIRTIO_DEV_IOMEM            = 6,    /**< ioMemory */
    VIRTIO_DEV_RPMSG            = 7,    /**< rpmsg */
    VIRTIO_DEV_SCSI             = 8,    /**< SCSI host */
    VIRTIO_DEV_9P               = 9,    /**< 9P transport */
    VIRTIO_DEV_MAC80211_WLAN    = 10,   /**< mac80211 wlan */
    VIRTIO_DEV_RPROC_SERIAL     = 11,   /**< rproc serial */
    VIRTIO_DEV_CAIF             = 12,   /**< Virtio CAIF */
    VIRTIO_DEV_MEMORY_BALLOON   = 13,   /**< Memory balloon (modern) */
    VIRTIO_DEV_GPU              = 16,   /**< GPU device */
    VIRTIO_DEV_TIMER            = 17,   /**< Timer/Clock */
    VIRTIO_DEV_INPUT            = 18,   /**< Input device */
    VIRTIO_DEV_SOCKET           = 19,   /**< Socket (vsock) */
    VIRTIO_DEV_CRYPTO           = 20,   /**< Crypto device */
    VIRTIO_DEV_SIGNAL_DIST      = 21,   /**< Signal distribution */
    VIRTIO_DEV_PSTORE           = 22,   /**< pstore device */
    VIRTIO_DEV_IOMMU            = 23,   /**< IOMMU */
    VIRTIO_DEV_MEM              = 24,   /**< Memory device */
    VIRTIO_DEV_SOUND            = 25,   /**< Sound device */
    VIRTIO_DEV_FS               = 26,   /**< File system */
    VIRTIO_DEV_PMEM             = 27,   /**< Persistent memory */
    VIRTIO_DEV_RPMB             = 28,   /**< RPMB */
    VIRTIO_DEV_MAC80211_HWSIM   = 29,   /**< mac80211 hwsim */
    VIRTIO_DEV_VIDEO_ENCODER    = 30,   /**< Video encoder */
    VIRTIO_DEV_VIDEO_DECODER    = 31,   /**< Video decoder */
    VIRTIO_DEV_SCMI             = 32,   /**< SCMI */
    VIRTIO_DEV_NITRO_SEC_MOD    = 33,   /**< Nitro Secure Module */
    VIRTIO_DEV_I2C_ADAPTER      = 34,   /**< I2C adapter */
    VIRTIO_DEV_WATCHDOG         = 35,   /**< Watchdog */
    VIRTIO_DEV_CAN              = 36,   /**< CAN bus */
    VIRTIO_DEV_DMABUF           = 37,   /**< DMABUF */
    VIRTIO_DEV_PARAM_SERVER     = 38,   /**< Parameter server */
    VIRTIO_DEV_AUDIO_POLICY     = 39,   /**< Audio policy */
    VIRTIO_DEV_BT               = 40,   /**< Bluetooth */
    VIRTIO_DEV_GPIO             = 41,   /**< GPIO */
} VIRTIO_DEVICE_TYPE;

/**
 * @brief virtio PCI device IDs
 */
#define VIRTIO_PCI_LEGACY_NET           0x1000  /**< Legacy network */
#define VIRTIO_PCI_LEGACY_BLOCK         0x1001  /**< Legacy block */
#define VIRTIO_PCI_LEGACY_BALLOON       0x1002  /**< Legacy balloon */
#define VIRTIO_PCI_LEGACY_CONSOLE       0x1003  /**< Legacy console */
#define VIRTIO_PCI_LEGACY_SCSI          0x1004  /**< Legacy SCSI */
#define VIRTIO_PCI_LEGACY_RNG           0x1005  /**< Legacy entropy */
#define VIRTIO_PCI_MODERN_BASE          0x1040  /**< Modern device base (+ device type) */

/**
 * @brief virtio transport type
 */
typedef enum _VIRTIO_TRANSPORT_TYPE {
    VIRTIO_TRANSPORT_PCI        = 0,    /**< PCI/PCIe transport */
    VIRTIO_TRANSPORT_MMIO       = 1,    /**< Memory-mapped I/O */
    VIRTIO_TRANSPORT_CHANNEL_IO = 2,    /**< Channel I/O (s390) */
} VIRTIO_TRANSPORT_TYPE;

/**
 * @brief virtio device status flags
 */
#define VIRTIO_STATUS_ACKNOWLEDGE       0x01    /**< Guest OS has found device */
#define VIRTIO_STATUS_DRIVER            0x02    /**< Guest OS knows how to drive */
#define VIRTIO_STATUS_DRIVER_OK         0x04    /**< Driver is set up */
#define VIRTIO_STATUS_FEATURES_OK       0x08    /**< Feature negotiation complete */
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET 0x40   /**< Device needs reset */
#define VIRTIO_STATUS_FAILED            0x80    /**< Fatal error */

/**
 * @brief virtio feature bits (common)
 */
#define VIRTIO_F_NOTIFY_ON_EMPTY        (1ULL << 24)    /**< Notify when queue empty */
#define VIRTIO_F_ANY_LAYOUT             (1ULL << 27)    /**< Flexible descriptor layout */
#define VIRTIO_F_RING_INDIRECT_DESC     (1ULL << 28)    /**< Indirect descriptors */
#define VIRTIO_F_RING_EVENT_IDX         (1ULL << 29)    /**< Event index */
#define VIRTIO_F_VERSION_1              (1ULL << 32)    /**< virtio 1.0 compliance */
#define VIRTIO_F_ACCESS_PLATFORM        (1ULL << 33)    /**< Platform access */
#define VIRTIO_F_RING_PACKED            (1ULL << 34)    /**< Packed ring layout */
#define VIRTIO_F_IN_ORDER               (1ULL << 35)    /**< In-order completion */
#define VIRTIO_F_ORDER_PLATFORM         (1ULL << 36)    /**< Platform ordering */
#define VIRTIO_F_SR_IOV                 (1ULL << 37)    /**< SR-IOV support */
#define VIRTIO_F_NOTIFICATION_DATA      (1ULL << 38)    /**< Notification data */

/**
 * @brief virtio-net feature bits
 */
#define VIRTIO_NET_F_CSUM               (1ULL << 0)     /**< Checksum offload */
#define VIRTIO_NET_F_GUEST_CSUM         (1ULL << 1)     /**< Guest checksum */
#define VIRTIO_NET_F_CTRL_GUEST_OFFLOADS (1ULL << 2)    /**< Control guest offloads */
#define VIRTIO_NET_F_MTU                (1ULL << 3)     /**< MTU setting */
#define VIRTIO_NET_F_MAC                (1ULL << 5)     /**< MAC address */
#define VIRTIO_NET_F_GSO                (1ULL << 6)     /**< Generic segmentation offload */
#define VIRTIO_NET_F_GUEST_TSO4         (1ULL << 7)     /**< Guest TSO4 */
#define VIRTIO_NET_F_GUEST_TSO6         (1ULL << 8)     /**< Guest TSO6 */
#define VIRTIO_NET_F_GUEST_ECN          (1ULL << 9)     /**< Guest ECN */
#define VIRTIO_NET_F_GUEST_UFO          (1ULL << 10)    /**< Guest UFO */
#define VIRTIO_NET_F_HOST_TSO4          (1ULL << 11)    /**< Host TSO4 */
#define VIRTIO_NET_F_HOST_TSO6          (1ULL << 12)    /**< Host TSO6 */
#define VIRTIO_NET_F_HOST_ECN           (1ULL << 13)    /**< Host ECN */
#define VIRTIO_NET_F_HOST_UFO           (1ULL << 14)    /**< Host UFO */
#define VIRTIO_NET_F_MRG_RXBUF          (1ULL << 15)    /**< Merge RX buffers */
#define VIRTIO_NET_F_STATUS             (1ULL << 16)    /**< Link status */
#define VIRTIO_NET_F_CTRL_VQ            (1ULL << 17)    /**< Control virtqueue */
#define VIRTIO_NET_F_CTRL_RX            (1ULL << 18)    /**< Control RX mode */
#define VIRTIO_NET_F_CTRL_VLAN          (1ULL << 19)    /**< VLAN filtering */
#define VIRTIO_NET_F_GUEST_ANNOUNCE     (1ULL << 21)    /**< Guest announce */
#define VIRTIO_NET_F_MQ                 (1ULL << 22)    /**< Multiqueue */
#define VIRTIO_NET_F_CTRL_MAC_ADDR      (1ULL << 23)    /**< Set MAC address */
#define VIRTIO_NET_F_RSC_EXT            (1ULL << 61)    /**< Extended coalescing */
#define VIRTIO_NET_F_STANDBY            (1ULL << 62)    /**< Standby mode */

/**
 * @brief virtio-blk feature bits
 */
#define VIRTIO_BLK_F_SIZE_MAX           (1ULL << 1)     /**< Maximum size */
#define VIRTIO_BLK_F_SEG_MAX            (1ULL << 2)     /**< Maximum segments */
#define VIRTIO_BLK_F_GEOMETRY           (1ULL << 4)     /**< Disk geometry */
#define VIRTIO_BLK_F_RO                 (1ULL << 5)     /**< Read-only */
#define VIRTIO_BLK_F_BLK_SIZE           (1ULL << 6)     /**< Block size */
#define VIRTIO_BLK_F_FLUSH              (1ULL << 9)     /**< Flush command */
#define VIRTIO_BLK_F_TOPOLOGY           (1ULL << 10)    /**< Topology info */
#define VIRTIO_BLK_F_CONFIG_WCE         (1ULL << 11)    /**< Writeback cache */
#define VIRTIO_BLK_F_DISCARD            (1ULL << 13)    /**< DISCARD */
#define VIRTIO_BLK_F_WRITE_ZEROES       (1ULL << 14)    /**< WRITE ZEROES */

/**
 * @brief virtio descriptor flags
 */
#define VIRTIO_DESC_F_NEXT              1       /**< Buffer continues via next */
#define VIRTIO_DESC_F_WRITE             2       /**< Write-only (device writes) */
#define VIRTIO_DESC_F_INDIRECT          4       /**< Indirect descriptor */

/**
 * @brief virtio descriptor
 */
typedef struct _VIRTIO_DESCRIPTOR {
    UINT64  Address;                /**< Buffer physical address */
    UINT32  Length;                 /**< Buffer length */
    UINT16  Flags;                  /**< Descriptor flags */
    UINT16  Next;                   /**< Next descriptor (if NEXT flag) */
} VIRTIO_DESCRIPTOR;

/**
 * @brief virtio available ring
 */
typedef struct _VIRTIO_AVAIL_RING {
    UINT16  Flags;                  /**< Flags */
    UINT16  Index;                  /**< Index */
    UINT16  Ring[0];                /**< Available ring entries */
    /* UINT16 UsedEvent follows after Ring[QueueSize] */
} VIRTIO_AVAIL_RING;

/**
 * @brief virtio used ring element
 */
typedef struct _VIRTIO_USED_ELEM {
    UINT32  Index;                  /**< Descriptor chain index */
    UINT32  Length;                 /**< Bytes written */
} VIRTIO_USED_ELEM;

/**
 * @brief virtio used ring
 */
typedef struct _VIRTIO_USED_RING {
    UINT16  Flags;                  /**< Flags */
    UINT16  Index;                  /**< Index */
    VIRTIO_USED_ELEM Ring[0];       /**< Used ring entries */
    /* UINT16 AvailEvent follows after Ring[QueueSize] */
} VIRTIO_USED_RING;

/**
 * @brief virtio queue information
 */
typedef struct _VIRTIO_QUEUE_INFO {
    UINT16              QueueSize;      /**< Queue size */
    UINT16              QueueIndex;     /**< Queue index */
    UINT64              DescriptorTable;/**< Descriptor table physical address */
    UINT64              AvailableRing;  /**< Available ring physical address */
    UINT64              UsedRing;       /**< Used ring physical address */
    UINT16              LastAvailIdx;   /**< Last available index */
    UINT16              LastUsedIdx;    /**< Last used index */
    BOOLEAN             bReady;         /**< Queue is ready */
} VIRTIO_QUEUE_INFO;

/**
 * @brief virtio device information
 */
typedef struct _VIRTIO_DEVICE_INFO {
    VIRTIO_DEVICE_TYPE      DeviceType;     /**< Device type */
    VIRTIO_TRANSPORT_TYPE   TransportType;  /**< Transport type */
    UINT16                  VendorID;       /**< Vendor ID */
    UINT16                  DeviceID;       /**< Device ID */
    UINT16                  SubsystemVendorID; /**< Subsystem vendor ID */
    UINT16                  SubsystemDeviceID; /**< Subsystem device ID */
    UINT64                  DeviceFeatures; /**< Device features (64-bit) */
    UINT64                  DriverFeatures; /**< Driver features (64-bit) */
    UINT8                   Status;         /**< Device status */
    UINT8                   ConfigGeneration; /**< Config generation */
    UINT32                  NumQueues;      /**< Number of queues */
    UINT32                  QueueSizeMax;   /**< Maximum queue size */
    UINT64                  ConfigSpaceBase;/**< Config space base (MMIO) */
    UINT32                  ConfigSpaceSize;/**< Config space size */
    CHAR8                   DeviceName[64]; /**< Device name */
} VIRTIO_DEVICE_INFO;

/**
 * @brief virtio-net configuration space
 */
typedef struct _VIRTIO_NET_CONFIG {
    UINT8   MAC[6];                 /**< MAC address */
    UINT16  Status;                 /**< Link status */
    UINT16  MaxVirtqueuePairs;      /**< Max virtqueue pairs */
    UINT16  MTU;                    /**< MTU */
    UINT32  Speed;                  /**< Link speed (Mbps) */
    UINT8   Duplex;                 /**< Duplex mode */
} VIRTIO_NET_CONFIG;

/**
 * @brief virtio-blk configuration space
 */
typedef struct _VIRTIO_BLK_CONFIG {
    UINT64  Capacity;               /**< Capacity in 512-byte sectors */
    UINT32  SizeMax;                /**< Maximum segment size */
    UINT32  SegMax;                 /**< Maximum segments */
    struct {
        UINT16  Cylinders;          /**< Cylinders */
        UINT8   Heads;              /**< Heads */
        UINT8   Sectors;            /**< Sectors per track */
    } Geometry;
    UINT32  BlkSize;                /**< Block size */
    UINT8   PhysicalBlockExp;       /**< Physical block size exponent */
    UINT8   AlignmentOffset;        /**< Alignment offset */
    UINT16  MinIOSize;              /**< Minimum I/O size */
    UINT32  OptIOSize;              /**< Optimal I/O size */
    UINT8   WritebackCacheEnabled;  /**< Writeback cache enabled */
    UINT8   Reserved[3];
    UINT32  MaxDiscardSectors;      /**< Max discard sectors */
    UINT32  MaxDiscardSeg;          /**< Max discard segments */
    UINT32  DiscardSectorAlignment; /**< Discard sector alignment */
    UINT32  MaxWriteZeroesSectors;  /**< Max write zeroes sectors */
    UINT32  MaxWriteZeroesSeg;      /**< Max write zeroes segments */
    UINT8   WriteZeroesMayUnmap;    /**< Write zeroes may unmap */
} VIRTIO_BLK_CONFIG;

/**
 * @brief virtio-blk request types
 */
#define VIRTIO_BLK_T_IN             0   /**< Read */
#define VIRTIO_BLK_T_OUT            1   /**< Write */
#define VIRTIO_BLK_T_FLUSH          4   /**< Flush */
#define VIRTIO_BLK_T_DISCARD        11  /**< Discard */
#define VIRTIO_BLK_T_WRITE_ZEROES   13  /**< Write zeroes */

//
// ============================================================================
// VMware Paravirtualization Support
// ============================================================================
//

/**
 * @brief VMware vendor ID
 */
#define VMWARE_VENDOR_ID            0x15AD

/**
 * @brief VMware device IDs
 */
#define VMWARE_DEVICE_VMXNET        0x0720  /**< vmxnet (legacy) */
#define VMWARE_DEVICE_VMXNET2       0x0721  /**< vmxnet2 */
#define VMWARE_DEVICE_VMXNET3       0x07B0  /**< vmxnet3 */
#define VMWARE_DEVICE_PVSCSI        0x07C0  /**< PVSCSI */
#define VMWARE_DEVICE_VMCI          0x0740  /**< VMCI */
#define VMWARE_DEVICE_VMEMDEV       0x0770  /**< Memory balloon */
#define VMWARE_DEVICE_HYPERVISOR    0x0405  /**< Hypervisor device */

/**
 * @brief VMXNET3 version
 */
typedef enum _VMXNET3_VERSION {
    VMXNET3_VERSION_2           = 2,    /**< VMXNET3 v2 */
    VMXNET3_VERSION_3           = 3,    /**< VMXNET3 v3 */
    VMXNET3_VERSION_4           = 4,    /**< VMXNET3 v4 (ESXi 6.7+) */
    VMXNET3_VERSION_5           = 5,    /**< VMXNET3 v5 (ESXi 7.0+) */
} VMXNET3_VERSION;

/**
 * @brief VMXNET3 device information
 */
typedef struct _VMXNET3_DEVICE_INFO {
    VMXNET3_VERSION Version;        /**< VMXNET3 version */
    UINT8           MAC[6];         /**< MAC address */
    UINT16          MTU;            /**< MTU */
    UINT32          NumTxQueues;    /**< Number of TX queues */
    UINT32          NumRxQueues;    /**< Number of RX queues */
    UINT64          Features;       /**< Feature flags */
    BOOLEAN         bLROSupported;  /**< LRO support */
    BOOLEAN         bRSSSupported;  /**< RSS support */
    CHAR8           DeviceName[64]; /**< Device name */
} VMXNET3_DEVICE_INFO;

/**
 * @brief PVSCSI device information
 */
typedef struct _PVSCSI_DEVICE_INFO {
    UINT32  MaxTargets;             /**< Maximum targets */
    UINT32  MaxLUNs;                /**< Maximum LUNs per target */
    UINT32  MaxCmdPerLUN;           /**< Max commands per LUN */
    UINT32  MaxCDBLength;           /**< Maximum CDB length */
    UINT32  RingPages;              /**< Number of ring pages */
    CHAR8   AdapterName[64];        /**< Adapter name */
} PVSCSI_DEVICE_INFO;

/**
 * @brief VMCI device information
 */
typedef struct _VMCI_DEVICE_INFO {
    UINT32  ContextID;              /**< VMCI context ID */
    UINT32  Version;                /**< VMCI version */
    UINT32  Capabilities;           /**< VMCI capabilities */
    CHAR8   DeviceName[64];         /**< Device name */
} VMCI_DEVICE_INFO;

/**
 * @brief VMware balloon information
 */
typedef struct _VMWARE_BALLOON_INFO {
    UINT64  MaxBalloonSize;         /**< Maximum balloon size (MB) */
    UINT64  CurrentBalloonSize;     /**< Current balloon size (MB) */
    UINT64  TargetBalloonSize;      /**< Target balloon size (MB) */
    UINT32  PageSize;               /**< Page size */
    BOOLEAN bEnabled;               /**< Balloon enabled */
} VMWARE_BALLOON_INFO;

//
// ============================================================================
// Hyper-V Paravirtualization Support
// ============================================================================
//

/**
 * @brief Hyper-V VMBus device GUID
 * Microsoft VMBus class GUID
 */
#define HYPERV_VMBUS_CLASS_GUID     "{C376C1C3-D276-48D2-90A9-C04748072C60}"

/**
 * @brief Hyper-V synthetic device type GUIDs
 */
#define HYPERV_NETVSC_GUID          "{F8615163-DF3E-46C5-913F-F2D2F965ED0E}"
#define HYPERV_STORVSC_GUID         "{BA6163D9-04A1-4D29-B605-72E2FFB1DC7F}"
#define HYPERV_KVP_GUID             "{242FF919-07DB-4180-9C2E-B86CB68C8C55}"
#define HYPERV_VSS_GUID             "{35FA2E29-EA23-4236-96AE-3A6EBACBA440}"
#define HYPERV_FCOPY_GUID           "{34D14BE3-DEE4-41C8-9AE7-6B174977C192}"
#define HYPERV_SHUTDOWN_GUID        "{0E0B6031-5213-4934-818B-38D90CED39DB}"
#define HYPERV_TIMESYNC_GUID        "{9527E630-D0AE-497B-ADCE-E80AB0175CAF}"
#define HYPERV_HEARTBEAT_GUID       "{57164F39-9115-4E78-AB55-382F3BD5422D}"

/**
 * @brief VMBus version
 */
typedef enum _VMBUS_VERSION {
    VMBUS_VERSION_WS2008        = 0x00010000,   /**< Windows Server 2008 */
    VMBUS_VERSION_WIN7          = 0x00020000,   /**< Windows 7 */
    VMBUS_VERSION_WIN8          = 0x00030000,   /**< Windows 8 */
    VMBUS_VERSION_WIN8_1        = 0x00030001,   /**< Windows 8.1 */
    VMBUS_VERSION_WIN10         = 0x00040000,   /**< Windows 10 */
    VMBUS_VERSION_WIN10_V5      = 0x00050000,   /**< Windows 10 v5 */
} VMBUS_VERSION;

/**
 * @brief VMBus channel information
 */
typedef struct _VMBUS_CHANNEL_INFO {
    UINT32      ChannelID;          /**< Channel ID */
    UINT32      MonitorID;          /**< Monitor ID */
    UINT32      RingBufferPages;    /**< Ring buffer pages */
    UINT64      RingBufferGPADLHandle; /**< Ring buffer GPADL handle */
    CHAR8       InterfaceType[128]; /**< Interface type GUID */
    CHAR8       InterfaceInstance[128]; /**< Interface instance GUID */
    BOOLEAN     bOpened;            /**< Channel opened */
} VMBUS_CHANNEL_INFO;

/**
 * @brief NetVSC (Hyper-V synthetic network) device information
 */
typedef struct _NETVSC_DEVICE_INFO {
    VMBUS_VERSION   Version;        /**< VMBus version */
    UINT8           MAC[6];         /**< MAC address */
    UINT16          MTU;            /**< MTU */
    UINT32          NumChannels;    /**< Number of channels */
    UINT32          SendSectionSize;/**< Send section size */
    UINT32          RecvSectionCount; /**< Receive section count */
    BOOLEAN         bRSSSupported;  /**< RSS support */
    CHAR8           DeviceName[64]; /**< Device name */
} NETVSC_DEVICE_INFO;

/**
 * @brief StorVSC (Hyper-V synthetic storage) device information
 */
typedef struct _STORVSC_DEVICE_INFO {
    VMBUS_VERSION   Version;        /**< VMBus version */
    UINT32          MaxTargets;     /**< Maximum targets */
    UINT32          MaxChannels;    /**< Maximum channels */
    UINT32          MaxTransferBytes; /**< Maximum transfer bytes */
    BOOLEAN         bVirtualDevice; /**< Virtual device */
    CHAR8           AdapterName[64]; /**< Adapter name */
} STORVSC_DEVICE_INFO;

/**
 * @brief Hyper-V balloon information
 */
typedef struct _HYPERV_BALLOON_INFO {
    UINT64  TotalMemory;            /**< Total memory (MB) */
    UINT64  CommittedMemory;        /**< Committed memory (MB) */
    UINT64  AvailableMemory;        /**< Available memory (MB) */
    UINT32  PageSize;               /**< Page size */
    BOOLEAN bDynamicMemoryEnabled;  /**< Dynamic memory enabled */
} HYPERV_BALLOON_INFO;

/**
 * @brief Hyper-V utility service types
 */
typedef enum _HYPERV_UTILITY_TYPE {
    HYPERV_UTIL_SHUTDOWN        = 0,    /**< Shutdown */
    HYPERV_UTIL_TIMESYNC        = 1,    /**< Time synchronization */
    HYPERV_UTIL_HEARTBEAT       = 2,    /**< Heartbeat */
    HYPERV_UTIL_KVP             = 3,    /**< Key-Value Pair */
    HYPERV_UTIL_VSS             = 4,    /**< Volume Shadow Copy */
    HYPERV_UTIL_FCOPY           = 5,    /**< File Copy */
} HYPERV_UTILITY_TYPE;

//
// ============================================================================
// Xen Paravirtualization Support
// ============================================================================
//

/**
 * @brief Xen platform device IDs
 */
#define XEN_VENDOR_ID               0x5853  /**< "XS" */
#define XEN_PLATFORM_DEVICE_ID      0x0001  /**< Xen platform device */

/**
 * @brief XenBus device state
 */
typedef enum _XENBUS_STATE {
    XENBUS_STATE_UNKNOWN        = 0,    /**< Unknown */
    XENBUS_STATE_INITIALISING   = 1,    /**< Initializing */
    XENBUS_STATE_INIT_WAIT      = 2,    /**< Waiting for init */
    XENBUS_STATE_INITIALISED    = 3,    /**< Initialized */
    XENBUS_STATE_CONNECTED      = 4,    /**< Connected */
    XENBUS_STATE_CLOSING        = 5,    /**< Closing */
    XENBUS_STATE_CLOSED         = 6,    /**< Closed */
    XENBUS_STATE_RECONFIGURING  = 7,    /**< Reconfiguring */
    XENBUS_STATE_RECONFIGURED   = 8,    /**< Reconfigured */
} XENBUS_STATE;

/**
 * @brief Xen grant reference
 */
typedef struct _XEN_GRANT_REF {
    UINT32  GrantRef;               /**< Grant reference */
    UINT64  FrameNumber;            /**< Frame number (PFN) */
    UINT16  DomainID;               /**< Domain ID */
    BOOLEAN bReadOnly;              /**< Read-only grant */
} XEN_GRANT_REF;

/**
 * @brief Xen event channel
 */
typedef struct _XEN_EVENT_CHANNEL {
    UINT32  Port;                   /**< Event channel port */
    UINT32  Vector;                 /**< Interrupt vector */
    UINT16  TargetDomain;           /**< Target domain ID */
    BOOLEAN bBound;                 /**< Channel bound */
} XEN_EVENT_CHANNEL;

/**
 * @brief Xen netfront (network) device information
 */
typedef struct _NETIF_INFO {
    XENBUS_STATE    State;          /**< Device state */
    UINT8           MAC[6];         /**< MAC address */
    UINT16          MTU;            /**< MTU */
    UINT32          NumTxRingPages; /**< TX ring pages */
    UINT32          NumRxRingPages; /**< RX ring pages */
    XEN_GRANT_REF   TxGrantRef;     /**< TX grant reference */
    XEN_GRANT_REF   RxGrantRef;     /**< RX grant reference */
    XEN_EVENT_CHANNEL EventChannel; /**< Event channel */
    CHAR8           DeviceName[64]; /**< Device name */
} NETIF_INFO;

/**
 * @brief Xen blkfront (block) device information
 */
typedef struct _BLKIF_INFO {
    XENBUS_STATE    State;          /**< Device state */
    UINT64          Capacity;       /**< Capacity in sectors */
    UINT32          SectorSize;     /**< Sector size */
    UINT32          NumRingPages;   /**< Ring pages */
    XEN_GRANT_REF   RingGrantRef;   /**< Ring grant reference */
    XEN_EVENT_CHANNEL EventChannel; /**< Event channel */
    BOOLEAN         bReadOnly;      /**< Read-only */
    BOOLEAN         bRemovable;     /**< Removable */
    CHAR8           DeviceName[64]; /**< Device name */
} BLKIF_INFO;

//
// ============================================================================
// IOMMU Virtualization Support
// ============================================================================
//

/**
 * @brief IOMMU type
 */
typedef enum _IOMMU_TYPE {
    IOMMU_TYPE_NONE             = 0,    /**< No IOMMU */
    IOMMU_TYPE_INTEL_VTD        = 1,    /**< Intel VT-d (DMAR) */
    IOMMU_TYPE_AMD_VI           = 2,    /**< AMD-Vi (IOMMU) */
    IOMMU_TYPE_ARM_SMMU         = 3,    /**< ARM SMMU */
    IOMMU_TYPE_ARM_SMMU_V3      = 4,    /**< ARM SMMUv3 */
    IOMMU_TYPE_VIRTIO           = 5,    /**< virtio-iommu */
} IOMMU_TYPE;

/**
 * @brief IOMMU domain type
 */
typedef enum _IOMMU_DOMAIN_TYPE {
    IOMMU_DOMAIN_UNMANAGED      = 0,    /**< Unmanaged */
    IOMMU_DOMAIN_DMA            = 1,    /**< DMA domain */
    IOMMU_DOMAIN_IDENTITY       = 2,    /**< Identity (pass-through) */
    IOMMU_DOMAIN_DMA_FQ         = 3,    /**< DMA with flush queue */
} IOMMU_DOMAIN_TYPE;

/**
 * @brief IOMMU capability flags
 */
#define IOMMU_CAP_CACHE_COHERENCY   0x00000001  /**< Cache coherency */
#define IOMMU_CAP_INTR_REMAP        0x00000002  /**< Interrupt remapping */
#define IOMMU_CAP_NOEXEC            0x00000004  /**< Execute-never */
#define IOMMU_CAP_PAGE_SIZE_4K      0x00000008  /**< 4KB pages */
#define IOMMU_CAP_PAGE_SIZE_2M      0x00000010  /**< 2MB pages */
#define IOMMU_CAP_PAGE_SIZE_1G      0x00000020  /**< 1GB pages */

/**
 * @brief IOMMU domain information
 */
typedef struct _IOMMU_DOMAIN_INFO {
    UINT32              DomainID;       /**< Domain ID */
    IOMMU_DOMAIN_TYPE   Type;           /**< Domain type */
    UINT64              PageTableBase;  /**< Page table base address */
    UINT32              Capabilities;   /**< Capability flags */
    BOOLEAN             bActive;        /**< Domain is active */
} IOMMU_DOMAIN_INFO;

/**
 * @brief PASID (Process Address Space ID) information
 */
typedef struct _PASID_INFO {
    UINT32  PASID;                  /**< PASID value (20 bits) */
    UINT32  IOASID;                 /**< I/O Address Space ID */
    UINT64  PageTableRoot;          /**< Page table root */
    BOOLEAN bEnabled;               /**< PASID enabled */
} PASID_INFO;

/**
 * @brief ATS (Address Translation Services) capability
 */
typedef struct _ATS_CAPABILITY {
    UINT16  CapOffset;              /**< Capability offset */
    UINT8   InvalidateQueueDepth;   /**< Invalidate queue depth */
    UINT8   PageAligned;            /**< Page-aligned request */
    BOOLEAN bEnabled;               /**< ATS enabled */
    BOOLEAN bGlobalInvalidate;      /**< Global invalidate support */
} ATS_CAPABILITY;

/**
 * @brief PRI (Page Request Interface) capability
 */
typedef struct _PRI_CAPABILITY {
    UINT16  CapOffset;              /**< Capability offset */
    UINT32  MaxOutstandingRequests; /**< Maximum outstanding requests */
    BOOLEAN bEnabled;               /**< PRI enabled */
    BOOLEAN bStopped;               /**< PRI stopped */
} PRI_CAPABILITY;

/**
 * @brief Page request queue entry
 */
typedef struct _PAGE_REQUEST_ENTRY {
    UINT64  Address;                /**< Requested address */
    UINT32  PASID;                  /**< PASID */
    UINT16  PRGI;                   /**< Page request group index */
    UINT8   Flags;                  /**< Request flags */
    BOOLEAN bLastInGroup;           /**< Last in group */
} PAGE_REQUEST_ENTRY;

//
// ============================================================================
// Interface Declarations
// ============================================================================
//

// Forward declarations
DECLARE_INTERFACE_(IIOVirtDevice, IIOService);
DECLARE_INTERFACE_(IIOSRIOVPhysicalFunction, IIOService);
DECLARE_INTERFACE_(IIOSRIOVVirtualFunction, IIOService);
DECLARE_INTERFACE_(IIOVirtioDevice, IIOService);
DECLARE_INTERFACE_(IIOVMBusDevice, IIOService);
DECLARE_INTERFACE_(IIOXenBusDevice, IIOService);

/**
 * @brief IIOVirtDevice - Generic virtualized device interface
 */
#undef INTERFACE
#define INTERFACE IIOVirtDevice

DECLARE_INTERFACE_(IIOVirtDevice, IIOService)
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

    // IIOVirtDevice methods

    /**
     * @brief Check if device is virtualized
     */
    STDMETHOD_(IO_RETURN, IsVirtualized)(THIS_
        BOOLEAN *pbVirtualized
        ) PURE;

    /**
     * @brief Get hypervisor type
     */
    STDMETHOD_(IO_RETURN, GetHypervisorType)(THIS_
        CHAR8 *pszHypervisor,
        UINT32 cbSize
        ) PURE;
};

/**
 * @brief IIOSRIOVPhysicalFunction - SR-IOV Physical Function interface
 */
#undef INTERFACE
#define INTERFACE IIOSRIOVPhysicalFunction

DECLARE_INTERFACE_(IIOSRIOVPhysicalFunction, IIOService)
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

    // IIOSRIOVPhysicalFunction methods

    /**
     * @brief Get PF information
     */
    STDMETHOD_(IO_RETURN, GetPFInfo)(THIS_
        SRIOV_PF_INFO *pInfo
        ) PURE;

    /**
     * @brief Get SR-IOV capability
     */
    STDMETHOD_(IO_RETURN, GetCapability)(THIS_
        SRIOV_CAPABILITY *pCapability
        ) PURE;

    /**
     * @brief Enable SR-IOV
     */
    STDMETHOD_(IO_RETURN, EnableSRIOV)(THIS) PURE;

    /**
     * @brief Disable SR-IOV
     */
    STDMETHOD_(IO_RETURN, DisableSRIOV)(THIS) PURE;

    /**
     * @brief Get number of VFs
     */
    STDMETHOD_(IO_RETURN, GetNumVFs)(THIS_
        UINT16 *puNumVFs
        ) PURE;

    /**
     * @brief Set number of VFs
     */
    STDMETHOD_(IO_RETURN, SetNumVFs)(THIS_
        UINT16 uNumVFs
        ) PURE;

    /**
     * @brief Create a VF
     */
    STDMETHOD_(IO_RETURN, CreateVF)(THIS_
        UINT16 uVFIndex,
        IIOSRIOVVirtualFunction **ppVF
        ) PURE;

    /**
     * @brief Destroy a VF
     */
    STDMETHOD_(IO_RETURN, DestroyVF)(THIS_
        UINT16 uVFIndex
        ) PURE;

    /**
     * @brief Get VF information
     */
    STDMETHOD_(IO_RETURN, GetVFInfo)(THIS_
        UINT16 uVFIndex,
        SRIOV_VF_INFO *pInfo
        ) PURE;

    /**
     * @brief Configure VF (MAC, VLAN, bandwidth)
     */
    STDMETHOD_(IO_RETURN, ConfigureVF)(THIS_
        UINT16 uVFIndex,
        CONST UINT8 *pMAC,
        UINT16 uVLAN,
        UINT32 uBandwidthMbps
        ) PURE;

    /**
     * @brief Migrate VF
     */
    STDMETHOD_(IO_RETURN, MigrateVF)(THIS_
        UINT16 uVFIndex,
        UINT64 uTargetAddress
        ) PURE;
};

/**
 * @brief IIOSRIOVVirtualFunction - SR-IOV Virtual Function interface
 */
#undef INTERFACE
#define INTERFACE IIOSRIOVVirtualFunction

DECLARE_INTERFACE_(IIOSRIOVVirtualFunction, IIOService)
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

    // IIOSRIOVVirtualFunction methods

    /**
     * @brief Get VF information
     */
    STDMETHOD_(IO_RETURN, GetVFInfo)(THIS_
        SRIOV_VF_INFO *pInfo
        ) PURE;

    /**
     * @brief Get parent PF
     */
    STDMETHOD_(IO_RETURN, GetPF)(THIS_
        IIOSRIOVPhysicalFunction **ppPF
        ) PURE;

    /**
     * @brief Get VF index
     */
    STDMETHOD_(IO_RETURN, GetVFIndex)(THIS_
        UINT16 *puVFIndex
        ) PURE;

    /**
     * @brief Reset VF
     */
    STDMETHOD_(IO_RETURN, Reset)(THIS) PURE;

    /**
     * @brief Get migration state
     */
    STDMETHOD_(IO_RETURN, GetMigrationState)(THIS_
        VF_MIGRATION_STATE *pState
        ) PURE;

    /**
     * @brief Begin migration
     */
    STDMETHOD_(IO_RETURN, BeginMigration)(THIS) PURE;

    /**
     * @brief Complete migration
     */
    STDMETHOD_(IO_RETURN, CompleteMigration)(THIS) PURE;
};

/**
 * @brief IIOVirtioDevice - virtio device interface
 */
#undef INTERFACE
#define INTERFACE IIOVirtioDevice

DECLARE_INTERFACE_(IIOVirtioDevice, IIOService)
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

    // IIOVirtioDevice methods

    /**
     * @brief Get device information
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        VIRTIO_DEVICE_INFO *pInfo
        ) PURE;

    /**
     * @brief Get device type
     */
    STDMETHOD_(IO_RETURN, GetDeviceType)(THIS_
        VIRTIO_DEVICE_TYPE *pType
        ) PURE;

    /**
     * @brief Get device features
     */
    STDMETHOD_(IO_RETURN, GetFeatures)(THIS_
        UINT64 *pFeatures
        ) PURE;

    /**
     * @brief Negotiate features
     */
    STDMETHOD_(IO_RETURN, NegotiateFeatures)(THIS_
        UINT64 uFeatures
        ) PURE;

    /**
     * @brief Get config space
     */
    STDMETHOD_(IO_RETURN, GetConfigSpace)(THIS_
        VOID *pBuffer,
        UINT32 uOffset,
        UINT32 uLength
        ) PURE;

    /**
     * @brief Set config space
     */
    STDMETHOD_(IO_RETURN, SetConfigSpace)(THIS_
        CONST VOID *pBuffer,
        UINT32 uOffset,
        UINT32 uLength
        ) PURE;

    /**
     * @brief Create virtqueue
     */
    STDMETHOD_(IO_RETURN, CreateQueue)(THIS_
        UINT16 uQueueIndex,
        UINT16 uQueueSize,
        VIRTIO_QUEUE_INFO *pQueueInfo
        ) PURE;

    /**
     * @brief Destroy virtqueue
     */
    STDMETHOD_(IO_RETURN, DestroyQueue)(THIS_
        UINT16 uQueueIndex
        ) PURE;

    /**
     * @brief Notify queue (kick)
     */
    STDMETHOD_(IO_RETURN, NotifyQueue)(THIS_
        UINT16 uQueueIndex
        ) PURE;

    /**
     * @brief Get device status
     */
    STDMETHOD_(IO_RETURN, GetStatus)(THIS_
        UINT8 *puStatus
        ) PURE;

    /**
     * @brief Set device status
     */
    STDMETHOD_(IO_RETURN, SetStatus)(THIS_
        UINT8 uStatus
        ) PURE;

    /**
     * @brief Reset device
     */
    STDMETHOD_(IO_RETURN, Reset)(THIS) PURE;
};

/**
 * @brief IIOVMBusDevice - Hyper-V VMBus device interface
 */
#undef INTERFACE
#define INTERFACE IIOVMBusDevice

DECLARE_INTERFACE_(IIOVMBusDevice, IIOService)
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

    // IIOVMBusDevice methods

    /**
     * @brief Get VMBus version
     */
    STDMETHOD_(IO_RETURN, GetVersion)(THIS_
        VMBUS_VERSION *pVersion
        ) PURE;

    /**
     * @brief Get channel information
     */
    STDMETHOD_(IO_RETURN, GetChannelInfo)(THIS_
        VMBUS_CHANNEL_INFO *pInfo
        ) PURE;

    /**
     * @brief Open VMBus channel
     */
    STDMETHOD_(IO_RETURN, OpenChannel)(THIS_
        UINT32 uRingBufferPages
        ) PURE;

    /**
     * @brief Close VMBus channel
     */
    STDMETHOD_(IO_RETURN, CloseChannel)(THIS) PURE;

    /**
     * @brief Send packet
     */
    STDMETHOD_(IO_RETURN, SendPacket)(THIS_
        CONST VOID *pData,
        UINT32 uLength
        ) PURE;

    /**
     * @brief Receive packet
     */
    STDMETHOD_(IO_RETURN, ReceivePacket)(THIS_
        VOID *pBuffer,
        UINT32 uBufferSize,
        UINT32 *puBytesReceived
        ) PURE;
};

/**
 * @brief IIOXenBusDevice - Xen bus device interface
 */
#undef INTERFACE
#define INTERFACE IIOXenBusDevice

DECLARE_INTERFACE_(IIOXenBusDevice, IIOService)
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

    // IIOXenBusDevice methods

    /**
     * @brief Get device state
     */
    STDMETHOD_(IO_RETURN, GetState)(THIS_
        XENBUS_STATE *pState
        ) PURE;

    /**
     * @brief Set device state
     */
    STDMETHOD_(IO_RETURN, SetState)(THIS_
        XENBUS_STATE State
        ) PURE;

    /**
     * @brief Grant page access
     */
    STDMETHOD_(IO_RETURN, GrantAccess)(THIS_
        UINT64 uFrameNumber,
        UINT16 uDomainID,
        BOOLEAN bReadOnly,
        XEN_GRANT_REF *pGrantRef
        ) PURE;

    /**
     * @brief Revoke grant
     */
    STDMETHOD_(IO_RETURN, RevokeGrant)(THIS_
        XEN_GRANT_REF *pGrantRef
        ) PURE;

    /**
     * @brief Allocate event channel
     */
    STDMETHOD_(IO_RETURN, AllocEventChannel)(THIS_
        UINT16 uTargetDomain,
        XEN_EVENT_CHANNEL *pEventChannel
        ) PURE;

    /**
     * @brief Free event channel
     */
    STDMETHOD_(IO_RETURN, FreeEventChannel)(THIS_
        XEN_EVENT_CHANNEL *pEventChannel
        ) PURE;

    /**
     * @brief Notify event channel
     */
    STDMETHOD_(IO_RETURN, NotifyEventChannel)(THIS_
        XEN_EVENT_CHANNEL *pEventChannel
        ) PURE;
};

#undef INTERFACE

//
// ============================================================================
// Convenience Macros
// ============================================================================
//

#if !defined(__cplusplus) || defined(CINTERFACE)

// IIOVirtDevice macros
#define IIOVirtDevice_IsVirtualized(p,a)                (p)->lpVtbl->IsVirtualized(p,a)
#define IIOVirtDevice_GetHypervisorType(p,a,b)          (p)->lpVtbl->GetHypervisorType(p,a,b)

// IIOSRIOVPhysicalFunction macros
#define IIOSRIOVPhysicalFunction_GetPFInfo(p,a)         (p)->lpVtbl->GetPFInfo(p,a)
#define IIOSRIOVPhysicalFunction_GetCapability(p,a)     (p)->lpVtbl->GetCapability(p,a)
#define IIOSRIOVPhysicalFunction_EnableSRIOV(p)         (p)->lpVtbl->EnableSRIOV(p)
#define IIOSRIOVPhysicalFunction_DisableSRIOV(p)        (p)->lpVtbl->DisableSRIOV(p)
#define IIOSRIOVPhysicalFunction_GetNumVFs(p,a)         (p)->lpVtbl->GetNumVFs(p,a)
#define IIOSRIOVPhysicalFunction_SetNumVFs(p,a)         (p)->lpVtbl->SetNumVFs(p,a)
#define IIOSRIOVPhysicalFunction_CreateVF(p,a,b)        (p)->lpVtbl->CreateVF(p,a,b)
#define IIOSRIOVPhysicalFunction_DestroyVF(p,a)         (p)->lpVtbl->DestroyVF(p,a)
#define IIOSRIOVPhysicalFunction_GetVFInfo(p,a,b)       (p)->lpVtbl->GetVFInfo(p,a,b)
#define IIOSRIOVPhysicalFunction_ConfigureVF(p,a,b,c,d) (p)->lpVtbl->ConfigureVF(p,a,b,c,d)
#define IIOSRIOVPhysicalFunction_MigrateVF(p,a,b)       (p)->lpVtbl->MigrateVF(p,a,b)

// IIOSRIOVVirtualFunction macros
#define IIOSRIOVVirtualFunction_GetVFInfo(p,a)          (p)->lpVtbl->GetVFInfo(p,a)
#define IIOSRIOVVirtualFunction_GetPF(p,a)              (p)->lpVtbl->GetPF(p,a)
#define IIOSRIOVVirtualFunction_GetVFIndex(p,a)         (p)->lpVtbl->GetVFIndex(p,a)
#define IIOSRIOVVirtualFunction_Reset(p)                (p)->lpVtbl->Reset(p)
#define IIOSRIOVVirtualFunction_GetMigrationState(p,a)  (p)->lpVtbl->GetMigrationState(p,a)
#define IIOSRIOVVirtualFunction_BeginMigration(p)       (p)->lpVtbl->BeginMigration(p)
#define IIOSRIOVVirtualFunction_CompleteMigration(p)    (p)->lpVtbl->CompleteMigration(p)

// IIOVirtioDevice macros
#define IIOVirtioDevice_GetDeviceInfo(p,a)              (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOVirtioDevice_GetDeviceType(p,a)              (p)->lpVtbl->GetDeviceType(p,a)
#define IIOVirtioDevice_GetFeatures(p,a)                (p)->lpVtbl->GetFeatures(p,a)
#define IIOVirtioDevice_NegotiateFeatures(p,a)          (p)->lpVtbl->NegotiateFeatures(p,a)
#define IIOVirtioDevice_GetConfigSpace(p,a,b,c)         (p)->lpVtbl->GetConfigSpace(p,a,b,c)
#define IIOVirtioDevice_SetConfigSpace(p,a,b,c)         (p)->lpVtbl->SetConfigSpace(p,a,b,c)
#define IIOVirtioDevice_CreateQueue(p,a,b,c)            (p)->lpVtbl->CreateQueue(p,a,b,c)
#define IIOVirtioDevice_DestroyQueue(p,a)               (p)->lpVtbl->DestroyQueue(p,a)
#define IIOVirtioDevice_NotifyQueue(p,a)                (p)->lpVtbl->NotifyQueue(p,a)
#define IIOVirtioDevice_GetStatus(p,a)                  (p)->lpVtbl->GetStatus(p,a)
#define IIOVirtioDevice_SetStatus(p,a)                  (p)->lpVtbl->SetStatus(p,a)
#define IIOVirtioDevice_Reset(p)                        (p)->lpVtbl->Reset(p)

// IIOVMBusDevice macros
#define IIOVMBusDevice_GetVersion(p,a)                  (p)->lpVtbl->GetVersion(p,a)
#define IIOVMBusDevice_GetChannelInfo(p,a)              (p)->lpVtbl->GetChannelInfo(p,a)
#define IIOVMBusDevice_OpenChannel(p,a)                 (p)->lpVtbl->OpenChannel(p,a)
#define IIOVMBusDevice_CloseChannel(p)                  (p)->lpVtbl->CloseChannel(p)
#define IIOVMBusDevice_SendPacket(p,a,b)                (p)->lpVtbl->SendPacket(p,a,b)
#define IIOVMBusDevice_ReceivePacket(p,a,b,c)           (p)->lpVtbl->ReceivePacket(p,a,b,c)

// IIOXenBusDevice macros
#define IIOXenBusDevice_GetState(p,a)                   (p)->lpVtbl->GetState(p,a)
#define IIOXenBusDevice_SetState(p,a)                   (p)->lpVtbl->SetState(p,a)
#define IIOXenBusDevice_GrantAccess(p,a,b,c,d)          (p)->lpVtbl->GrantAccess(p,a,b,c,d)
#define IIOXenBusDevice_RevokeGrant(p,a)                (p)->lpVtbl->RevokeGrant(p,a)
#define IIOXenBusDevice_AllocEventChannel(p,a,b)        (p)->lpVtbl->AllocEventChannel(p,a,b)
#define IIOXenBusDevice_FreeEventChannel(p,a)           (p)->lpVtbl->FreeEventChannel(p,a)
#define IIOXenBusDevice_NotifyEventChannel(p,a)         (p)->lpVtbl->NotifyEventChannel(p,a)

#endif

//
// ============================================================================
// Module Functions
// ============================================================================
//

/**
 * @brief Initialize virtualization subsystem
 *
 * @retval IO_SUCCESS   Subsystem initialized successfully
 */
IO_RETURN
VirtInitialize(
    VOID
    );

/**
 * @brief Shutdown virtualization subsystem
 *
 * @retval IO_SUCCESS   Subsystem shut down successfully
 */
IO_RETURN
VirtShutdown(
    VOID
    );

/**
 * @brief Detect hypervisor
 *
 * @param pszHypervisor Buffer to receive hypervisor name
 * @param cbSize        Buffer size
 *
 * @retval IO_SUCCESS   Hypervisor detected
 * @retval IO_NO_MATCH  No hypervisor detected
 */
IO_RETURN
VirtDetectHypervisor(
    CHAR8  *pszHypervisor,
    UINT32  cbSize
    );

/**
 * @brief Create SR-IOV PF instance
 *
 * @param pszName   Device name
 * @param ppPF      Receives PF interface
 *
 * @retval IO_SUCCESS       PF created successfully
 * @retval IO_ERR_NO_MEMORY Insufficient memory
 */
IO_RETURN
IOSRIOVPFCreate(
    CONST CHAR8                 *pszName,
    IIOSRIOVPhysicalFunction   **ppPF
    );

/**
 * @brief Create virtio device instance
 *
 * @param pszName       Device name
 * @param ppDevice      Receives virtio device interface
 *
 * @retval IO_SUCCESS       Device created successfully
 * @retval IO_ERR_NO_MEMORY Insufficient memory
 */
IO_RETURN
IOVirtioDeviceCreate(
    CONST CHAR8      *pszName,
    IIOVirtioDevice **ppDevice
    );

/**
 * @brief Create VMBus device instance
 *
 * @param pszName       Device name
 * @param ppDevice      Receives VMBus device interface
 *
 * @retval IO_SUCCESS       Device created successfully
 * @retval IO_ERR_NO_MEMORY Insufficient memory
 */
IO_RETURN
IOVMBusDeviceCreate(
    CONST CHAR8      *pszName,
    IIOVMBusDevice  **ppDevice
    );

/**
 * @brief Create XenBus device instance
 *
 * @param pszName       Device name
 * @param ppDevice      Receives XenBus device interface
 *
 * @retval IO_SUCCESS       Device created successfully
 * @retval IO_ERR_NO_MEMORY Insufficient memory
 */
IO_RETURN
IOXenBusDeviceCreate(
    CONST CHAR8       *pszName,
    IIOXenBusDevice  **ppDevice
    );

#ifdef __cplusplus
}
#endif

#endif // IOKIT_VIRT_H
