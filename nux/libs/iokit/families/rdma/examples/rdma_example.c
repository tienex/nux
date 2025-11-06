/**
 * @file rdma_example.c
 * @brief RDMA Driver Usage Example
 *
 * This example demonstrates:
 * - RDMA device initialization and detection
 * - InfiniBand, RoCE, iWARP, and OmniPath transport identification
 * - Protection domain and queue pair creation
 * - Memory registration for RDMA operations
 * - Send/receive operations
 * - RDMA Read/Write operations
 * - Atomic operations (Compare-and-Swap, Fetch-and-Add)
 * - Completion queue polling
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/rdma/rdma.h>
#include <iokit/families/pcie/pcie.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief Helper: Get transport type name
 */
static const char *
GetTransportTypeName(RDMA_TRANSPORT_TYPE Type)
{
    switch (Type) {
        case RDMA_TRANSPORT_IB:         return "InfiniBand";
        case RDMA_TRANSPORT_ROCE_V1:    return "RoCE v1";
        case RDMA_TRANSPORT_ROCE_V2:    return "RoCE v2";
        case RDMA_TRANSPORT_IWARP:      return "iWARP";
        case RDMA_TRANSPORT_OMNIPATH:   return "Intel OmniPath";
        default:                        return "Unknown";
    }
}

/**
 * @brief Helper: Get queue pair type name
 */
static const char *
GetQPTypeName(RDMA_QP_TYPE Type)
{
    switch (Type) {
        case RDMA_QP_RC:        return "Reliable Connected (RC)";
        case RDMA_QP_UC:        return "Unreliable Connected (UC)";
        case RDMA_QP_UD:        return "Unreliable Datagram (UD)";
        case RDMA_QP_RAW:       return "Raw Packet";
        case RDMA_QP_XRC_SEND:  return "XRC Send";
        case RDMA_QP_XRC_RECV:  return "XRC Receive";
        default:                return "Unknown";
    }
}

/**
 * @brief Helper: Get port state name
 */
static const char *
GetPortStateName(IB_PORT_STATE State)
{
    switch (State) {
        case IB_PORT_DOWN:          return "Down";
        case IB_PORT_INIT:          return "Initializing";
        case IB_PORT_ARMED:         return "Armed";
        case IB_PORT_ACTIVE:        return "Active";
        case IB_PORT_ACTIVE_DEFER:  return "Active (Deferred)";
        default:                    return "Unknown";
    }
}

/**
 * @brief Example 1: Detect and initialize RDMA devices
 */
static int
Example_RDMA_Detection(void)
{
    IO_RETURN Status;
    IIOPCIDevice *pPCIDevices[256];
    UINT32 uDeviceCount;
    UINT32 i, j;
    UINT32 uRDMACount = 0;

    printf("\n");
    printf("========================================\n");
    printf("  Example 1: RDMA Device Detection\n");
    printf("========================================\n\n");

    // Initialize PCI subsystem
    printf("Scanning PCI bus for RDMA devices...\n\n");
    uDeviceCount = 256;
    Status = PCIScanBus(0, pPCIDevices, &uDeviceCount);
    if (Status != IO_SUCCESS) {
        printf("ERROR: PCI bus scan failed (status=0x%08X)\n", Status);
        return -1;
    }

    // Look for RDMA-capable devices
    for (i = 0; i < uDeviceCount; i++) {
        PCI_DEVICE_INFO PCIInfo;
        IIORDMADevice *pRDMADevice;

        Status = IIOPCIDevice_GetDeviceInfo(pPCIDevices[i], &PCIInfo);
        if (Status != IO_SUCCESS) {
            continue;
        }

        // Check if this is a known RDMA vendor
        if (PCIInfo.VendorID == RDMA_VENDOR_MELLANOX ||
            PCIInfo.VendorID == RDMA_VENDOR_INTEL ||
            PCIInfo.VendorID == RDMA_VENDOR_CHELSIO ||
            PCIInfo.VendorID == RDMA_VENDOR_BROADCOM ||
            PCIInfo.VendorID == RDMA_VENDOR_QLOGIC ||
            PCIInfo.VendorID == RDMA_VENDOR_CISCO) {

            printf("RDMA Device %u:\n", uRDMACount);
            printf("  Location:    %02X:%02X.%X\n",
                   PCIInfo.Location.Bus, PCIInfo.Location.Device,
                   PCIInfo.Location.Function);
            printf("  Vendor/Device: 0x%04X:0x%04X\n",
                   PCIInfo.VendorID, PCIInfo.DeviceID);

            // Try to create RDMA device
            Status = RDMADeviceCreate((IIOService *)pPCIDevices[i], &pRDMADevice);
            if (Status == IO_SUCCESS) {
                RDMA_DEVICE_INFO DeviceInfo;

                // Start the device
                pRDMADevice->lpVtbl->Start(pRDMADevice, (IIOService *)pPCIDevices[i]);

                // Get device information
                Status = pRDMADevice->lpVtbl->GetDeviceInfo(pRDMADevice, &DeviceInfo);
                if (Status == IO_SUCCESS) {
                    printf("  Vendor:      %s\n", DeviceInfo.VendorName);
                    printf("  Model:       %s\n", DeviceInfo.ModelName);
                    printf("  Transport:   %s\n", GetTransportTypeName(DeviceInfo.TransportType));
                    printf("  Ports:       %u\n", DeviceInfo.PhysPortCount);
                    printf("  FW Version:  0x%08X\n", DeviceInfo.FWVersion);
                    printf("  Max QPs:     %u\n", DeviceInfo.MaxQP);
                    printf("  Max CQs:     %u\n", DeviceInfo.MaxCQ);
                    printf("  Max MRs:     %u\n", DeviceInfo.MaxMR);

                    // Query port information
                    for (j = 1; j <= DeviceInfo.PhysPortCount; j++) {
                        RDMA_PORT_INFO PortInfo;

                        Status = pRDMADevice->lpVtbl->GetPortInfo(pRDMADevice, j, &PortInfo);
                        if (Status == IO_SUCCESS) {
                            UINT32 uSpeedGbps = RDMAGetLinkSpeedGbps(PortInfo.ActiveSpeed);
                            UINT32 uLanes = RDMAGetLinkWidthLanes(PortInfo.ActiveWidth);
                            UINT32 uTotalGbps = uSpeedGbps * uLanes;

                            printf("\n  Port %u:\n", j);
                            printf("    State:     %s\n", GetPortStateName(PortInfo.State));
                            printf("    Speed:     %u Gbps (%ux%u)\n",
                                   uTotalGbps, uLanes, uSpeedGbps);
                            printf("    MTU:       %u bytes\n", RDMAGetMTUSize(PortInfo.ActiveMTU));
                            printf("    LID:       0x%04X\n", PortInfo.LID);
                            printf("    GID:       %02X%02X:%02X%02X:%02X%02X:%02X%02X:"
                                                  "%02X%02X:%02X%02X:%02X%02X:%02X%02X\n",
                                   PortInfo.GID.Raw[0], PortInfo.GID.Raw[1],
                                   PortInfo.GID.Raw[2], PortInfo.GID.Raw[3],
                                   PortInfo.GID.Raw[4], PortInfo.GID.Raw[5],
                                   PortInfo.GID.Raw[6], PortInfo.GID.Raw[7],
                                   PortInfo.GID.Raw[8], PortInfo.GID.Raw[9],
                                   PortInfo.GID.Raw[10], PortInfo.GID.Raw[11],
                                   PortInfo.GID.Raw[12], PortInfo.GID.Raw[13],
                                   PortInfo.GID.Raw[14], PortInfo.GID.Raw[15]);
                        }
                    }
                }

                pRDMADevice->lpVtbl->Release(pRDMADevice);
                uRDMACount++;
            }

            printf("\n");
        }

        IIOPCIDevice_Release(pPCIDevices[i]);
    }

    if (uRDMACount == 0) {
        printf("No RDMA devices found\n");
    } else {
        printf("Found %u RDMA device(s)\n", uRDMACount);
    }

    return 0;
}

/**
 * @brief Example 2: Create and manage queue pairs
 */
static int
Example_RDMA_QueuePairs(IIORDMADevice *pDevice)
{
    IO_RETURN Status;
    UINT32 uPDHandle;
    UINT32 uSendCQHandle, uRecvCQHandle;
    IIORDMAConnection *pQP;
    RDMA_QP_INFO QPInfo;

    printf("\n");
    printf("========================================\n");
    printf("  Example 2: Queue Pair Management\n");
    printf("========================================\n\n");

    if (pDevice == NULL) {
        printf("No RDMA device available\n");
        return 0;
    }

    // Allocate protection domain
    printf("Allocating Protection Domain...\n");
    Status = pDevice->lpVtbl->AllocPD(pDevice, &uPDHandle);
    if (Status != IO_SUCCESS) {
        printf("ERROR: Failed to allocate PD (status=0x%08X)\n", Status);
        return -1;
    }
    printf("  PD Handle: 0x%X\n\n", uPDHandle);

    // Create completion queues
    printf("Creating Completion Queues...\n");
    Status = pDevice->lpVtbl->CreateCQ(pDevice, 1024, &uSendCQHandle);
    if (Status != IO_SUCCESS) {
        printf("ERROR: Failed to create send CQ (status=0x%08X)\n", Status);
        return -1;
    }
    printf("  Send CQ Handle: 0x%X\n", uSendCQHandle);

    Status = pDevice->lpVtbl->CreateCQ(pDevice, 1024, &uRecvCQHandle);
    if (Status != IO_SUCCESS) {
        printf("ERROR: Failed to create recv CQ (status=0x%08X)\n", Status);
        return -1;
    }
    printf("  Recv CQ Handle: 0x%X\n\n", uRecvCQHandle);

    // Create queue pair (Reliable Connected)
    printf("Creating Queue Pair (RC)...\n");
    Status = pDevice->lpVtbl->CreateQP(pDevice, uPDHandle, RDMA_QP_RC,
                                       256, 256, &pQP);
    if (Status != IO_SUCCESS) {
        printf("ERROR: Failed to create QP (status=0x%08X)\n", Status);
        return -1;
    }

    // Query QP information
    Status = pDevice->lpVtbl->QueryQP(pDevice, pQP, &QPInfo);
    if (Status == IO_SUCCESS) {
        printf("  QP Number:      %u\n", QPInfo.QPNum);
        printf("  QP Type:        %s\n", GetQPTypeName(QPInfo.Type));
        printf("  Send Depth:     %u\n", QPInfo.SendQueueDepth);
        printf("  Recv Depth:     %u\n", QPInfo.RecvQueueDepth);
        printf("  Max Inline:     %u bytes\n", QPInfo.MaxInlineSend);
        printf("  Max Send SGE:   %u\n", QPInfo.MaxSendSGE);
        printf("  Max Recv SGE:   %u\n\n", QPInfo.MaxRecvSGE);
    }

    // Modify QP state: RESET -> INIT -> RTR -> RTS
    printf("Transitioning QP state...\n");
    Status = pDevice->lpVtbl->ModifyQP(pDevice, pQP, RDMA_QPS_INIT, NULL);
    if (Status == IO_SUCCESS) {
        printf("  State: RESET -> INIT\n");
    }

    Status = pDevice->lpVtbl->ModifyQP(pDevice, pQP, RDMA_QPS_RTR, NULL);
    if (Status == IO_SUCCESS) {
        printf("  State: INIT -> RTR (Ready To Receive)\n");
    }

    Status = pDevice->lpVtbl->ModifyQP(pDevice, pQP, RDMA_QPS_RTS, NULL);
    if (Status == IO_SUCCESS) {
        printf("  State: RTR -> RTS (Ready To Send)\n");
    }

    // Clean up
    printf("\nCleaning up...\n");
    pDevice->lpVtbl->DestroyQP(pDevice, pQP);
    pDevice->lpVtbl->DestroyCQ(pDevice, uSendCQHandle);
    pDevice->lpVtbl->DestroyCQ(pDevice, uRecvCQHandle);
    pDevice->lpVtbl->DeallocPD(pDevice, uPDHandle);

    return 0;
}

/**
 * @brief Example 3: Memory registration and RDMA operations
 */
static int
Example_RDMA_MemoryOperations(IIORDMADevice *pDevice)
{
    IO_RETURN Status;
    UINT32 uPDHandle;
    IIORDMAMemoryRegion *pMR;
    IIORDMAConnection *pQP;
    RDMA_MR_INFO MRInfo;
    RDMA_WR SendWR;
    RDMA_SGE SGE;
    UINT8 *pBuffer;
    UINT32 cbBufferSize = 4096;

    printf("\n");
    printf("========================================\n");
    printf("  Example 3: Memory Registration\n");
    printf("========================================\n\n");

    if (pDevice == NULL) {
        printf("No RDMA device available\n");
        return 0;
    }

    // Allocate buffer
    pBuffer = (UINT8 *)malloc(cbBufferSize);
    if (pBuffer == NULL) {
        printf("ERROR: Failed to allocate buffer\n");
        return -1;
    }
    memset(pBuffer, 0xAA, cbBufferSize);

    // Allocate PD
    Status = pDevice->lpVtbl->AllocPD(pDevice, &uPDHandle);
    if (Status != IO_SUCCESS) {
        free(pBuffer);
        return -1;
    }

    // Register memory
    printf("Registering memory region...\n");
    printf("  Address:     %p\n", pBuffer);
    printf("  Size:        %u bytes\n", cbBufferSize);

    Status = pDevice->lpVtbl->RegisterMemory(pDevice, uPDHandle, pBuffer,
                                             cbBufferSize,
                                             RDMA_ACCESS_LOCAL_WRITE |
                                             RDMA_ACCESS_REMOTE_WRITE |
                                             RDMA_ACCESS_REMOTE_READ,
                                             &pMR);
    if (Status != IO_SUCCESS) {
        printf("ERROR: Failed to register memory (status=0x%08X)\n", Status);
        pDevice->lpVtbl->DeallocPD(pDevice, uPDHandle);
        free(pBuffer);
        return -1;
    }

    // Get MR information
    Status = pMR->lpVtbl->GetInfo(pMR, &MRInfo);
    if (Status == IO_SUCCESS) {
        printf("\nMemory Region Info:\n");
        printf("  LKey:        0x%08X\n", MRInfo.LKey);
        printf("  RKey:        0x%08X\n", MRInfo.RKey);
        printf("  Access:      0x%08X\n", MRInfo.AccessFlags);
        if (MRInfo.AccessFlags & RDMA_ACCESS_LOCAL_WRITE)
            printf("    - Local Write\n");
        if (MRInfo.AccessFlags & RDMA_ACCESS_REMOTE_WRITE)
            printf("    - Remote Write\n");
        if (MRInfo.AccessFlags & RDMA_ACCESS_REMOTE_READ)
            printf("    - Remote Read\n");
    }

    // Create QP for operations
    printf("\nCreating Queue Pair for operations...\n");
    Status = pDevice->lpVtbl->CreateQP(pDevice, uPDHandle, RDMA_QP_RC,
                                       128, 128, &pQP);
    if (Status == IO_SUCCESS) {
        UINT32 uLKey;

        pMR->lpVtbl->GetLKey(pMR, &uLKey);

        // Prepare send work request
        memset(&SendWR, 0, sizeof(SendWR));
        SendWR.WorkRequestID = 0x1234;
        SendWR.Opcode = RDMA_WR_SEND;
        SendWR.Flags = RDMA_SEND_SIGNALED;
        SendWR.pSGList = &SGE;
        SendWR.NumSGE = 1;

        SGE.Address = (UINT64)(UINTN)pBuffer;
        SGE.Length = 1024;
        SGE.LKey = uLKey;

        // Post send (simulated)
        printf("\nPosting Send operation...\n");
        printf("  WR ID:       0x%llX\n", SendWR.WorkRequestID);
        printf("  Opcode:      SEND\n");
        printf("  Length:      %u bytes\n", SGE.Length);
        printf("  Flags:       SIGNALED\n");

        Status = pQP->lpVtbl->PostSend(pQP, &SendWR, NULL);
        if (Status == IO_SUCCESS) {
            printf("  Status:      Posted successfully\n");
        }

        // Demonstrate RDMA Write
        printf("\nPosting RDMA Write operation...\n");
        Status = pQP->lpVtbl->PostWrite(pQP, 0xDEADBEEF00000000ULL,
                                        0x12345678, &SGE, 1, 0x5678);
        if (Status == IO_SUCCESS) {
            printf("  Remote Addr: 0xDEADBEEF00000000\n");
            printf("  Remote Key:  0x12345678\n");
            printf("  WR ID:       0x5678\n");
            printf("  Status:      Posted successfully\n");
        }

        // Demonstrate RDMA Read
        printf("\nPosting RDMA Read operation...\n");
        Status = pQP->lpVtbl->PostRead(pQP, 0xCAFEBABE00000000ULL,
                                       0x87654321, &SGE, 0x9ABC);
        if (Status == IO_SUCCESS) {
            printf("  Remote Addr: 0xCAFEBABE00000000\n");
            printf("  Remote Key:  0x87654321\n");
            printf("  WR ID:       0x9ABC\n");
            printf("  Status:      Posted successfully\n");
        }

        // Demonstrate Atomic operation
        printf("\nPosting Atomic Compare-and-Swap...\n");
        Status = pQP->lpVtbl->PostAtomic(pQP, 0x1000000000000000ULL,
                                         0xABCDEF00, 42, 100, &SGE,
                                         TRUE, 0xDEF0);
        if (Status == IO_SUCCESS) {
            printf("  Remote Addr: 0x1000000000000000\n");
            printf("  Compare:     42\n");
            printf("  Swap:        100\n");
            printf("  WR ID:       0xDEF0\n");
            printf("  Status:      Posted successfully\n");
        }

        pDevice->lpVtbl->DestroyQP(pDevice, pQP);
    }

    // Clean up
    printf("\nDeregistering memory...\n");
    pDevice->lpVtbl->DeregisterMemory(pDevice, pMR);
    pDevice->lpVtbl->DeallocPD(pDevice, uPDHandle);
    free(pBuffer);

    return 0;
}

/**
 * @brief Example 4: Completion queue polling
 */
static int
Example_RDMA_Completions(IIORDMADevice *pDevice)
{
    IO_RETURN Status;
    UINT32 uCQHandle;
    RDMA_WC Completions[32];
    UINT32 uNumCompletions;

    printf("\n");
    printf("========================================\n");
    printf("  Example 4: Completion Polling\n");
    printf("========================================\n\n");

    if (pDevice == NULL) {
        printf("No RDMA device available\n");
        return 0;
    }

    // Create CQ
    printf("Creating Completion Queue...\n");
    Status = pDevice->lpVtbl->CreateCQ(pDevice, 256, &uCQHandle);
    if (Status != IO_SUCCESS) {
        printf("ERROR: Failed to create CQ (status=0x%08X)\n", Status);
        return -1;
    }
    printf("  CQ Handle: 0x%X\n\n", uCQHandle);

    // Poll for completions (simulated - no actual operations pending)
    printf("Polling for completions...\n");
    Status = pDevice->lpVtbl->PollCQ(pDevice, uCQHandle, Completions,
                                     32, &uNumCompletions);
    if (Status == IO_SUCCESS) {
        printf("  Polled %u completion(s)\n", uNumCompletions);

        // In a real scenario, we would process completions here
        for (UINT32 i = 0; i < uNumCompletions; i++) {
            printf("\n  Completion %u:\n", i);
            printf("    WR ID:     0x%llX\n", Completions[i].WorkRequestID);
            printf("    Status:    %s\n",
                   Completions[i].Status == RDMA_WC_SUCCESS ? "SUCCESS" : "ERROR");
            printf("    Opcode:    %u\n", Completions[i].Opcode);
            printf("    Byte Len:  %u\n", Completions[i].ByteLen);
        }
    }

    // Clean up
    pDevice->lpVtbl->DestroyCQ(pDevice, uCQHandle);

    return 0;
}

/**
 * @brief Main function - Run all examples
 */
int
main(void)
{
    IIORDMADevice *pFirstDevice = NULL;
    IO_RETURN Status;
    IIOPCIDevice *pPCIDevices[256];
    UINT32 uDeviceCount;
    UINT32 i;

    printf("\n");
    printf("================================================\n");
    printf("  RDMA Driver Examples\n");
    printf("  InfiniBand, RoCE, iWARP, OmniPath\n");
    printf("================================================\n");

    // Run detection example
    Example_RDMA_Detection();

    // Try to get first RDMA device for other examples
    uDeviceCount = 256;
    Status = PCIScanBus(0, pPCIDevices, &uDeviceCount);
    if (Status == IO_SUCCESS) {
        for (i = 0; i < uDeviceCount; i++) {
            PCI_DEVICE_INFO PCIInfo;

            Status = IIOPCIDevice_GetDeviceInfo(pPCIDevices[i], &PCIInfo);
            if (Status == IO_SUCCESS &&
                (PCIInfo.VendorID == RDMA_VENDOR_MELLANOX ||
                 PCIInfo.VendorID == RDMA_VENDOR_INTEL ||
                 PCIInfo.VendorID == RDMA_VENDOR_CHELSIO)) {

                Status = RDMADeviceCreate((IIOService *)pPCIDevices[i], &pFirstDevice);
                if (Status == IO_SUCCESS) {
                    pFirstDevice->lpVtbl->Start(pFirstDevice, (IIOService *)pPCIDevices[i]);
                    break;
                }
            }
        }

        // Release all PCI devices
        for (i = 0; i < uDeviceCount; i++) {
            IIOPCIDevice_Release(pPCIDevices[i]);
        }
    }

    // Run other examples with first device
    if (pFirstDevice != NULL) {
        Example_RDMA_QueuePairs(pFirstDevice);
        Example_RDMA_MemoryOperations(pFirstDevice);
        Example_RDMA_Completions(pFirstDevice);

        pFirstDevice->lpVtbl->Release(pFirstDevice);
    }

    printf("\n");
    printf("================================================\n");
    printf("  All examples completed\n");
    printf("================================================\n");
    printf("\n");

    return 0;
}
