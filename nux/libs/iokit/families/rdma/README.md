# RDMA Family - Remote Direct Memory Access and InfiniBand

## Overview

The RDMA family provides comprehensive support for high-performance interconnect technologies including InfiniBand, RoCE (RDMA over Converged Ethernet), iWARP (RDMA over TCP/IP), and Intel OmniPath Architecture. This family implements the RDMA verbs interface for zero-copy, kernel-bypass networking.

## Supported Technologies

### 1. InfiniBand (IB)
Native InfiniBand transport with comprehensive speed support:
- **SDR** (Single Data Rate) - 2.5 Gbps per lane
- **DDR** (Double Data Rate) - 5 Gbps per lane
- **QDR** (Quad Data Rate) - 10 Gbps per lane
- **FDR10** - 10.3125 Gbps per lane
- **FDR** (Fourteen Data Rate) - 14.0625 Gbps per lane
- **EDR** (Enhanced Data Rate) - 25.78125 Gbps per lane
- **HDR** (High Data Rate) - 50 Gbps per lane
- **NDR** (Next Data Rate) - 100 Gbps per lane
- **XDR** (eXtended Data Rate) - 250 Gbps per lane

Link widths: 1x, 4x, 8x, 12x

### 2. RoCE (RDMA over Converged Ethernet)
- **RoCE v1**: Layer 2 only (non-routable)
- **RoCE v2**: UDP/IP encapsulation (routable)

Features:
- Priority Flow Control (PFC)
- Enhanced Transmission Selection (ETS)
- Lossless Ethernet operation

### 3. iWARP
RDMA over TCP/IP stack:
- **DDP** (Direct Data Placement)
- **RDMAP** (Remote Direct Memory Access Protocol)
- **MPA** (Marker PDU Aligned Framing)

Runs over standard TCP/IP networks without special switches.

### 4. Intel OmniPath Architecture
Intel's proprietary HPC fabric:
- 100 Gbps Gen 1
- Optimized for MPI and HPC workloads
- Advanced congestion control

## RDMA Verbs

### Queue Pairs (QP)
- **RC** (Reliable Connected) - Connection-oriented, reliable delivery
- **UC** (Unreliable Connected) - Connection-oriented, no guarantees
- **UD** (Unreliable Datagram) - Connectionless datagram service
- **XRC** (eXtended Reliable Connection) - Scalable RC
- **Raw** - Raw packet interface

### Operations
- **Send/Receive** - Traditional message passing
- **RDMA Read** - Remote memory read (zero-copy)
- **RDMA Write** - Remote memory write (zero-copy)
- **Atomic Operations**:
  - Compare-and-Swap (CAS)
  - Fetch-and-Add
- **Memory Windows** - Dynamic memory registration
- **Fast Memory Registration (FRMR)** - Optimized registration

### Memory Management
- **Memory Regions (MR)** - Registered memory for RDMA access
- **Protection Domains (PD)** - Security isolation
- **Memory Windows (MW)** - Dynamic virtual address spaces
- **Access Rights**: Local Write, Remote Read, Remote Write, Remote Atomic

### Completion Model
- **Completion Queues (CQ)** - Asynchronous completion notification
- **Work Completions (WC)** - Operation results
- **Signaled vs Unsignaled** - Completion generation control

## Supported Hardware

### Mellanox/NVIDIA ConnectX Series (30+ adapters)

#### ConnectX-2
- ConnectX-2 VPI (IB QDR / 10GbE)

#### ConnectX-3
- ConnectX-3 VPI (IB FDR / 40GbE)
- ConnectX-3 Pro VPI
- ConnectX-3 EN (Ethernet only)

#### ConnectX-4
- ConnectX-4 VPI (IB EDR / 100GbE)
- ConnectX-4 Lx (IB EDR / 50GbE, 25GbE)
- ConnectX-4 EN

#### ConnectX-5
- ConnectX-5 VPI (IB EDR / 100GbE)
- ConnectX-5 Ex VPI
- ConnectX-5 EN

#### ConnectX-6
- ConnectX-6 VPI (IB HDR / 200GbE)
- ConnectX-6 Dx VPI
- ConnectX-6 Lx EN (100GbE)
- ConnectX-6 EN

#### ConnectX-7
- ConnectX-7 VPI (IB NDR / 400GbE)
- ConnectX-7 Ex VPI
- ConnectX-7 EN

### Intel TrueScale and OmniPath (10+ adapters)

#### TrueScale
- QLE7340 HCA (Single port QDR)
- QLE7342 HCA (Dual port QDR)
- Edge Switch 12200

#### OmniPath
- HFI Silicon 100 Series
- HFI Integrated
- Fabric Switch (48-port)
- Edge Switch (48-port)
- Director Switch (768-port)

### Chelsio iWARP NICs (15+ models)

#### T4 Series
- T420-CR, T420-BCH, T440-CH
- T420-SO, T420-CX

#### T5 Series
- T520-CR, T520-BCH, T540-CH
- T520-SO, T522-CR

#### T6 Series
- T6225-CR, T6225-SO-CR
- T6425-CR
- T62100-LP-CR, T62100-SO-CR

### Broadcom/QLogic (15+ models)

#### QLogic InfiniPath
- PE-800 (DDR)
- QLE7340 (QDR, single port)
- QLE7342 (QDR, dual port)

#### Broadcom NetXtreme RoCE
- BCM57301 NetXtreme-C (10Gb)
- BCM57412 NetXtreme-E (10Gb)
- BCM57414 NetXtreme-E (10Gb/25Gb)
- BCM57416 NetXtreme-E (10Gb/50Gb)
- BCM57417 NetXtreme-E (10Gb/25Gb)

### Cisco VIC (Virtual Interface Card) (6+ models)
- VIC 1280 RoCE (40Gb)
- VIC 1340 RoCE (40Gb)
- VIC 1380 RoCE (40Gb)
- VIC 1385 RoCE (40Gb)
- VIC 1387 RoCE (100Gb)
- VIC 1457 RoCE (40Gb quad-port)

## Architecture

### InfiniBand Components
- **HCA** (Host Channel Adapter) - Network interface
- **Switch** - Fabric switching element
- **Router** - Inter-subnet routing
- **Gateway** - Protocol translation

### Port States
1. **Down** - Port is inactive
2. **Init** - Port is initializing
3. **Armed** - Port is ready but not active
4. **Active** - Port is fully operational
5. **Active Defer** - Active with deferred training

### Addressing
- **LID** (Local Identifier) - 16-bit local address
- **GID** (Global Identifier) - 128-bit global address
  - Subnet Prefix (64 bits)
  - Interface ID (64 bits)
- **Service Level** (SL) - QoS priority (0-15)
- **Virtual Lanes** - Traffic segregation

### Management
- **Subnet Management** (SM) - Fabric configuration
- **Subnet Administration** (SA) - Path resolution
- **Performance Management** (PM) - Statistics collection
- **MAD** (Management Datagram) - Management messages

## Interface Definitions

### IIORDMADevice
Main device interface for RDMA adapters:
- `GetDeviceInfo()` - Query device capabilities
- `GetPortInfo()` - Query port status and properties
- `AllocPD()` / `DeallocPD()` - Protection domain management
- `CreateQP()` / `DestroyQP()` - Queue pair lifecycle
- `CreateCQ()` / `DestroyCQ()` - Completion queue management
- `PollCQ()` - Poll for completions
- `RegisterMemory()` / `DeregisterMemory()` - Memory registration
- `CreateMW()` / `BindMW()` - Memory window operations

### IIORDMAConnection
Queue pair interface for RDMA operations:
- `PostSend()` - Post send work request
- `PostReceive()` - Post receive work request
- `PostRead()` - RDMA Read operation
- `PostWrite()` - RDMA Write operation
- `PostAtomic()` - Atomic operations (CAS, FetchAdd)
- `GetState()` / `ModifyState()` - QP state management

### IIORDMAMemoryRegion
Registered memory region interface:
- `GetInfo()` - Get MR properties
- `GetLKey()` - Get local key
- `GetRKey()` - Get remote key

## Key Structures

### RDMA_DEVICE_INFO
Comprehensive device information including:
- Vendor/model identification
- Transport type (IB/RoCE/iWARP/OPA)
- Hardware/firmware versions
- Port count and capabilities
- Resource limits (QPs, CQs, MRs, etc.)
- Feature flags

### RDMA_PORT_INFO
Port-specific information:
- Link state and physical state
- Active speed and width
- MTU configuration
- GID and LID addressing
- Performance counters

### RDMA_QP_INFO
Queue pair properties:
- QP number and type
- Current state
- Queue depths
- SGE limits

### RDMA_WR (Work Request)
Operation descriptor:
- Opcode (Send, Recv, Read, Write, Atomic)
- Scatter-gather list
- Remote addressing (for RDMA ops)
- Flags (fence, signaled, inline)

### RDMA_WC (Work Completion)
Operation result:
- Status (success or error code)
- Completed opcode
- Bytes transferred
- Immediate data (if present)

### RDMA_SGE (Scatter-Gather Element)
Memory descriptor:
- Virtual address
- Length
- Local key (LKey)

## Device Capabilities

Supported capability flags:
- Memory windows (Type 2A, 2B)
- Atomic operations
- Automatic path migration
- XRC (eXtended Reliable Connection)
- SRQ (Shared Receive Queue)
- Memory management extensions
- On-demand paging
- Flow steering
- Signature handover
- IP checksum offload

## Performance Features

### Zero-Copy Networking
- Direct memory access without CPU copy
- Kernel bypass for data path
- User-space queue pair posting

### Multi-Queue Support
- Multiple queue pairs per device
- Per-CPU queue assignment
- Parallel operation processing

### Interrupt Coalescing
- Configurable CQ notification
- Signaled vs unsignaled completions
- Adaptive interrupt moderation

### Inline Data
- Small message optimization
- Up to 512 bytes inline
- Reduced DMA operations

## Usage Example

```c
IIORDMADevice *pDevice;
IIORDMAConnection *pQP;
IIORDMAMemoryRegion *pMR;
RDMA_DEVICE_INFO DeviceInfo;
RDMA_PORT_INFO PortInfo;
UINT32 uPDHandle;
UINT32 uCQHandle;
VOID *pBuffer;
RDMA_WR SendWR;
RDMA_SGE SGE;

// Create device
RDMADeviceCreate(pPCIDevice, &pDevice);

// Start device
pDevice->lpVtbl->Start(pDevice, pProvider);

// Query device
pDevice->lpVtbl->GetDeviceInfo(pDevice, &DeviceInfo);
pDevice->lpVtbl->GetPortInfo(pDevice, 1, &PortInfo);

// Allocate protection domain
pDevice->lpVtbl->AllocPD(pDevice, &uPDHandle);

// Create completion queue
pDevice->lpVtbl->CreateCQ(pDevice, 1024, &uCQHandle);

// Create queue pair
pDevice->lpVtbl->CreateQP(pDevice, uPDHandle, RDMA_QP_RC,
                          256, 256, &pQP);

// Register memory
pDevice->lpVtbl->RegisterMemory(pDevice, uPDHandle, pBuffer,
                                4096, RDMA_ACCESS_LOCAL_WRITE,
                                &pMR);

// Post send operation
memset(&SendWR, 0, sizeof(SendWR));
SendWR.Opcode = RDMA_WR_SEND;
SendWR.WorkRequestID = 1;
SendWR.pSGList = &SGE;
SendWR.NumSGE = 1;
SendWR.Flags = RDMA_SEND_SIGNALED;

pQP->lpVtbl->PostSend(pQP, &SendWR, NULL);
```

## Build System Integration

The RDMA family integrates with the IOKit build system:

```makefile
LIB = iokit_rdma

SRCS = sources/rdma.c
INCS = include/rdma.h

CFLAGS += -I$(top_srcdir)/nux/libs/iokit/families/rdma/include
CFLAGS += -I$(top_srcdir)/nux/libs/iokit/families/pcie/include
```

## Testing and Validation

### Loopback Testing
- Create two QPs on same device
- Exchange addressing information
- Post send/receive operations
- Verify completions

### Performance Testing
- Latency measurement (ping-pong)
- Bandwidth measurement (streaming)
- Message rate (small messages)
- CPU utilization

### Error Handling
- Queue overflow conditions
- Memory protection violations
- Network timeout scenarios
- State transition validation

## Future Enhancements

1. **Advanced Features**
   - RDMA-CM (Connection Manager)
   - Multicast support
   - Flow steering and RSS
   - SR-IOV virtualization

2. **Protocol Support**
   - NVMe over Fabrics (NVMe-oF)
   - iSER (iSCSI Extensions for RDMA)
   - SMB Direct
   - RDS (Reliable Datagram Sockets)

3. **Performance Optimizations**
   - Adaptive polling
   - Doorbell batching
   - Memory pool management
   - NUMA awareness

4. **Management**
   - SNMP MIB support
   - Performance monitoring
   - Diagnostic capabilities
   - Firmware update support

## References

- InfiniBand Architecture Specification
- RDMA Consortium Specifications
- RoCE v2 Protocol Specification
- iWARP Protocol Suite (RFC 5040-5045)
- Intel OmniPath Architecture Specification

## Copyright

Copyright (c) 2025 NUX Project. All rights reserved.
