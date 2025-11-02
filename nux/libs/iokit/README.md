# IOKit Driver Framework for NUX

## Overview

The IOKit Driver Framework is a COM-based driver development framework inspired by Apple's IOKit. It provides a unified architecture for implementing both kernel-space and user-space drivers in the NUX microkernel.

## Key Features

- **COM-based Architecture**: All interfaces use COM/IUnknown for binary compatibility
- **Dual-Space Support**: Drivers can run in kernel-space or user-space
- **Device Tree Registry**: Hierarchical device registry (IORegistry) for device discovery
- **Property-Based Matching**: Flexible device-to-driver matching system
- **User-Kernel Communication**: IOUserClient interface for user-space drivers
- **Memory Management**: IOMemoryDescriptor for DMA and I/O memory operations
- **Event Processing**: IOWorkLoop for serialized driver event handling
- **NT-Style Naming**: PascalCase functions, Hungarian notation parameters

## Architecture

### Core Components

1. **IIOService** - Base interface for all drivers and devices
   - Lifecycle management (Probe, Start, Stop, Terminate)
   - Property-based configuration
   - Parent-child relationships

2. **IIORegistry** - Device tree management
   - Hierarchical service registry
   - Service discovery and enumeration
   - Path-based service lookup

3. **IIOUserClient** - User-space communication
   - Method invocation from user-space
   - Shared memory regions
   - Asynchronous notifications

4. **IIOMemoryDescriptor** - I/O memory management
   - Physical/virtual memory abstraction
   - DMA scatter-gather support
   - Memory mapping and protection

5. **IIOWorkLoop** - Event processing
   - Single-threaded event serialization
   - Interrupt, timer, and gate support
   - Thread-safe driver operations

### Driver Types

#### Kernel-Space Drivers

Kernel-space drivers run in privileged mode and have direct hardware access:

- Direct memory-mapped I/O
- Interrupt handling
- DMA operations
- Maximum performance
- Higher risk (kernel crashes)

#### User-Space Drivers

User-space drivers run in unprivileged mode with kernel mediation:

- Protected execution
- Fault isolation
- Easier development and debugging
- Slightly higher overhead
- Communication via IOUserClient

## Quick Start

### Creating a Kernel-Space Driver

```c
#include <iokit/IOKit.h>

// 1. Create your driver service
IIOService *pMyDriver;
IOServiceCreate("MyDriver", &pMyDriver);

// 2. Set device matching properties
UINT32 uDeviceClass = 0x12345678;
IIOService_SetProperty(pMyDriver, "device-class",
                       &uDeviceClass, sizeof(uDeviceClass),
                       IO_PROPERTY_TYPE_NUMBER);

// 3. Register with the framework
IOKitRegisterService(pMyDriver, NULL);

// 4. Implement lifecycle methods (Probe, Start, Stop, Terminate)
//    by creating a custom vtable or using inheritance
```

### Creating a User-Space Driver

```c
#include <iokit/IOKit.h>

// 1. Find the kernel service
IIORegistry *pRegistry;
IOKitGetRegistry(&pRegistry);

IIOService *pServices[16];
UINT32 uCount = 16;
IIORegistry_FindServicesByName(pRegistry, "MyDevice", NULL, 0,
                               pServices, &uCount);

// 2. Create user client (if kernel service supports it)
IIOUserClient *pUserClient;
// ... obtain user client from service ...

// 3. Open connection
IIOUserClient_ClientOpen(pUserClient, IO_OPTION_SHARED);

// 4. Call methods
UINT64 InputArgs[2] = { 0x1000, 0x42 };
UINT64 OutputArgs[1];
UINT32 uOutputCount = 1;
IIOUserClient_CallScalarMethod(pUserClient, 0x100,
                                InputArgs, 2,
                                OutputArgs, &uOutputCount);

// 5. Close when done
IIOUserClient_ClientClose(pUserClient, 0);
```

## Driver Development Guide

### Driver Lifecycle

1. **Probe** - Determine if driver supports device
   ```c
   IO_RETURN MyDriver_Probe(IIOService *pThis, IIOService *pProvider,
                            UINT32 *puProbeScore) {
       // Check provider properties
       // Return IO_SUCCESS and probe score if supported
       *puProbeScore = 1000;
       return IO_SUCCESS;
   }
   ```

2. **Start** - Initialize driver and hardware
   ```c
   IO_RETURN MyDriver_Start(IIOService *pThis, IIOService *pProvider) {
       // Allocate resources
       // Configure hardware
       // Register sub-services
       return IO_SUCCESS;
   }
   ```

3. **Stop** - Quiesce driver operations
   ```c
   IO_RETURN MyDriver_Stop(IIOService *pThis, IIOService *pProvider) {
       // Stop I/O operations
       // Prepare for termination
       return IO_SUCCESS;
   }
   ```

4. **Terminate** - Cleanup and deallocate
   ```c
   IO_RETURN MyDriver_Terminate(IIOService *pThis, UINT32 uOptions) {
       // Release all resources
       // Free memory
       return IO_SUCCESS;
   }
   ```

### Property-Based Matching

Drivers and devices communicate capabilities through properties:

```c
// Device publishes properties
IIOService_SetProperty(pDevice, "vendor-id", &uVendorID,
                       sizeof(uVendorID), IO_PROPERTY_TYPE_NUMBER);
IIOService_SetProperty(pDevice, "device-id", &uDeviceID,
                       sizeof(uDeviceID), IO_PROPERTY_TYPE_NUMBER);

// Driver checks properties in Probe
UINT32 uVendorID;
UINTN cbSize = sizeof(uVendorID);
if (IIOService_GetProperty(pProvider, "vendor-id", &uVendorID,
                           &cbSize, NULL) == IO_SUCCESS) {
    if (uVendorID == MY_VENDOR_ID) {
        // Driver supports this device
        return IO_SUCCESS;
    }
}
```

### Registry Operations

```c
// Register a new service
IIORegistry *pRegistry;
IOKitGetRegistry(&pRegistry);
IIORegistry_RegisterService(pRegistry, pMyService, pParentService);

// Find services by name
IIOService *pServices[16];
UINT32 uCount = 16;
IIORegistry_FindServicesByName(pRegistry, "SerialPort", NULL, 0,
                               pServices, &uCount);

// Find services by property
IIORegistry_FindServicesByProperty(pRegistry, "device-class",
                                   &uClass, sizeof(uClass), NULL,
                                   pServices, &uCount);

// Iterate through registry
VOID *pIterator;
IIORegistry_CreateIterator(pRegistry, NULL, NULL, 0, &pIterator);
while (IIORegistry_IteratorNext(pRegistry, pIterator, &pService) == IO_SUCCESS) {
    // Process service
    IIOService_Release(pService);
}
IIORegistry_DestroyIterator(pRegistry, pIterator);
```

## Examples

### Example 1: Simple Kernel Driver

See `examples/example_driver.c` for a complete kernel-space driver implementation.

### Example 2: User-Space Driver

See `examples/example_userclient.c` for a user-space driver using IOUserClient.

## API Reference

### Return Codes

- `IO_SUCCESS` (0x00000000) - Operation succeeded
- `IO_ERROR` (0xE0000001) - General error
- `IO_NO_MEMORY` (0xE0000002) - Insufficient memory
- `IO_NO_DEVICE` (0xE0000005) - Device not found
- `IO_BAD_ARGUMENT` (0xE0000007) - Invalid argument
- `IO_UNSUPPORTED` (0xE000000C) - Operation not supported
- `IO_BUSY` (0xE0000019) - Device busy
- `IO_TIMEOUT` (0xE000001A) - Operation timed out
- `IO_NOT_READY` (0xE000001C) - Device not ready

### Service States

- `IO_SERVICE_INACTIVE` - Service created but not registered
- `IO_SERVICE_REGISTERED` - Service registered in registry
- `IO_SERVICE_MATCHED` - Service matched with driver
- `IO_SERVICE_STARTED` - Service started and active
- `IO_SERVICE_BUSY` - Service processing requests
- `IO_SERVICE_TERMINATED` - Service terminated

## Integration with NUX Kernel

The IOKit framework integrates with NUX kernel components:

### Initialization

Call from kernel main:
```c
#include <iokit/IOKit.h>

void kernel_main(void) {
    // ... other initialization ...

    IOKitInitialize();

    // Register platform drivers
    // ...
}
```

### Shutdown

Call from kernel shutdown:
```c
void kernel_shutdown(void) {
    // Shutdown drivers
    IOKitShutdown();

    // ... other cleanup ...
}
```

## Building

The IOKit framework is built as part of the NUX kernel:

```bash
# Configure for your architecture
./configure ARCH=amd64

# Build
make

# The library will be built as libnux_iokit.a
```

To include IOKit in your kernel module:

```makefile
LIBS += -lnux_iokit
CFLAGS += -I$(top_srcdir)/nux/libs/iokit/include/public
```

## Debugging

### Registry Dump

To see all registered services:

```c
IOKitDumpRegistry();
```

This prints the entire device tree to the console.

### Service Properties

To inspect service properties:

```c
CHAR8 szName[64];
IIOService_GetServiceName(pService, szName, sizeof(szName));
printf("Service: %s\n", szName);

UINT32 uState;
IIOService_GetServiceState(pService, &uState);
printf("State: 0x%08X\n", uState);
```

## Comparison with Apple's IOKit

| Feature | Apple IOKit | NUX IOKit |
|---------|-------------|-----------|
| Language | C++ | C (COM-based) |
| Base Class | IOService | IIOService (interface) |
| Memory Management | Reference counting | COM AddRef/Release |
| User-Kernel IPC | IOUserClient | IIOUserClient |
| Device Matching | OSDictionary | Property table |
| Event Handling | IOWorkLoop | IIOWorkLoop |
| Memory Descriptors | IOMemoryDescriptor | IIOMemoryDescriptor |

## Best Practices

1. **Always check return codes** - All IOKit functions return IO_RETURN status
2. **Manage reference counts** - Call AddRef/Release properly to avoid leaks
3. **Use properties for matching** - Publish device capabilities as properties
4. **Implement all lifecycle methods** - Probe, Start, Stop, Terminate
5. **Handle errors gracefully** - Return appropriate error codes
6. **Minimize probe score** - Only claim devices you actually support
7. **Release resources in Terminate** - Clean up all allocations
8. **Use IOWorkLoop for synchronization** - Serialize driver operations
9. **Validate user input** - Especially in IOUserClient methods
10. **Log operations** - Use printf for debugging (remove in production)

## Future Enhancements

Planned features for future releases:

- [ ] Power management interfaces (IIOPowerManagement)
- [ ] Interrupt event sources (IIOInterruptEventSource)
- [ ] Timer event sources (IIOTimerEventSource)
- [ ] DMA controller abstraction
- [ ] USB family interfaces
- [ ] PCI family interfaces
- [ ] Storage family interfaces
- [ ] Network family interfaces
- [ ] Hot-plug support
- [ ] Driver matching optimization
- [ ] Lazy driver loading

## License

Copyright (c) 2025 NUX Project

## Support

For bug reports and feature requests, please file an issue on the NUX GitHub repository.

## Authors

- NUX Development Team

---

**Happy Driver Development!**
