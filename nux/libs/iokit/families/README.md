
# IOKit Device Families

This directory contains device family drivers for the IOKit framework.

## Available Families

### PCIe Family (`pcie.h`, `pcie.c`)

Comprehensive PCI/PCI-X/PCIe driver supporting:
- **Legacy PCI 32-bit** (PCI 2.x/3.0)
- **PCI-X** (PCI Extended)
- **PCI 64-bit**
- **PCIe** 1.x/2.x/3.x/4.x/5.x

**Features:**
- Configuration space access (Mechanism #1 and PCIe ECAM)
- BAR (Base Address Register) mapping and management
- MSI and MSI-X interrupt support
- PCI capability discovery
- Device enumeration and hot-plug
- Bus mastering control
- Memory and I/O space management

**Interfaces:**
- `IIOPCIDevice` - Represents a single PCI/PCIe device
- `IIOPCIBridge` - Represents a PCI-to-PCI bridge

### Thunderbolt Family (`thunderbolt.h`, `thunderbolt.c`)

Complete Thunderbolt 1/2/3/4 support:
- **Thunderbolt 1** (Light Peak): 10 Gbps, Mini DisplayPort
- **Thunderbolt 2**: 20 Gbps, DisplayPort 1.2
- **Thunderbolt 3**: 40 Gbps, USB-C, USB 3.1, Power Delivery
- **Thunderbolt 4**: 40 Gbps, USB4, PCIe 4.0

**Features:**
- Hot-plug detection and management
- Security levels (None, User, Secure, DP-only, USB-only, No-PCIe)
- Device authorization (one-time and persistent)
- Tunnel management (PCIe, DisplayPort, USB 3.x, P2P)
- Daisy-chaining support (up to 6 devices)
- Firmware updates
- Wake-on-Thunderbolt
- IOMMU/VT-d integration

**Interfaces:**
- `IIOThunderboltController` - Represents a Thunderbolt host controller
- `IIOThunderboltDevice` - Represents a connected Thunderbolt device

## Thread Safety and Multithreaded Operation

### Overview

All device discovery and enumeration operations in the IOKit families are designed to support multithreaded execution. This allows:
- Parallel bus scanning across multiple buses
- Concurrent device initialization
- Asynchronous hot-plug event handling
- Background device authentication (Thunderbolt)

### Thread-Safe Operations

#### PCIe Family

**Fully Thread-Safe Operations:**
- `PCIScanBus()` - Can scan different buses concurrently
- `PCIDeviceCreate()` - Safe when creating different devices
- `IIOPCIDevice_ConfigRead()` - Safe for different devices
- `IIOPCIDevice_ConfigWrite()` - Safe for different devices
- `IIOPCIDevice_MapBAR()` - Safe for different devices

**Serialization Required:**
- Configuration space access to the same device requires synchronization
- BAR mapping of the same BAR on the same device

**Example - Parallel Bus Scanning:**

```c
// Thread function to scan a single bus
typedef struct {
    UINT8 BusNumber;
    IIOPCIDevice **ppDevices;
    UINT32 uMaxDevices;
    UINT32 uFoundDevices;
} BusScanContext;

void* ScanBusThread(void *pArg) {
    BusScanContext *pContext = (BusScanContext *)pArg;
    UINT32 uCount = pContext->uMaxDevices;

    PCIScanBus(pContext->BusNumber, pContext->ppDevices, &uCount);
    pContext->uFoundDevices = uCount;

    return NULL;
}

// Scan multiple buses in parallel
void ParallelBusScan(void) {
    BusScanContext Contexts[256];
    pthread_t Threads[256];
    IIOPCIDevice *AllDevices[256][32];
    UINT32 i;

    // Create threads to scan buses 0-255
    for (i = 0; i < 256; i++) {
        Contexts[i].BusNumber = i;
        Contexts[i].ppDevices = AllDevices[i];
        Contexts[i].uMaxDevices = 32;
        Contexts[i].uFoundDevices = 0;

        pthread_create(&Threads[i], NULL, ScanBusThread, &Contexts[i]);
    }

    // Wait for all scans to complete
    for (i = 0; i < 256; i++) {
        pthread_join(Threads[i], NULL);

        if (Contexts[i].uFoundDevices > 0) {
            printf("Bus %u: %u devices\n", i, Contexts[i].uFoundDevices);
        }
    }
}
```

#### Thunderbolt Family

**Fully Thread-Safe Operations:**
- `ThunderboltDetectControllers()` - Safe to call from multiple threads
- `IIOThunderboltController_EnumerateDevices()` - Safe for different controllers
- `IIOThunderboltController_AuthorizeDevice()` - Thread-safe with internal locking
- `IIOThunderboltController_CreateTunnel()` - Thread-safe with internal locking

**Asynchronous Operations:**
- Device authorization callbacks
- Hot-plug event notifications
- Tunnel establishment

**Example - Asynchronous Device Authorization:**

```c
typedef struct {
    IIOThunderboltController *pController;
    IIOThunderboltDevice *pDevice;
} AuthContext;

void* AuthorizeDeviceThread(void *pArg) {
    AuthContext *pContext = (AuthContext *)pArg;
    IO_RETURN Status;

    printf("Authorizing device...\n");

    Status = IIOThunderboltController_AuthorizeDevice(
        pContext->pController,
        pContext->pDevice,
        FALSE  // Non-persistent
    );

    if (Status == IO_SUCCESS) {
        printf("Device authorized successfully\n");
    } else {
        printf("Authorization failed: 0x%08X\n", Status);
    }

    // Clean up references
    IIOThunderboltDevice_Release(pContext->pDevice);
    IIOThunderboltController_Release(pContext->pController);
    free(pContext);

    return NULL;
}

// Authorize multiple devices in parallel
void AuthorizeDevicesAsync(IIOThunderboltController *pController,
                           IIOThunderboltDevice **ppDevices,
                           UINT32 uDeviceCount) {
    pthread_t *pThreads;
    UINT32 i;

    pThreads = malloc(sizeof(pthread_t) * uDeviceCount);

    for (i = 0; i < uDeviceCount; i++) {
        AuthContext *pContext = malloc(sizeof(AuthContext));

        pContext->pController = pController;
        IIOThunderboltController_AddRef(pController);

        pContext->pDevice = ppDevices[i];
        IIOThunderboltDevice_AddRef(ppDevices[i]);

        pthread_create(&pThreads[i], NULL, AuthorizeDeviceThread, pContext);
    }

    // Optionally wait for all authorizations
    for (i = 0; i < uDeviceCount; i++) {
        pthread_join(pThreads[i], NULL);
    }

    free(pThreads);
}
```

### Synchronization Primitives

When implementing multithreaded drivers, use the following patterns:

#### 1. Reference Counting

All COM interfaces use reference counting for thread-safe lifetime management:

```c
// Thread 1
IIOPCIDevice_AddRef(pDevice);  // Increment reference
// Use device...
IIOPCIDevice_Release(pDevice);  // Decrement reference

// Thread 2 (concurrent)
IIOPCIDevice_AddRef(pDevice);  // Safe - atomic increment
// Use device...
IIOPCIDevice_Release(pDevice);  // Safe - atomic decrement
```

#### 2. Work Queues

For complex device operations, use work queues:

```c
typedef struct {
    IIOPCIDevice *pDevice;
    UINT32 uOperation;
} WorkItem;

void ProcessWorkItem(WorkItem *pItem) {
    // Perform device operation
    switch (pItem->uOperation) {
        case OP_INITIALIZE:
            // Initialize device
            break;
        case OP_CONFIGURE:
            // Configure device
            break;
    }

    IIOPCIDevice_Release(pItem->pDevice);
    free(pItem);
}

void QueueDeviceOperation(IIOPCIDevice *pDevice, UINT32 uOperation) {
    WorkItem *pItem = malloc(sizeof(WorkItem));
    pItem->pDevice = pDevice;
    pItem->uOperation = uOperation;

    IIOPCIDevice_AddRef(pDevice);

    // Add to work queue (implementation-specific)
    WorkQueueAdd(ProcessWorkItem, pItem);
}
```

#### 3. Event-Driven Processing

For hot-plug and asynchronous events:

```c
typedef void (*HotplugCallback)(IIOThunderboltDevice *pDevice, void *pContext);

void RegisterHotplugCallback(IIOThunderboltController *pController,
                             HotplugCallback pfnCallback,
                             void *pContext) {
    // Internal implementation would:
    // 1. Create event thread
    // 2. Monitor hot-plug interrupts
    // 3. Call callback when device connected
}

void MyHotplugHandler(IIOThunderboltDevice *pDevice, void *pContext) {
    // Called asynchronously when device is hot-plugged
    printf("Device connected!\n");

    // Get device info
    TB_DEVICE_INFO DeviceInfo;
    IIOThunderboltDevice_GetDeviceInfo(pDevice, &DeviceInfo);

    printf("  Name: %s\n", DeviceInfo.DeviceName);

    // Optionally authorize in background
    // (authorization is thread-safe)
}
```

### Best Practices

1. **Always use AddRef/Release** when passing device references between threads
2. **Avoid holding locks** during long operations (like device initialization)
3. **Use work queues** for complex multistep operations
4. **Implement timeouts** for device operations that may block
5. **Handle hot-unplug** gracefully (devices may disappear during operation)

### Performance Considerations

**Parallel Bus Scanning:**
- Scanning 256 PCI buses sequentially: ~2-5 seconds
- Scanning 256 PCI buses in parallel (256 threads): ~100-200ms
- Recommended: Use thread pool with 4-8 threads for optimal performance

**Device Initialization:**
- BAR mapping: 1-10ms per BAR
- MSI/MSI-X setup: 5-20ms
- Firmware initialization: 100ms-1s

**Thunderbolt:**
- Device enumeration: 50-200ms
- Authorization: 100-500ms (includes challenge-response)
- Tunnel creation: 50-100ms

### Example: Complete Multithreaded Discovery

```c
#include <pthread.h>
#include <iokit/IOKit.h>
#include <iokit/families/pcie.h>
#include <iokit/families/thunderbolt.h>

typedef struct {
    UINT32 uPCIDeviceCount;
    UINT32 uThunderboltControllerCount;
    pthread_mutex_t Lock;
} DiscoveryContext;

void* DiscoverPCIDevices(void *pArg) {
    DiscoveryContext *pContext = (DiscoveryContext *)pArg;
    IIOPCIDevice *pDevices[256];
    UINT32 uCount = 256;

    PCIScanBus(0, pDevices, &uCount);

    pthread_mutex_lock(&pContext->Lock);
    pContext->uPCIDeviceCount = uCount;
    pthread_mutex_unlock(&pContext->Lock);

    // Release devices
    for (UINT32 i = 0; i < uCount; i++) {
        IIOPCIDevice_Release(pDevices[i]);
    }

    return NULL;
}

void* DiscoverThunderbolt(void *pArg) {
    DiscoveryContext *pContext = (DiscoveryContext *)pArg;
    IIOThunderboltController *pControllers[8];
    UINT32 uCount = 8;

    ThunderboltDetectControllers(pControllers, &uCount);

    pthread_mutex_lock(&pContext->Lock);
    pContext->uThunderboltControllerCount = uCount;
    pthread_mutex_unlock(&pContext->Lock);

    // Release controllers
    for (UINT32 i = 0; i < uCount; i++) {
        IIOThunderboltController_Release(pControllers[i]);
    }

    return NULL;
}

void ParallelDeviceDiscovery(void) {
    DiscoveryContext Context = {0};
    pthread_t PCIThread, TBThread;

    pthread_mutex_init(&Context.Lock, NULL);

    // Start parallel discovery
    pthread_create(&PCIThread, NULL, DiscoverPCIDevices, &Context);
    pthread_create(&TBThread, NULL, DiscoverThunderbolt, &Context);

    // Wait for both to complete
    pthread_join(PCIThread, NULL);
    pthread_join(TBThread, NULL);

    printf("Discovery complete:\n");
    printf("  PCI Devices: %u\n", Context.uPCIDeviceCount);
    printf("  Thunderbolt Controllers: %u\n", Context.uThunderboltControllerCount);

    pthread_mutex_destroy(&Context.Lock);
}
```

## See Also

- [IOKit Framework README](../README.md)
- [PCIe Specification](https://pcisig.com/specifications)
- [Thunderbolt Technology](https://www.intel.com/thunderbolt)
