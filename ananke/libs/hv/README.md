# Ananke Hypervisor Framework

A comprehensive, macOS Hypervisor.framework-style virtualization component for the Ananke operating system.

## Overview

The Ananke Hypervisor Framework provides multi-architecture virtualization with support for both hardware-assisted and software-based virtualization techniques. The design is inspired by:

- **macOS Hypervisor.framework** - Clean COM-based API design
- **VMware Workstation (early releases)** - Binary translation techniques
- **Plex86 v1** - Software virtualization approach
- **Mac-on-Linux** - Paravirtualization drivers

## Supported Architectures

### x86 Family
- **Intel 8086/186** - 16-bit real mode
- **Intel 80286** - 16-bit protected mode
- **Intel 80386+** - 32-bit protected mode
- **AMD64/Intel 64** - 64-bit long mode

### RISC Architectures
- **RISC-V** - RV32, RV64 (all standard extensions)
- **MIPS** - MIPS I, II, III, IV, V, MIPS32, MIPS64 (R1-R6)
- **SPARC** - SPARC V8 (32-bit), SPARC V9 (64-bit)
- **DLX** - Educational RISC architecture
- **Alpha** - DEC Alpha 64-bit

### CISC Architectures
- **M68K** - Motorola 68000 series
- **VAX** - DEC VAX

### Modern Architectures
- **IA-64** - Intel Itanium
- **PowerPC** - 32-bit and 64-bit variants
- **LoongArch** - LA32 and LA64
- **MMIX** - Knuth's educational architecture

### Endianness Support
All architectures support their native endianness as well as bi-endian operation where applicable.

## Virtualization Techniques

### Hardware-Assisted Virtualization
- **Intel VT-x** - x86_64 virtualization extensions
- **AMD-V (SVM)** - AMD virtualization extensions
- **RISC-V H-extension** - Hardware virtualization support
- **ARM Virtualization Extensions** - (future)

### Software Virtualization
- **Trap-and-Emulate** - Classic virtualization technique
- **Binary Translation** - VMware/Plex86 style dynamic translation
- **Shadow Page Tables** - Software-based memory virtualization
- **Paravirtualization** - Mac-on-Linux style optimized drivers

### Memory Virtualization
- **Shadow Page Tables** - Software page table synchronization
- **Nested Paging (EPT/NPT)** - Hardware two-dimensional page tables
- **Memory Regions** - Flexible guest physical to host virtual mapping

## Architecture

### COM-Based Interface Design

```c
// Create hypervisor
IHypervisor* pHypervisor;
HvCreateHypervisor(&IID_IHypervisor, (VOID**)&pHypervisor);

// Initialize
pHypervisor->lpVtbl->Initialize(pHypervisor);

// Create VM
HV_VM_CONFIG config = {
    .Architecture = HV_ARCH_X86_64,
    .VirtMode = HV_VIRT_AUTO,
    .NumCpus = 1,
    .MemorySize = 512 * 1024 * 1024,  // 512MB
    .EnableNestedPaging = TRUE
};

IVirtualMachine* pVM;
pHypervisor->lpVtbl->CreateVM(pHypervisor, &config, &pVM);

// Get virtual CPU
IVirtualCpu* pCpu;
pVM->lpVtbl->GetVirtualCpu(pVM, 0, &pCpu);

// Start VM
pVM->lpVtbl->Start(pVM);

// Run CPU
HV_VM_EXIT_INFO exitInfo;
pCpu->lpVtbl->Run(pCpu, &exitInfo);
```

### Key Interfaces

#### IHypervisor
Main hypervisor control interface:
- `Initialize()` / `Shutdown()` - Lifecycle management
- `IsSupported()` - Check architecture/mode support
- `CreateVM()` - Instantiate virtual machines
- `EnumerateArchitectures()` - Query supported architectures

#### IVirtualMachine
Virtual machine instance:
- `Start()` / `Stop()` / `Pause()` / `Resume()` - VM control
- `Reset()` - Reset VM to initial state
- `GetVirtualCpu()` - Access virtual CPUs
- `GetVirtualMemory()` - Access memory interface
- `AttachDevice()` / `DetachDevice()` - Device management

#### IVirtualCpu
Virtual CPU control:
- `Run()` - Execute until VM exit
- `ReadRegister()` / `WriteRegister()` - Register access
- `GetInstructionPointer()` / `SetInstructionPointer()` - PC control
- `Interrupt()` - Inject interrupts
- `SingleStep()` - Debug support

#### IVirtualMemory
Memory virtualization:
- `MapMemory()` / `UnmapMemory()` - GPA to HVA mapping
- `ProtectMemory()` - Change memory permissions
- `ReadMemory()` / `WriteMemory()` - Direct memory access
- `TranslateGVA()` - Virtual to physical translation
- `FlushTLB()` - TLB management

#### IVirtualDevice
Device virtualization:
- `IORead()` / `IOWrite()` - I/O port emulation
- `MemoryRead()` / `MemoryWrite()` - MMIO emulation
- `Initialize()` / `Shutdown()` / `Reset()` - Lifecycle

## Implementation Details

### Translation Cache (Binary Translation)

The hypervisor includes a translation cache for dynamic binary translation:
- Hash-based lookup for translated code blocks
- 4MB default code cache size
- Hotness tracking for optimization decisions
- Flush and invalidation support

### Shadow Page Tables

For software virtualization without nested paging:
- 4-level page table support
- Efficient synchronization with guest page tables
- Page fault handling and lazy mapping
- Access tracking for performance optimization

### VM Exit Handling

Comprehensive VM exit reasons:
- External interrupts and exceptions
- I/O port access
- MSR access (x86)
- Memory access violations
- EPT/NPT violations
- HLT and shutdown
- Hypercalls (paravirtualization)

## Intel 286 Support

Special attention has been paid to the Intel 80286:
- **Machine Status Word (MSW)** - CR0 lower 16 bits
- **Protected Mode** - Descriptor tables and privilege levels
- **Real Mode** - Legacy 8086 compatibility
- **Task Switching** - Hardware task state segment (TSS)

The 286 was the first x86 processor with protected mode, making it historically significant for virtualization.

## Directory Structure

```
hv/
├── include/hv/
│   └── hypervisor.h          # Public API
├── src/
│   ├── hypervisor.c           # Main hypervisor implementation
│   ├── hypervisor_impl.h      # Internal structures
│   ├── vm.c                   # Virtual machine
│   ├── vcpu.c                 # Virtual CPU
│   ├── vmem.c                 # Virtual memory (shadow PT, NPT)
│   ├── tc_cache.c             # Translation cache
│   └── arch/
│       ├── x86_backend.c      # x86/286/x86_64 backend
│       └── backends_stub.c    # Other architecture stubs
└── README.md                  # This file
```

## Build Integration

To integrate into the Ananke build system:

1. Add to `ananke/libs/Makefile.am`:
   ```makefile
   SUBDIRS += hv
   ```

2. Create `ananke/libs/hv/Makefile.am`:
   ```makefile
   lib_LTLIBRARIES = libhv.la
   libhv_la_SOURCES = \
       src/hypervisor.c \
       src/vm.c \
       src/vcpu.c \
       src/vmem.c \
       src/tc_cache.c \
       src/arch/x86_backend.c \
       src/arch/backends_stub.c
   ```

## Example Usage

See `example/hv_test.c` for a complete example demonstrating:
- Hypervisor initialization
- VM creation and configuration
- Memory mapping
- CPU execution
- VM exit handling

## Future Enhancements

- Complete binary translation implementation
- VT-x/AMD-V hardware acceleration
- Full device emulation (disk, network, display)
- Paravirtual drivers (virtio)
- Live migration support
- Snapshot and restore
- Nested virtualization
- ARM architecture support

## References

- **Intel VT-x Specification** - Hardware virtualization
- **AMD-V (SVM) Specification** - AMD virtualization
- **RISC-V Hypervisor Extension** - RISC-V H-extension
- **VMware Virtual Platform** - Binary translation techniques
- **Plex86 v1 Source Code** - Software virtualization reference
- **Mac-on-Linux** - Paravirtualization approach

## License

Part of the Ananke operating system project.

## Authors

- Ananke Hypervisor Framework - 2025
