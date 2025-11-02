/**
 * @file virtio_example.c
 * @brief virtio Example - Demonstrates virtio device management
 *
 * This example shows how to:
 * - Detect virtio devices
 * - Initialize virtio-net (network) devices
 * - Initialize virtio-blk (block) devices
 * - Negotiate features
 * - Set up virtqueues
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <stdio.h>
#include <iokit/IOKit.h>
#include <iokit/families/virt/virt.h>
#include <iokit/families/pcie/pcie.h>

/**
 * @brief Example: Initialize virtio-net device
 */
static void
Example_VirtioNet(
    IIOPCIDevice *pPCIDevice
    )
{
    IO_RETURN Status;
    IIOVirtioDevice *pVirtioDevice = NULL;
    VIRTIO_DEVICE_INFO DeviceInfo;
    VIRTIO_DEVICE_TYPE DeviceType;
    UINT64 Features;

    printf("\n=== virtio-net Example ===\n");

    // Create virtio device instance
    Status = IOVirtioDeviceCreate("virtio-net0", &pVirtioDevice);
    if (Status != IO_SUCCESS) {
        printf("Failed to create virtio device: 0x%08X\n", Status);
        return;
    }

    // Start the device
    Status = IIOService_Start((IIOService*)pVirtioDevice, (IIOService*)pPCIDevice);
    if (Status != IO_SUCCESS) {
        printf("Failed to start virtio device: 0x%08X\n", Status);
        goto cleanup;
    }

    // Get device information
    Status = IIOVirtioDevice_GetDeviceInfo(pVirtioDevice, &DeviceInfo);
    if (Status == IO_SUCCESS) {
        printf("virtio Device Information:\n");
        printf("  Device Name:  %s\n", DeviceInfo.DeviceName);
        printf("  Device Type:  %d\n", DeviceInfo.DeviceType);
        printf("  Vendor ID:    0x%04X\n", DeviceInfo.VendorID);
        printf("  Device ID:    0x%04X\n", DeviceInfo.DeviceID);
        printf("  Transport:    %s\n",
               DeviceInfo.TransportType == VIRTIO_TRANSPORT_PCI ? "PCI" :
               DeviceInfo.TransportType == VIRTIO_TRANSPORT_MMIO ? "MMIO" : "Unknown");
    }

    // Get device type
    Status = IIOVirtioDevice_GetDeviceType(pVirtioDevice, &DeviceType);
    if (Status == IO_SUCCESS && DeviceType == VIRTIO_DEV_NET) {
        printf("\nThis is a virtio-net (network) device\n");

        // Get features
        Status = IIOVirtioDevice_GetFeatures(pVirtioDevice, &Features);
        if (Status == IO_SUCCESS) {
            printf("\nDevice Features (0x%016llX):\n", Features);

            // Check for common virtio-net features
            if (Features & VIRTIO_NET_F_MAC)
                printf("  - MAC address\n");
            if (Features & VIRTIO_NET_F_CSUM)
                printf("  - Checksum offload\n");
            if (Features & VIRTIO_NET_F_HOST_TSO4)
                printf("  - TCP Segmentation Offload (IPv4)\n");
            if (Features & VIRTIO_NET_F_HOST_TSO6)
                printf("  - TCP Segmentation Offload (IPv6)\n");
            if (Features & VIRTIO_NET_F_MRG_RXBUF)
                printf("  - Merge RX buffers\n");
            if (Features & VIRTIO_NET_F_STATUS)
                printf("  - Link status\n");
            if (Features & VIRTIO_NET_F_CTRL_VQ)
                printf("  - Control virtqueue\n");
            if (Features & VIRTIO_NET_F_MQ)
                printf("  - Multiqueue\n");
            if (Features & VIRTIO_F_VERSION_1)
                printf("  - virtio 1.0 compliant\n");
        }

        // Negotiate features (accept all offered features for this example)
        printf("\nNegotiating features...\n");
        Status = IIOVirtioDevice_NegotiateFeatures(pVirtioDevice, Features);
        if (Status == IO_SUCCESS) {
            printf("Features negotiated successfully\n");
        }

        // Read MAC address from config space
        VIRTIO_NET_CONFIG NetConfig;
        Status = IIOVirtioDevice_GetConfigSpace(pVirtioDevice, &NetConfig, 0, sizeof(NetConfig));
        if (Status == IO_SUCCESS) {
            printf("\nNetwork Configuration:\n");
            printf("  MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                   NetConfig.MAC[0], NetConfig.MAC[1], NetConfig.MAC[2],
                   NetConfig.MAC[3], NetConfig.MAC[4], NetConfig.MAC[5]);
            if (Features & VIRTIO_NET_F_STATUS) {
                printf("  Link Status: %s\n", (NetConfig.Status & 0x01) ? "Up" : "Down");
            }
            if (Features & VIRTIO_NET_F_MQ) {
                printf("  Max Queue Pairs: %d\n", NetConfig.MaxVirtqueuePairs);
            }
            if (Features & VIRTIO_NET_F_MTU) {
                printf("  MTU: %d\n", NetConfig.MTU);
            }
        }

        // Create virtqueues (typically RX and TX queues)
        printf("\nCreating virtqueues...\n");
        VIRTIO_QUEUE_INFO QueueInfo;

        // RX queue (queue 0)
        Status = IIOVirtioDevice_CreateQueue(pVirtioDevice, 0, 256, &QueueInfo);
        if (Status == IO_SUCCESS) {
            printf("  RX queue (0) created: %d entries\n", QueueInfo.QueueSize);
        }

        // TX queue (queue 1)
        Status = IIOVirtioDevice_CreateQueue(pVirtioDevice, 1, 256, &QueueInfo);
        if (Status == IO_SUCCESS) {
            printf("  TX queue (1) created: %d entries\n", QueueInfo.QueueSize);
        }

        // Set device status to DRIVER_OK
        printf("\nSetting device status to DRIVER_OK...\n");
        Status = IIOVirtioDevice_SetStatus(pVirtioDevice,
                                           VIRTIO_STATUS_ACKNOWLEDGE |
                                           VIRTIO_STATUS_DRIVER |
                                           VIRTIO_STATUS_FEATURES_OK |
                                           VIRTIO_STATUS_DRIVER_OK);
        if (Status == IO_SUCCESS) {
            printf("Device is now ready for operation!\n");
        }
    }

cleanup:
    if (pVirtioDevice) {
        IIOService_Release((IUnknown*)pVirtioDevice);
    }
}

/**
 * @brief Example: Initialize virtio-blk device
 */
static void
Example_VirtioBlk(
    IIOPCIDevice *pPCIDevice
    )
{
    IO_RETURN Status;
    IIOVirtioDevice *pVirtioDevice = NULL;
    VIRTIO_DEVICE_TYPE DeviceType;
    UINT64 Features;

    printf("\n=== virtio-blk Example ===\n");

    // Create virtio device instance
    Status = IOVirtioDeviceCreate("virtio-blk0", &pVirtioDevice);
    if (Status != IO_SUCCESS) {
        printf("Failed to create virtio device: 0x%08X\n", Status);
        return;
    }

    // Start the device
    Status = IIOService_Start((IIOService*)pVirtioDevice, (IIOService*)pPCIDevice);
    if (Status != IO_SUCCESS) {
        printf("Failed to start virtio device: 0x%08X\n", Status);
        goto cleanup;
    }

    // Get device type
    Status = IIOVirtioDevice_GetDeviceType(pVirtioDevice, &DeviceType);
    if (Status == IO_SUCCESS && DeviceType == VIRTIO_DEV_BLOCK) {
        printf("This is a virtio-blk (block storage) device\n");

        // Get features
        Status = IIOVirtioDevice_GetFeatures(pVirtioDevice, &Features);
        if (Status == IO_SUCCESS) {
            printf("\nDevice Features (0x%016llX):\n", Features);

            if (Features & VIRTIO_BLK_F_SIZE_MAX)
                printf("  - Maximum segment size\n");
            if (Features & VIRTIO_BLK_F_SEG_MAX)
                printf("  - Maximum segments\n");
            if (Features & VIRTIO_BLK_F_GEOMETRY)
                printf("  - Disk geometry\n");
            if (Features & VIRTIO_BLK_F_RO)
                printf("  - Read-only\n");
            if (Features & VIRTIO_BLK_F_BLK_SIZE)
                printf("  - Block size\n");
            if (Features & VIRTIO_BLK_F_FLUSH)
                printf("  - Flush command\n");
            if (Features & VIRTIO_BLK_F_TOPOLOGY)
                printf("  - Topology information\n");
            if (Features & VIRTIO_BLK_F_CONFIG_WCE)
                printf("  - Writeback cache enable\n");
            if (Features & VIRTIO_BLK_F_DISCARD)
                printf("  - DISCARD command\n");
            if (Features & VIRTIO_BLK_F_WRITE_ZEROES)
                printf("  - WRITE ZEROES command\n");
        }

        // Read block device config
        VIRTIO_BLK_CONFIG BlkConfig;
        Status = IIOVirtioDevice_GetConfigSpace(pVirtioDevice, &BlkConfig, 0, sizeof(BlkConfig));
        if (Status == IO_SUCCESS) {
            printf("\nBlock Device Configuration:\n");
            printf("  Capacity:    %llu sectors (512-byte)\n", BlkConfig.Capacity);
            printf("  Size:        %llu bytes (%.2f GB)\n",
                   BlkConfig.Capacity * 512ULL,
                   (BlkConfig.Capacity * 512ULL) / (1024.0 * 1024.0 * 1024.0));

            if (Features & VIRTIO_BLK_F_BLK_SIZE) {
                printf("  Block Size:  %u bytes\n", BlkConfig.BlkSize);
            }

            if (Features & VIRTIO_BLK_F_GEOMETRY) {
                printf("  Geometry:    C/H/S = %u/%u/%u\n",
                       BlkConfig.Geometry.Cylinders,
                       BlkConfig.Geometry.Heads,
                       BlkConfig.Geometry.Sectors);
            }

            if (Features & VIRTIO_BLK_F_TOPOLOGY) {
                printf("  Physical Block: %u bytes\n",
                       1 << BlkConfig.PhysicalBlockExp);
                printf("  Optimal I/O:    %u bytes\n", BlkConfig.OptIOSize);
            }

            if (Features & VIRTIO_BLK_F_RO) {
                printf("  Access:      Read-Only\n");
            } else {
                printf("  Access:      Read-Write\n");
            }
        }

        // Create I/O queue
        printf("\nCreating I/O virtqueue...\n");
        VIRTIO_QUEUE_INFO QueueInfo;
        Status = IIOVirtioDevice_CreateQueue(pVirtioDevice, 0, 128, &QueueInfo);
        if (Status == IO_SUCCESS) {
            printf("  I/O queue created: %d entries\n", QueueInfo.QueueSize);
        }

        // Negotiate features and set status
        IIOVirtioDevice_NegotiateFeatures(pVirtioDevice, Features);
        IIOVirtioDevice_SetStatus(pVirtioDevice,
                                  VIRTIO_STATUS_ACKNOWLEDGE |
                                  VIRTIO_STATUS_DRIVER |
                                  VIRTIO_STATUS_FEATURES_OK |
                                  VIRTIO_STATUS_DRIVER_OK);

        printf("\nBlock device is ready for I/O operations!\n");
    }

cleanup:
    if (pVirtioDevice) {
        IIOService_Release((IUnknown*)pVirtioDevice);
    }
}

/**
 * @brief Example: Detect hypervisor
 */
static void
Example_DetectHypervisor(
    VOID
    )
{
    IO_RETURN Status;
    CHAR8 HypervisorName[64];

    printf("\n=== Hypervisor Detection Example ===\n");

    Status = VirtDetectHypervisor(HypervisorName, sizeof(HypervisorName));
    if (Status == IO_SUCCESS) {
        printf("Running under hypervisor: %s\n", HypervisorName);

        if (strcmp(HypervisorName, "KVM") == 0) {
            printf("  - KVM detected, virtio devices likely available\n");
        } else if (strcmp(HypervisorName, "Microsoft Hyper-V") == 0) {
            printf("  - Hyper-V detected, VMBus devices available\n");
        } else if (strcmp(HypervisorName, "VMware") == 0) {
            printf("  - VMware detected, VMXNET3/PVSCSI devices may be available\n");
        } else if (strcmp(HypervisorName, "Xen") == 0) {
            printf("  - Xen detected, XenBus devices available\n");
        }
    } else {
        printf("Not running under a hypervisor (bare metal)\n");
    }
}

int
main(
    int   argc,
    char *argv[]
    )
{
    IO_RETURN Status;

    printf("=== IOKit Virtualization Family - virtio Example ===\n\n");

    // Initialize virtualization subsystem
    Status = VirtInitialize();
    if (Status != IO_SUCCESS) {
        printf("Failed to initialize virtualization subsystem: 0x%08X\n", Status);
        return 1;
    }

    // Run examples
    Example_DetectHypervisor();

    // Note: In a real scenario, you would get pPCIDevice from device enumeration
    // Example_VirtioNet(pPCIDevice);
    // Example_VirtioBlk(pPCIDevice);

    printf("\n=== virtio Device Types Supported ===\n");
    printf("  - virtio-net:     Network device\n");
    printf("  - virtio-blk:     Block storage\n");
    printf("  - virtio-scsi:    SCSI host\n");
    printf("  - virtio-console: Console/serial\n");
    printf("  - virtio-balloon: Memory ballooning\n");
    printf("  - virtio-gpu:     Graphics adapter\n");
    printf("  - virtio-fs:      Filesystem sharing\n");
    printf("  - virtio-vsock:   Socket communication\n");
    printf("  - virtio-crypto:  Cryptographic acceleration\n");
    printf("  - virtio-mem:     Memory hotplug\n");
    printf("  - virtio-pmem:    Persistent memory\n");
    printf("  - virtio-iommu:   IOMMU\n");

    // Shutdown
    VirtShutdown();

    printf("\n=== Example Complete ===\n");
    return 0;
}
