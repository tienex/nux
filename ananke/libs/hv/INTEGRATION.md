# Ananke Hypervisor Framework - Integration Guide

This guide explains how to integrate the Ananke Hypervisor Framework into your build system and applications.

## Build System Integration

### Step 1: Add to Autoconf/Automake Build

Add the hypervisor library to your `configure.ac`:

```m4
# Hypervisor Framework
AC_CONFIG_FILES([
    ananke/libs/hv/Makefile
])
```

Add to `ananke/libs/Makefile.am`:

```makefile
SUBDIRS += hv
```

### Step 2: Rebuild Configuration

```bash
autoreconf -i
./configure
make
make install
```

This will build `libhv.la` and install it to your library directory.

## Using the Hypervisor Framework

### Basic Usage Example

```c
#include <hv/hypervisor.h>
#include <stdio.h>

int main(void) {
    IHypervisor* pHypervisor = NULL;
    IVirtualMachine* pVM = NULL;
    HRESULT hr;

    // Create and initialize hypervisor
    hr = HvCreateHypervisor(&IID_IHypervisor, (VOID**)&pHypervisor);
    if (SUCCEEDED(hr)) {
        pHypervisor->lpVtbl->Initialize(pHypervisor);

        // Configure VM
        HV_VM_CONFIG config = {
            .Architecture = HV_ARCH_X86_64,
            .VirtMode = HV_VIRT_AUTO,
            .NumCpus = 1,
            .MemorySize = 512 * 1024 * 1024,
            .EnableNestedPaging = TRUE
        };

        // Create VM
        hr = pHypervisor->lpVtbl->CreateVM(pHypervisor, &config, &pVM);
        if (SUCCEEDED(hr)) {
            // Use VM...
            IVirtualMachine_Release(pVM);
        }

        pHypervisor->lpVtbl->Shutdown(pHypervisor);
        IHypervisor_Release(pHypervisor);
    }

    return 0;
}
```

### Compilation

```bash
gcc -o myapp myapp.c -I/usr/local/include -L/usr/local/lib -lhv -lntrtl
```

## Advanced Integration

### 1. Virtual Devices

#### Creating a Virtual Disk

```c
IVirtualDevice* pDisk;
HRESULT hr = HvCreateVirtualDisk(
    10 * 1024 * 1024,  // 10MB
    FALSE,              // Read-write
    &pDisk
);

if (SUCCEEDED(hr)) {
    IVirtualMachine_AttachDevice(pVM, pDisk);
}
```

#### Creating a Virtual Network

```c
UINT8 macAddr[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
IVirtualDevice* pNetwork;

HRESULT hr = HvCreateVirtualNetwork(macAddr, &pNetwork);
if (SUCCEEDED(hr)) {
    IVirtualMachine_AttachDevice(pVM, pNetwork);
}
```

#### Creating a Virtual Serial Port

```c
void SerialOutput(VOID* ctx, UINT8 data) {
    printf("%c", data);
}

IVirtualDevice* pSerial;
HRESULT hr = HvCreateVirtualSerial(SerialOutput, NULL, &pSerial);
if (SUCCEEDED(hr)) {
    IVirtualMachine_AttachDevice(pVM, pSerial);
}
```

### 2. Memory Management

#### Mapping Guest Memory

```c
IVirtualMemory* pMem;
IVirtualMachine_GetVirtualMemory(pVM, &pMem);

// Allocate host memory
VOID* hostMem = malloc(4096);

// Map to guest physical address
IVirtualMemory_MapMemory(
    pMem,
    0x0000,              // Guest physical address
    (UINT64)hostMem,     // Host virtual address
    4096,                // Size
    HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC
);

IVirtualMemory_Release(pMem);
```

#### Reading/Writing Guest Memory

```c
IVirtualMemory* pMem;
IVirtualMachine_GetVirtualMemory(pVM, &pMem);

// Write to guest memory
UINT8 data[] = {0x90, 0x90, 0x90};  // NOP instructions
IVirtualMemory_WriteMemory(pMem, 0x1000, data, sizeof(data));

// Read from guest memory
UINT8 buffer[16];
IVirtualMemory_ReadMemory(pMem, 0x1000, buffer, sizeof(buffer));

IVirtualMemory_Release(pMem);
```

### 3. CPU Execution

#### Running a Virtual CPU

```c
IVirtualCpu* pCpu;
IVirtualMachine_GetVirtualCpu(pVM, 0, &pCpu);

// Set initial state
IVirtualCpu_SetInstructionPointer(pCpu, 0x1000);

HV_REGISTER_VALUE regValue;
regValue.u64 = 0x7000;
IVirtualCpu_WriteRegister(pCpu, HV_X86_RSP, &regValue);

// Start VM
IVirtualMachine_Start(pVM);

// Run CPU
HV_VM_EXIT_INFO exitInfo;
HRESULT hr = IVirtualCpu_Run(pCpu, &exitInfo);

// Handle exit
switch (exitInfo.Reason) {
    case HV_EXIT_HLT:
        printf("CPU halted\n");
        break;
    case HV_EXIT_IO_INSTRUCTION:
        printf("I/O instruction at 0x%llx\n", exitInfo.GuestLinearAddress);
        break;
    // ... handle other exits
}

IVirtualCpu_Release(pCpu);
```

### 4. Paravirtualization

#### Using Hypercalls

Guest code can use hypercalls for optimized operations:

```c
// Guest code (x86_64)
static inline uint64_t hypercall(
    uint32_t call_num,
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2
) {
    uint64_t result;
    asm volatile(
        "vmcall"  // or "vmmcall" for AMD
        : "=a" (result)
        : "a" (call_num), "b" (arg0), "c" (arg1), "d" (arg2)
        : "memory"
    );
    return result;
}

// Console write
hypercall(HV_HC_CONSOLE_WRITE, (uint64_t)buffer, length, 0);

// Yield CPU
hypercall(HV_HC_YIELD, 0, 0, 0);
```

#### Mac-on-Linux Style Shared Memory

```c
// Host: Create paravirtual console
HV_PARAVIRT_CONSOLE* pConsole;
HvParavirtConsole_Create(4096, &pConsole);

// Write to console
HvParavirtConsole_Write(pConsole, "Hello from host\n", 16);

// Read from console (guest can write to shared buffer)
CHAR8 buffer[256];
UINT32 actual;
HvParavirtConsole_Read(pConsole, buffer, sizeof(buffer), &actual);

// Cleanup
HvParavirtConsole_Destroy(pConsole);
```

## Architecture-Specific Notes

### Intel x86/x86_64

- **Hardware Acceleration**: VT-x support is automatically detected
- **286 Support**: Full Machine Status Word (MSW) and protected mode support
- **Registers**: Complete x86_64 register set including:
  - General purpose: RAX-R15
  - Control: CR0, CR2, CR3, CR4, CR8
  - Segment: CS, DS, ES, FS, GS, SS
  - Debug: DR0-DR7
  - MSRs: EFER, KERNEL_GS_BASE, APIC_BASE

### RISC-V

- **RV32/RV64**: Both 32-bit and 64-bit variants supported
- **H-Extension**: Hardware virtualization extension framework ready
- **CSRs**: Full CSR access via register interface

### MIPS

- Supports all MIPS generations (I through R6)
- Both 32-bit and 64-bit variants
- All endianness modes (big, little, bi-endian)

### Other Architectures

All supported architectures provide:
- Generic backend with trap-and-emulate
- Binary translation framework
- Full register access via COM interfaces

## Performance Considerations

### Hardware vs Software Virtualization

```c
// Check what's supported
BOOLEAN hwSupported, swSupported;
pHypervisor->lpVtbl->IsSupported(
    pHypervisor,
    HV_ARCH_X86_64,
    HV_VIRT_HARDWARE,
    &hwSupported
);

pHypervisor->lpVtbl->IsSupported(
    pHypervisor,
    HV_ARCH_X86_64,
    HV_VIRT_SOFTWARE,
    &swSupported
);

// Prefer hardware when available
HV_VM_CONFIG config = {
    .VirtMode = hwSupported ? HV_VIRT_HARDWARE : HV_VIRT_SOFTWARE,
    // ...
};
```

### Memory Optimization

```c
HV_VM_CONFIG config = {
    // Enable nested paging for better performance
    .EnableNestedPaging = TRUE,

    // Disable shadow page tables when EPT/NPT is available
    .EnableShadowPageTables = FALSE,

    // Enable binary translation for software virtualization
    .EnableBinaryTranslation = TRUE,

    // ...
};
```

### Device Performance

For best performance:
1. Use paravirtual devices (virtio-style) instead of fully emulated
2. Use shared memory for bulk data transfer
3. Enable hypercalls for frequent operations

## Error Handling

Always check HRESULT return codes:

```c
HRESULT hr = IHypervisor_Initialize(pHypervisor);
if (FAILED(hr)) {
    switch (hr) {
        case HV_UNSUPPORTED:
            printf("Hypervisor not supported on this platform\n");
            break;
        case HV_NO_RESOURCES:
            printf("Insufficient resources\n");
            break;
        case HV_DENIED:
            printf("Access denied\n");
            break;
        default:
            printf("Error: 0x%08X\n", hr);
            break;
    }
}
```

## Threading and Synchronization

- Each virtual CPU should run in its own thread
- Use locks when accessing shared VM resources
- VM state changes (start/stop) are thread-safe

Example multi-threaded setup:

```c
void* vcpu_thread(void* arg) {
    IVirtualCpu* pCpu = (IVirtualCpu*)arg;
    HV_VM_EXIT_INFO exitInfo;

    while (running) {
        HRESULT hr = IVirtualCpu_Run(pCpu, &exitInfo);
        if (FAILED(hr)) break;

        // Handle exit
        handle_vm_exit(&exitInfo);
    }

    return NULL;
}

// Create threads for each vCPU
for (int i = 0; i < num_cpus; i++) {
    IVirtualCpu* pCpu;
    IVirtualMachine_GetVirtualCpu(pVM, i, &pCpu);
    pthread_create(&threads[i], NULL, vcpu_thread, pCpu);
}
```

## Debugging

### Enable Debug Output

The hypervisor uses NTRTL for internal operations. Enable verbose logging:

```c
// Before initialization
extern void RtlSetDebugLevel(int level);
RtlSetDebugLevel(3);  // Verbose

IHypervisor_Initialize(pHypervisor);
```

### Inspecting CPU State

```c
IVirtualCpu* pCpu;
IVirtualMachine_GetVirtualCpu(pVM, 0, &pCpu);

// Read all GPRs
HV_REGISTER regs[16] = {
    {HV_X86_RAX}, {HV_X86_RCX}, {HV_X86_RDX}, {HV_X86_RBX},
    {HV_X86_RSP}, {HV_X86_RBP}, {HV_X86_RSI}, {HV_X86_RDI},
    {HV_X86_R8},  {HV_X86_R9},  {HV_X86_R10}, {HV_X86_R11},
    {HV_X86_R12}, {HV_X86_R13}, {HV_X86_R14}, {HV_X86_R15}
};

IVirtualCpu_ReadRegisters(pCpu, 16, regs);

for (int i = 0; i < 16; i++) {
    printf("R%d: 0x%llx\n", i, regs[i].Value.u64);
}

IVirtualCpu_Release(pCpu);
```

## References

- [README.md](README.md) - Overview and architecture details
- [example/hv_test.c](example/hv_test.c) - Complete working example
- [include/hv/hypervisor.h](include/hv/hypervisor.h) - API reference

## Support

For issues and questions:
- Check the example code in `example/hv_test.c`
- Review API documentation in header files
- File issues in the project tracker
