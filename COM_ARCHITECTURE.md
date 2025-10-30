# NUX COM Architecture Diagram

## Interface Hierarchy

This document provides visual representations of the NUX COM interface architecture.

## Complete Interface Map

```
┌─────────────────────────────────────────────────────────────────────┐
│                           IUnknown                                  │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │  QueryInterface(riid, ppvObject)                             │  │
│  │  AddRef()                                                    │  │
│  │  Release()                                                   │  │
│  └──────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                 ┌──────────────────┼──────────────────┐
                 │                  │                  │
       ┌─────────▼─────────┐  ┌────▼────┐  ┌─────────▼─────────┐
       │   HAL Layer (7)   │  │PLT (5)  │  │   NUX Layer (10)  │
       └───────────────────┘  └─────────┘  └───────────────────┘
```

## Hardware Abstraction Layer (HAL) - 7 Interfaces

```
IHal (Main HAL Interface)
├── GetCpuInterface() → IHalCpu
├── GetPhysMemInterface() → IHalPhysMem
├── GetVirtMemInterface() → IHalVirtMem
├── GetMapInterface() → IHalMap
├── GetPcpuInterface() → IHalPcpu
└── GetFrameInterface() → IHalFrame

IHalCpu (CPU Operations)
├── IoIn() / IoOut()
├── MsrRead() / MsrWrite()
├── Cpuid()
├── PutChar()
└── Relax()

IHalPhysMem (Physical Memory)
├── GetMaxPfn()
├── PhysToVirt()
└── VirtToPhys()

IHalVirtMem (Virtual Memory)
├── GetMaxVaddr()
├── GetDirectMapBase()
├── GetDirectMapEnd()
└── GetPageSize()

IHalMap (Page Mapping)
├── GetPte()
├── SetPte()
├── FlushTlbPage()
├── FlushTlbAll()
└── FlushTlbGlobal()

IHalPcpu (Per-CPU Data)
├── GetPtr()
└── SetPtr()

IHalFrame (CPU Frame)
├── GetIp() / SetIp()
├── GetSp() / SetSp()
├── GetGp() / SetGp()
└── Print()
```

## Platform Layer (PLT) - 5 Interfaces

```
IPlt (Main Platform Interface)
├── Init()
├── GetHardwareInterface() → IPltHardware
├── GetIrqInterface() → IPltIrq
├── GetPcpuInterface() → IPltPcpu
└── GetTimerInterface() → IPltTimer

IPltHardware (Hardware)
└── PutChar()

IPltIrq (IRQ Management)
├── GetType()
├── Enable() / Disable()
├── GetMaxIrq()
├── IsLevel()
└── EndOfInterrupt()

IPltPcpu (Physical CPU)
├── Iterate()
├── Enter()
├── SendNmi() / BroadcastNmi()
├── SendIpi() / BroadcastIpi()
├── GetId()
└── Start()

IPltTimer (Timer)
├── GetCounter() / SetCounter()
├── GetPeriod()
├── SetAlarm() / ClearAlarm()
└── EndOfInterrupt()
```

## NUX Kernel API - 10 Interfaces

```
INux (Main Aggregator)
├── GetMemoryInterface() → INuxMemory
├── GetKvaInterface() → INuxKva
├── GetKmapInterface() → INuxKmap
├── GetKmemInterface() → INuxKmem
├── GetCpuInterface() → INuxCpu
├── GetTimerInterface() → INuxTimer
├── GetUmapInterface() → INuxUmap
├── GetUaddrInterface() → INuxUaddr
└── GetUctxtInterface() → INuxUctxt

INuxMemory (Physical Memory)
├── PfnGet() / PfnPut()
├── PfnAllocate() / PfnFree()
├── PfnAvailable()
└── SetAllocator()

INuxKva (Kernel Virtual Address)
├── Allocate() / Free()
├── Map()
├── PhysMap()
└── Unmap()

INuxKmap (Kernel Mapping)
├── GetPfn()
├── Map() / MapNoAlloc() / Unmap()
├── IsMapped() / IsMappedRange()
├── Ensure() / EnsureRange()
├── GetTlbGen() / GetTlbGenGlobal()
└── Commit()

INuxKmem (Kernel Memory)
├── Brk() / Sbrk()
├── BrkGrow() / BrkShrink()
├── Allocate() / Free()
├── SetTrimMode()
└── TrimOne()

INuxCpu (CPU Management)
├── StartAll()
├── GetId() / GetNum() / GetActiveMask()
├── SetData() / GetData()
├── Idle()
├── SendNmi() / SendNmiMask() / BroadcastNmi()
├── SendIpi() / SendIpiMask() / BroadcastIpi()
├── FlushTlb() / FlushTlbMask() / BroadcastFlushTlb()
├── UpdateKernelTlb() / ReachKernelTlb()
├── Stop() / StopMask() / BroadcastStop()
├── UserAccessCopyFrom() / UserAccessCopyTo()
├── UserAccessMemset()
└── GetCurrentUmap() / EnterUmap() / ExitUmap()

INuxTimer (Timer)
├── SetAlarm() / ClearAlarm()
└── GetTime()

INuxUmap (User Mapping)
├── Bootstrap() / Init() / Free()
├── Map() / Unmap()
├── ChangeFlags()
└── Commit()

INuxUaddr (User Address)
├── Valid() / ValidRange()
├── CopyFrom() / CopyTo()
└── Memset()

INuxUctxt (User Context)
├── Bootstrap() / Init()
├── SetIp() / GetIp()
├── SetSp() / GetSp()
├── SetGp() / GetGp()
├── SetRet()
├── SetA0() / SetA1() / SetA2()
├── SetTls()
└── Print()
```

## Interface Relationship Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                     Global Interface Pointers                   │
├─────────────────────────────────────────────────────────────────┤
│  extern IHal *gpHal;     // Hardware Abstraction Layer          │
│  extern IPlt *gpPlt;     // Platform Layer                      │
│  extern INux *gpNux;     // NUX Kernel API                      │
└─────────────────────────────────────────────────────────────────┘
                                    │
                 ┌──────────────────┼──────────────────┐
                 │                  │                  │
                 ▼                  ▼                  ▼
        ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
        │   Hardware  │    │  Platform   │    │   Kernel    │
        │   Access    │    │   Services  │    │   Services  │
        └─────────────┘    └─────────────┘    └─────────────┘
                │                  │                  │
        ┌───────┴───────┐  ┌───────┴───────┐  ┌───────┴───────┐
        │               │  │               │  │               │
        ▼               ▼  ▼               ▼  ▼               ▼
    CPU/Memory    Mapping  IRQ          Timer Memory       Context
    Operations           Management           Management
```

## Layered Architecture

```
┌────────────────────────────────────────────────────────────┐
│                      Application Layer                     │
│                    (User-mode programs)                    │
└────────────────────────────────────────────────────────────┘
                            │
                            │ System Calls
                            ▼
┌────────────────────────────────────────────────────────────┐
│                      NUX Kernel API                        │
│  ┌──────────┬──────────┬──────────┬──────────┬─────────┐  │
│  │ INuxCpu  │INuxMemory│ INuxKmap │ INuxUmap │INuxUctxt│  │
│  └──────────┴──────────┴──────────┴──────────┴─────────┘  │
└────────────────────────────────────────────────────────────┘
                            │
                            │ Abstract Interface
                            ▼
┌────────────────────────────────────────────────────────────┐
│                    Platform Layer (PLT)                    │
│  ┌──────────┬──────────┬──────────┬──────────┐            │
│  │  IPltIrq │IPltPcpu  │IPltTimer │IPltHW    │            │
│  └──────────┴──────────┴──────────┴──────────┘            │
└────────────────────────────────────────────────────────────┘
                            │
                            │ Platform Abstraction
                            ▼
┌────────────────────────────────────────────────────────────┐
│           Hardware Abstraction Layer (HAL)                 │
│  ┌──────────┬──────────┬──────────┬──────────┐            │
│  │ IHalCpu  │IHalPhysMem│IHalMap  │IHalFrame │            │
│  └──────────┴──────────┴──────────┴──────────┘            │
└────────────────────────────────────────────────────────────┘
                            │
                            │ Hardware Access
                            ▼
┌────────────────────────────────────────────────────────────┐
│                     Physical Hardware                      │
│         (x86_64, i386, RISC-V processors)                 │
└────────────────────────────────────────────────────────────┘
```

## Interface Usage Example

```c
// Access global interfaces
extern IHal *gpHal;
extern IPlt *gpPlt;
extern INux *gpNux;

// Get sub-interfaces
IHalCpu *pHalCpu = NULL;
gpHal->lpVtbl->GetCpuInterface(gpHal, &pHalCpu);

INuxMemory *pMemory = NULL;
gpNux->lpVtbl->GetMemoryInterface(gpNux, &pMemory);

// Use interface methods
pHalCpu->lpVtbl->PutChar(pHalCpu, 'H');
PFN pfn = pMemory->lpVtbl->PfnAllocate(pMemory, 0);

// Or use legacy wrappers (backward compatible)
hal_putchar('H');
pfn_t pfn = pfn_alloc(0);
```

## Data Flow Example: Memory Allocation

```
Application
    │
    │ 1. Allocate memory request
    ▼
INuxKmem->Allocate()
    │
    │ 2. Request KVA
    ▼
INuxKva->Allocate()
    │
    │ 3. Allocate physical pages
    ▼
INuxMemory->PfnAllocate()
    │
    │ 4. Map pages
    ▼
INuxKmap->Map()
    │
    │ 5. Low-level mapping
    ▼
IHalMap->SetPte()
    │
    │ 6. Hardware operation
    ▼
Physical Page Tables
```

## Interface Benefits

### 1. Separation of Concerns
- **HAL**: Hardware-specific operations (CPU, memory, I/O)
- **PLT**: Platform-specific services (IRQ, timers, ACPI/DTB)
- **NUX**: Kernel-level abstractions (memory management, process)

### 2. Extensibility
- New interfaces can be added without breaking existing code
- QueryInterface enables runtime capability discovery
- Versioning support through interface IDs

### 3. Testability
- Interfaces can be mocked for unit testing
- Clear boundaries between layers
- Dependency injection friendly

### 4. Maintainability
- Clear ownership of functionality
- Self-documenting through interface design
- Easy to understand dependencies

## Interface Design Principles

### Single Responsibility
Each interface has one clear purpose:
- IHalCpu: CPU operations only
- INuxMemory: Physical memory only
- IPltTimer: Timer operations only

### Interface Segregation
Clients depend only on interfaces they use:
- Don't need full HAL to access CPU
- Don't need full PLT to use timers
- Get exactly what you need

### Dependency Inversion
High-level code depends on abstractions:
- NUX depends on HAL interfaces, not implementations
- Applications depend on NUX interfaces
- Implementations can be swapped

### Open/Closed Principle
- Interfaces are stable (closed for modification)
- New functionality via new interfaces (open for extension)
- Backward compatibility maintained

---

**Document Version**: 1.0
**Last Updated**: October 24, 2025
**Related Documents**: TRANSFORMATION_GUIDE.md, TRANSFORMATION_SUMMARY.md
