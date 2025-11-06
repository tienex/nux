/*++
    Module Name:

        hypervisor_impl.h

    Abstract:

        Internal implementation structures for the hypervisor framework.

--*/

#pragma once

#include <hv/hypervisor.h>
#include <ananke/ntrtl.h>

/* Forward declarations */
typedef struct HvHypervisor HvHypervisor;
typedef struct HvVirtualMachine HvVirtualMachine;
typedef struct HvVirtualCpu HvVirtualCpu;
typedef struct HvVirtualMemory HvVirtualMemory;

/* ======================================================================= */
/* Memory virtualization structures                                        */
/* ======================================================================= */

/* Shadow page table entry (for software virtualization) */
typedef struct HV_SHADOW_PTE {
    UINT64         GuestPTE;         /* Guest page table entry */
    UINT64         HostPTE;          /* Shadow (host) page table entry */
    UINT64         GuestPhysical;    /* Guest physical address */
    UINT64         HostPhysical;     /* Host physical address */
    UINT32         Flags;
    UINT32         AccessCount;      /* For tracking hot pages */
} HV_SHADOW_PTE;

/* Shadow page table (per-VM) */
typedef struct HV_SHADOW_PT {
    UINT64         BaseAddress;      /* Shadow page table base */
    UINT32         Level;            /* Page table level (0-4) */
    UINT32         EntryCount;
    HV_SHADOW_PTE* Entries;
} HV_SHADOW_PT;

/* Memory region tracking */
typedef struct HV_MEM_REGION {
    struct HV_MEM_REGION* Next;
    UINT64         GuestPhysicalAddress;
    UINT64         HostVirtualAddress;
    UINT64         Size;
    UINT32         Flags;
    UINT32         RefCount;
} HV_MEM_REGION;

/* ======================================================================= */
/* Binary translation cache (VMware/Plex86 style)                         */
/* ======================================================================= */

/* Translated code block */
typedef struct HV_TC_BLOCK {
    struct HV_TC_BLOCK* Next;
    UINT64         GuestAddress;     /* Guest instruction address */
    UINT64         HostAddress;      /* Translated code address */
    UINT32         GuestSize;        /* Original instruction size */
    UINT32         HostSize;         /* Translated code size */
    UINT32         ExecutionCount;   /* Hotness tracking */
    UINT32         Flags;
} HV_TC_BLOCK;

/* Translation cache */
typedef struct HV_TC_CACHE {
    HV_TC_BLOCK**  HashTable;
    UINT32         HashSize;
    UINT32         BlockCount;
    UINT64         CacheSize;
    UINT64         CacheUsed;
    VOID*          CodeMemory;       /* Executable memory region */
    RTL_MEMORY_POOL Pool;
} HV_TC_CACHE;

/* ======================================================================= */
/* CPU virtualization backend                                              */
/* ======================================================================= */

/* Architecture-specific CPU backend operations */
typedef struct HV_CPU_BACKEND_OPS {
    /* Initialize backend */
    HRESULT (*Initialize)(HvVirtualCpu* pCpu);

    /* Shutdown backend */
    HRESULT (*Shutdown)(HvVirtualCpu* pCpu);

    /* Run virtual CPU */
    HRESULT (*Run)(HvVirtualCpu* pCpu, HV_VM_EXIT_INFO* pExitInfo);

    /* Read register */
    HRESULT (*ReadRegister)(HvVirtualCpu* pCpu, UINT32 RegId, HV_REGISTER_VALUE* pValue);

    /* Write register */
    HRESULT (*WriteRegister)(HvVirtualCpu* pCpu, UINT32 RegId, CONST HV_REGISTER_VALUE* pValue);

    /* Handle VM exit */
    HRESULT (*HandleExit)(HvVirtualCpu* pCpu, HV_VM_EXIT_INFO* pExitInfo);

    /* Translate instruction (for binary translation) */
    HRESULT (*TranslateInstruction)(HvVirtualCpu* pCpu, UINT64 GuestAddr, VOID* HostAddr, UINT32* pSize);
} HV_CPU_BACKEND_OPS;

/* CPU backend descriptor */
typedef struct HV_CPU_BACKEND {
    HV_ARCHITECTURE        Architecture;
    HV_VIRT_MODE           Mode;
    HV_CPU_BACKEND_OPS*    Ops;
    VOID*                  PrivateData;
} HV_CPU_BACKEND;

/* ======================================================================= */
/* Virtual CPU implementation                                              */
/* ======================================================================= */

struct HvVirtualCpu {
    IVirtualCpu            Interface;
    UINT32                 RefCount;
    HvVirtualMachine*      VM;

    /* CPU state */
    UINT32                 CpuId;
    HV_VM_STATE            State;
    UINT64                 InstructionPointer;
    HV_REGISTER_VALUE      Registers[128];  /* Architecture-dependent */

    /* Backend */
    HV_CPU_BACKEND*        Backend;

    /* Translation cache (for binary translation mode) */
    HV_TC_CACHE*           TransCache;

    /* Hardware-assisted virtualization context */
    VOID*                  HwContext;  /* VT-x VMCS, AMD-V VMCB, etc. */
};

/* ======================================================================= */
/* Virtual Memory implementation                                           */
/* ======================================================================= */

struct HvVirtualMemory {
    IVirtualMemory         Interface;
    UINT32                 RefCount;
    HvVirtualMachine*      VM;

    /* Memory regions */
    HV_MEM_REGION*         Regions;
    UINT32                 RegionCount;

    /* Shadow page tables (for software virtualization) */
    HV_SHADOW_PT*          ShadowPageTables;
    UINT32                 ShadowPTCount;

    /* Nested page tables (for hardware virtualization) */
    UINT64                 NestedPageTableBase;
    BOOLEAN                NestedPagingEnabled;

    /* Memory allocation pool */
    RTL_MEMORY_POOL        Pool;
};

/* ======================================================================= */
/* Virtual Machine implementation                                          */
/* ======================================================================= */

struct HvVirtualMachine {
    IVirtualMachine        Interface;
    UINT32                 RefCount;
    HvHypervisor*          Hypervisor;

    /* Configuration */
    HV_VM_CONFIG           Config;
    HV_VM_STATE            State;

    /* Virtual CPUs */
    HvVirtualCpu**         VCpus;
    UINT32                 NumCpus;

    /* Virtual memory */
    HvVirtualMemory*       Memory;

    /* Virtual devices */
    struct IVirtualDevice** Devices;
    UINT32                 DeviceCount;
    UINT32                 DeviceCapacity;

    /* Backend */
    HV_CPU_BACKEND*        Backend;

    /* Memory pool */
    RTL_MEMORY_POOL        Pool;
};

/* ======================================================================= */
/* Hypervisor implementation                                               */
/* ======================================================================= */

struct HvHypervisor {
    IHypervisor            Interface;
    UINT32                 RefCount;

    /* State */
    BOOLEAN                Initialized;

    /* Supported architectures and backends */
    HV_CPU_BACKEND*        Backends;
    UINT32                 BackendCount;

    /* Active VMs */
    HvVirtualMachine**     VMs;
    UINT32                 VMCount;

    /* Memory pool */
    RTL_MEMORY_POOL        Pool;
};

/* ======================================================================= */
/* Internal functions                                                      */
/* ======================================================================= */

/* Backend registration */
HRESULT HvRegisterBackend(HvHypervisor* pHv, HV_CPU_BACKEND* pBackend);
HV_CPU_BACKEND* HvFindBackend(HvHypervisor* pHv, HV_ARCHITECTURE Arch, HV_VIRT_MODE Mode);

/* Shadow page table management */
HRESULT HvShadowPT_Initialize(HvVirtualMemory* pMem);
HRESULT HvShadowPT_Shutdown(HvVirtualMemory* pMem);
HRESULT HvShadowPT_Map(HvVirtualMemory* pMem, UINT64 GuestPhys, UINT64 HostVirt, UINT64 Size, UINT32 Flags);
HRESULT HvShadowPT_Unmap(HvVirtualMemory* pMem, UINT64 GuestPhys, UINT64 Size);
HRESULT HvShadowPT_Translate(HvVirtualMemory* pMem, UINT64 GuestVirt, UINT64* pGuestPhys);

/* Translation cache management */
HRESULT HvTC_Initialize(HvVirtualCpu* pCpu);
HRESULT HvTC_Shutdown(HvVirtualCpu* pCpu);
HRESULT HvTC_Lookup(HvVirtualCpu* pCpu, UINT64 GuestAddr, HV_TC_BLOCK** ppBlock);
HRESULT HvTC_Insert(HvVirtualCpu* pCpu, UINT64 GuestAddr, VOID* HostCode, UINT32 GuestSize, UINT32 HostSize);
HRESULT HvTC_Invalidate(HvVirtualCpu* pCpu, UINT64 GuestAddr);
HRESULT HvTC_Flush(HvVirtualCpu* pCpu);

/* Architecture-specific backend functions */
HRESULT HvBackend_X86_Create(HV_VIRT_MODE Mode, HV_CPU_BACKEND** ppBackend);
HRESULT HvBackend_X86_64_Create(HV_VIRT_MODE Mode, HV_CPU_BACKEND** ppBackend);
HRESULT HvBackend_RISCV_Create(HV_VIRT_MODE Mode, HV_ARCHITECTURE Arch, HV_CPU_BACKEND** ppBackend);
HRESULT HvBackend_MIPS_Create(HV_VIRT_MODE Mode, HV_ARCHITECTURE Arch, HV_CPU_BACKEND** ppBackend);
HRESULT HvBackend_SPARC_Create(HV_VIRT_MODE Mode, HV_ARCHITECTURE Arch, HV_CPU_BACKEND** ppBackend);
HRESULT HvBackend_M68K_Create(HV_VIRT_MODE Mode, HV_CPU_BACKEND** ppBackend);
HRESULT HvBackend_VAX_Create(HV_VIRT_MODE Mode, HV_CPU_BACKEND** ppBackend);
HRESULT HvBackend_Alpha_Create(HV_VIRT_MODE Mode, HV_CPU_BACKEND** ppBackend);
HRESULT HvBackend_IA64_Create(HV_VIRT_MODE Mode, HV_CPU_BACKEND** ppBackend);
HRESULT HvBackend_PPC_Create(HV_VIRT_MODE Mode, HV_ARCHITECTURE Arch, HV_CPU_BACKEND** ppBackend);
HRESULT HvBackend_LoongArch_Create(HV_VIRT_MODE Mode, HV_ARCHITECTURE Arch, HV_CPU_BACKEND** ppBackend);
HRESULT HvBackend_DLX_Create(HV_VIRT_MODE Mode, HV_CPU_BACKEND** ppBackend);
HRESULT HvBackend_MMIX_Create(HV_VIRT_MODE Mode, HV_CPU_BACKEND** ppBackend);
