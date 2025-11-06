/*++
    Module Name:

        hypervisor.h

    Abstract:

        Ananke Hypervisor Framework - macOS Hypervisor.framework style API
        for multi-architecture virtualization with hardware and software
        assistance.

        Supports: x86 (286/386+), x86_64, RISC-V (32/64), MIPS (I-R6, 32/64),
                  SPARC (32/64), M68K, VAX, Alpha, IA-64, PowerPC (32/64),
                  LoongArch (32/64), DLX, MMIX (all endianness variants)

        Virtualization techniques:
        - Hardware-assisted (VT-x, AMD-V, RISC-V H-extension, etc.)
        - Software-assisted (trap-and-emulate, binary translation)
        - Paravirtualization (Mac-on-Linux style drivers)

    Environment:

        C and C++ compatible. COM-based interfaces.

--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>

/* Hypervisor error codes */
#define HV_SUCCESS              S_OK
#define HV_ERROR                E_FAIL
#define HV_BUSY                 HRESULT_FROM_WIN32(ERROR_BUSY)
#define HV_BAD_ARGUMENT         E_INVALIDARG
#define HV_NO_RESOURCES         E_OUTOFMEMORY
#define HV_NO_DEVICE            HRESULT_FROM_WIN32(ERROR_NOT_FOUND)
#define HV_UNSUPPORTED          E_NOTIMPL
#define HV_DENIED               E_ACCESSDENIED
#define HV_OPERATION_FAILED     HRESULT_FROM_WIN32(ERROR_OPERATION_FAILED)

/* ======================================================================= */
/* Architecture enumeration                                                */
/* ======================================================================= */
typedef enum HV_ARCHITECTURE {
    HV_ARCH_X86_16        = 0x0001,  /* Intel 8086/186 */
    HV_ARCH_X86_286       = 0x0002,  /* Intel 80286 */
    HV_ARCH_X86_386       = 0x0003,  /* Intel 80386 */
    HV_ARCH_X86_486       = 0x0004,  /* Intel 80486 */
    HV_ARCH_X86_32        = 0x0005,  /* Intel Pentium+ (32-bit) */
    HV_ARCH_X86_64        = 0x0010,  /* AMD64/Intel 64 */
    HV_ARCH_RISCV32       = 0x0020,  /* RISC-V RV32 */
    HV_ARCH_RISCV64       = 0x0021,  /* RISC-V RV64 */
    HV_ARCH_MIPS_I        = 0x0030,  /* MIPS I */
    HV_ARCH_MIPS_II       = 0x0031,  /* MIPS II */
    HV_ARCH_MIPS_III      = 0x0032,  /* MIPS III */
    HV_ARCH_MIPS_IV       = 0x0033,  /* MIPS IV */
    HV_ARCH_MIPS_V        = 0x0034,  /* MIPS V */
    HV_ARCH_MIPS32        = 0x0035,  /* MIPS32 (R1-R6) */
    HV_ARCH_MIPS64        = 0x0036,  /* MIPS64 (R1-R6) */
    HV_ARCH_SPARC32       = 0x0040,  /* SPARC V8 */
    HV_ARCH_SPARC64       = 0x0041,  /* SPARC V9 */
    HV_ARCH_M68K          = 0x0050,  /* Motorola 68000 series */
    HV_ARCH_VAX           = 0x0060,  /* DEC VAX */
    HV_ARCH_ALPHA         = 0x0070,  /* DEC Alpha */
    HV_ARCH_IA64          = 0x0080,  /* Intel Itanium */
    HV_ARCH_PPC32         = 0x0090,  /* PowerPC 32-bit */
    HV_ARCH_PPC64         = 0x0091,  /* PowerPC 64-bit */
    HV_ARCH_LOONGARCH32   = 0x00A0,  /* LoongArch LA32 */
    HV_ARCH_LOONGARCH64   = 0x00A1,  /* LoongArch LA64 */
    HV_ARCH_DLX           = 0x00B0,  /* DLX (educational) */
    HV_ARCH_MMIX          = 0x00C0,  /* MMIX (Knuth) */
} HV_ARCHITECTURE;

/* Endianness */
typedef enum HV_ENDIAN {
    HV_ENDIAN_LITTLE      = 0,
    HV_ENDIAN_BIG         = 1,
    HV_ENDIAN_BI          = 2,       /* Bi-endian (runtime configurable) */
} HV_ENDIAN;

/* Virtualization mode */
typedef enum HV_VIRT_MODE {
    HV_VIRT_AUTO          = 0,       /* Auto-detect best mode */
    HV_VIRT_HARDWARE      = 1,       /* Hardware-assisted (VT-x, AMD-V, etc.) */
    HV_VIRT_SOFTWARE      = 2,       /* Software (trap-and-emulate) */
    HV_VIRT_BINARY_TRANS  = 3,       /* Binary translation (VMware/Plex86 style) */
    HV_VIRT_PARAVIRT      = 4,       /* Paravirtualization */
} HV_VIRT_MODE;

/* VM execution state */
typedef enum HV_VM_STATE {
    HV_VM_STATE_STOPPED   = 0,
    HV_VM_STATE_RUNNING   = 1,
    HV_VM_STATE_PAUSED    = 2,
    HV_VM_STATE_SUSPENDED = 3,
    HV_VM_STATE_ERROR     = 4,
} HV_VM_STATE;

/* VM exit reasons (inspired by VT-x exit reasons) */
typedef enum HV_VM_EXIT_REASON {
    HV_EXIT_EXCEPTION_NMI         = 0,
    HV_EXIT_EXTERNAL_INTERRUPT    = 1,
    HV_EXIT_TRIPLE_FAULT          = 2,
    HV_EXIT_INIT                  = 3,
    HV_EXIT_SIPI                  = 4,
    HV_EXIT_IO_INSTRUCTION        = 30,
    HV_EXIT_RDMSR                 = 31,
    HV_EXIT_WRMSR                 = 32,
    HV_EXIT_INVALID_GUEST_STATE   = 33,
    HV_EXIT_EPT_VIOLATION         = 48,
    HV_EXIT_EPT_MISCONFIGURATION  = 49,
    HV_EXIT_MEMORY_ACCESS         = 100,  /* Generic memory fault */
    HV_EXIT_PAGE_FAULT            = 101,
    HV_EXIT_SYSCALL               = 102,  /* Hypercall/paravirt syscall */
    HV_EXIT_DEVICE_IO             = 103,
    HV_EXIT_HLT                   = 104,
    HV_EXIT_SHUTDOWN              = 105,
    HV_EXIT_DEBUG                 = 106,
} HV_VM_EXIT_REASON;

/* Memory permissions */
typedef enum HV_MEMORY_FLAGS {
    HV_MEMORY_READ        = 0x01,
    HV_MEMORY_WRITE       = 0x02,
    HV_MEMORY_EXEC        = 0x04,
    HV_MEMORY_USER        = 0x08,
} HV_MEMORY_FLAGS;

/* ======================================================================= */
/* Virtual CPU register types                                              */
/* ======================================================================= */

/* Generic register value (up to 512-bit for SIMD) */
typedef union HV_REGISTER_VALUE {
    UINT8   u8;
    UINT16  u16;
    UINT32  u32;
    UINT64  u64;
    struct {
        UINT64 low;
        UINT64 high;
    } u128;
    UINT8   bytes[64];  /* For large SIMD registers */
} HV_REGISTER_VALUE;

/* Register descriptor */
typedef struct HV_REGISTER {
    UINT32             Id;           /* Architecture-specific register ID */
    HV_REGISTER_VALUE  Value;
} HV_REGISTER;

/* VM configuration */
typedef struct HV_VM_CONFIG {
    HV_ARCHITECTURE    Architecture;
    HV_ENDIAN          Endianness;
    HV_VIRT_MODE       VirtMode;
    UINT32             NumCpus;
    UINT64             MemorySize;   /* In bytes */
    BOOLEAN            EnableNestedPaging;
    BOOLEAN            EnableShadowPageTables;
    BOOLEAN            EnableBinaryTranslation;
    BOOLEAN            EnableParavirt;
} HV_VM_CONFIG;

/* VM exit information */
typedef struct HV_VM_EXIT_INFO {
    HV_VM_EXIT_REASON  Reason;
    UINT32             CpuId;
    UINT64             ExitQualification;
    UINT64             GuestLinearAddress;
    UINT64             GuestPhysicalAddress;
    UINT64             InstructionLength;
    UINT32             InterruptionType;
    UINT32             InterruptionVector;
    UINT64             ErrorCode;
} HV_VM_EXIT_INFO;

/* Memory region descriptor */
typedef struct HV_MEMORY_REGION {
    UINT64             GuestPhysicalAddress;
    UINT64             HostVirtualAddress;
    UINT64             Size;
    UINT32             Flags;  /* HV_MEMORY_FLAGS */
} HV_MEMORY_REGION;

/* ======================================================================= */
/* IHypervisor - Main hypervisor interface                                 */
/* ======================================================================= */

/* {4E5C8A10-1B2C-4D3E-8F9A-0B1C2D3E4F50} */
#define ANX_IID_IHypervisor "4E5C8A10-1B2C-4D3E-8F9A-0B1C2D3E4F50"
ANX_DEFINE_GUID(IID_IHypervisor,
    0x4E5C8A10, 0x1B2C, 0x4D3E, 0x8F, 0x9A, 0x0B, 0x1C, 0x2D, 0x3E, 0x4F, 0x50);

struct IVirtualMachine;

ANX_BEGIN_INTERFACE(IHypervisor, IUnknown, IID_IHypervisor, ANX_IID_IHypervisor)
    /**
     * Initialize the hypervisor subsystem.
     */
    ANX_IFACE_METHOD(HRESULT, Initialize, (VOID))

    /**
     * Shutdown the hypervisor subsystem.
     */
    ANX_IFACE_METHOD(HRESULT, Shutdown, (VOID))

    /**
     * Check if hypervisor is supported on this hardware/platform.
     */
    ANX_IFACE_METHOD(HRESULT, IsSupported, (
        IN HV_ARCHITECTURE Arch,
        IN HV_VIRT_MODE Mode,
        OUT BOOLEAN* pSupported))

    /**
     * Get hypervisor capabilities.
     */
    ANX_IFACE_METHOD(HRESULT, GetCapabilities, (
        IN HV_ARCHITECTURE Arch,
        OUT UINT32* pCapabilities))

    /**
     * Create a virtual machine.
     */
    ANX_IFACE_METHOD(HRESULT, CreateVM, (
        IN CONST HV_VM_CONFIG* pConfig,
        OUT struct IVirtualMachine** ppVM))

    /**
     * Enumerate supported architectures.
     */
    ANX_IFACE_METHOD(HRESULT, EnumerateArchitectures, (
        OUT HV_ARCHITECTURE* pArchitectures,
        IN OUT UINT32* pCount))
ANX_END_INTERFACE(IHypervisor, IID_IHypervisor)

/* ======================================================================= */
/* IVirtualMachine - Virtual machine instance                              */
/* ======================================================================= */

/* {5F6D7E8F-9A0B-1C2D-3E4F-506172839495} */
#define ANX_IID_IVirtualMachine "5F6D7E8F-9A0B-1C2D-3E4F-506172839495"
ANX_DEFINE_GUID(IID_IVirtualMachine,
    0x5F6D7E8F, 0x9A0B, 0x1C2D, 0x3E, 0x4F, 0x50, 0x61, 0x72, 0x83, 0x94, 0x95);

struct IVirtualCpu;
struct IVirtualMemory;
struct IVirtualDevice;

ANX_BEGIN_INTERFACE(IVirtualMachine, IUnknown, IID_IVirtualMachine, ANX_IID_IVirtualMachine)
    /**
     * Get VM configuration.
     */
    ANX_IFACE_METHOD(HRESULT, GetConfig, (
        OUT HV_VM_CONFIG* pConfig))

    /**
     * Get VM state.
     */
    ANX_IFACE_METHOD(HRESULT, GetState, (
        OUT HV_VM_STATE* pState))

    /**
     * Start the virtual machine.
     */
    ANX_IFACE_METHOD(HRESULT, Start, (VOID))

    /**
     * Stop the virtual machine.
     */
    ANX_IFACE_METHOD(HRESULT, Stop, (VOID))

    /**
     * Pause the virtual machine.
     */
    ANX_IFACE_METHOD(HRESULT, Pause, (VOID))

    /**
     * Resume the virtual machine.
     */
    ANX_IFACE_METHOD(HRESULT, Resume, (VOID))

    /**
     * Reset the virtual machine.
     */
    ANX_IFACE_METHOD(HRESULT, Reset, (VOID))

    /**
     * Get a virtual CPU interface.
     */
    ANX_IFACE_METHOD(HRESULT, GetVirtualCpu, (
        IN UINT32 CpuId,
        OUT struct IVirtualCpu** ppCpu))

    /**
     * Get the virtual memory interface.
     */
    ANX_IFACE_METHOD(HRESULT, GetVirtualMemory, (
        OUT struct IVirtualMemory** ppMemory))

    /**
     * Attach a virtual device.
     */
    ANX_IFACE_METHOD(HRESULT, AttachDevice, (
        IN struct IVirtualDevice* pDevice))

    /**
     * Detach a virtual device.
     */
    ANX_IFACE_METHOD(HRESULT, DetachDevice, (
        IN struct IVirtualDevice* pDevice))
ANX_END_INTERFACE(IVirtualMachine, IID_IVirtualMachine)

/* ======================================================================= */
/* IVirtualCpu - Virtual CPU interface                                     */
/* ======================================================================= */

/* {607182A3-B4C5-D6E7-F809-1A2B3C4D5E6F} */
#define ANX_IID_IVirtualCpu "607182A3-B4C5-D6E7-F809-1A2B3C4D5E6F"
ANX_DEFINE_GUID(IID_IVirtualCpu,
    0x607182A3, 0xB4C5, 0xD6E7, 0xF8, 0x09, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F);

ANX_BEGIN_INTERFACE(IVirtualCpu, IUnknown, IID_IVirtualCpu, ANX_IID_IVirtualCpu)
    /**
     * Get CPU ID.
     */
    ANX_IFACE_METHOD(HRESULT, GetId, (
        OUT UINT32* pCpuId))

    /**
     * Run the virtual CPU until exit.
     */
    ANX_IFACE_METHOD(HRESULT, Run, (
        OUT HV_VM_EXIT_INFO* pExitInfo))

    /**
     * Interrupt the virtual CPU.
     */
    ANX_IFACE_METHOD(HRESULT, Interrupt, (
        IN UINT32 Vector))

    /**
     * Read register value.
     */
    ANX_IFACE_METHOD(HRESULT, ReadRegister, (
        IN UINT32 RegisterId,
        OUT HV_REGISTER_VALUE* pValue))

    /**
     * Write register value.
     */
    ANX_IFACE_METHOD(HRESULT, WriteRegister, (
        IN UINT32 RegisterId,
        IN CONST HV_REGISTER_VALUE* pValue))

    /**
     * Read multiple registers.
     */
    ANX_IFACE_METHOD(HRESULT, ReadRegisters, (
        IN UINT32 Count,
        IN OUT HV_REGISTER* pRegisters))

    /**
     * Write multiple registers.
     */
    ANX_IFACE_METHOD(HRESULT, WriteRegisters, (
        IN UINT32 Count,
        IN CONST HV_REGISTER* pRegisters))

    /**
     * Get instruction pointer.
     */
    ANX_IFACE_METHOD(HRESULT, GetInstructionPointer, (
        OUT UINT64* pIP))

    /**
     * Set instruction pointer.
     */
    ANX_IFACE_METHOD(HRESULT, SetInstructionPointer, (
        IN UINT64 IP))

    /**
     * Single-step execution.
     */
    ANX_IFACE_METHOD(HRESULT, SingleStep, (
        OUT HV_VM_EXIT_INFO* pExitInfo))
ANX_END_INTERFACE(IVirtualCpu, IID_IVirtualCpu)

/* ======================================================================= */
/* IVirtualMemory - Virtual memory management                              */
/* ======================================================================= */

/* {718293A4-B5C6-D7E8-F910-2B3C4D5E6F70} */
#define ANX_IID_IVirtualMemory "718293A4-B5C6-D7E8-F910-2B3C4D5E6F70"
ANX_DEFINE_GUID(IID_IVirtualMemory,
    0x718293A4, 0xB5C6, 0xD7E8, 0xF9, 0x10, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F, 0x70);

ANX_BEGIN_INTERFACE(IVirtualMemory, IUnknown, IID_IVirtualMemory, ANX_IID_IVirtualMemory)
    /**
     * Map guest physical memory to host virtual memory.
     */
    ANX_IFACE_METHOD(HRESULT, MapMemory, (
        IN UINT64 GuestPhysicalAddress,
        IN UINT64 HostVirtualAddress,
        IN UINT64 Size,
        IN UINT32 Flags))

    /**
     * Unmap guest physical memory.
     */
    ANX_IFACE_METHOD(HRESULT, UnmapMemory, (
        IN UINT64 GuestPhysicalAddress,
        IN UINT64 Size))

    /**
     * Protect memory region.
     */
    ANX_IFACE_METHOD(HRESULT, ProtectMemory, (
        IN UINT64 GuestPhysicalAddress,
        IN UINT64 Size,
        IN UINT32 Flags))

    /**
     * Read guest physical memory.
     */
    ANX_IFACE_METHOD(HRESULT, ReadMemory, (
        IN UINT64 GuestPhysicalAddress,
        OUT VOID* Buffer,
        IN UINT64 Size))

    /**
     * Write guest physical memory.
     */
    ANX_IFACE_METHOD(HRESULT, WriteMemory, (
        IN UINT64 GuestPhysicalAddress,
        IN CONST VOID* Buffer,
        IN UINT64 Size))

    /**
     * Translate guest virtual to guest physical address.
     */
    ANX_IFACE_METHOD(HRESULT, TranslateGVA, (
        IN UINT32 CpuId,
        IN UINT64 GuestVirtualAddress,
        OUT UINT64* pGuestPhysicalAddress))

    /**
     * Query memory regions.
     */
    ANX_IFACE_METHOD(HRESULT, QueryMemoryRegions, (
        OUT HV_MEMORY_REGION* pRegions,
        IN OUT UINT32* pCount))

    /**
     * Flush TLB.
     */
    ANX_IFACE_METHOD(HRESULT, FlushTLB, (
        IN UINT32 CpuId))
ANX_END_INTERFACE(IVirtualMemory, IID_IVirtualMemory)

/* ======================================================================= */
/* IVirtualDevice - Virtual device interface                               */
/* ======================================================================= */

/* {829304B5-C6D7-E8F9-0A12-3C4D5E6F7081} */
#define ANX_IID_IVirtualDevice "829304B5-C6D7-E8F9-0A12-3C4D5E6F7081"
ANX_DEFINE_GUID(IID_IVirtualDevice,
    0x829304B5, 0xC6D7, 0xE8F9, 0x0A, 0x12, 0x3C, 0x4D, 0x5E, 0x6F, 0x70, 0x81);

typedef enum HV_DEVICE_TYPE {
    HV_DEVICE_GENERIC     = 0,
    HV_DEVICE_DISK        = 1,
    HV_DEVICE_NETWORK     = 2,
    HV_DEVICE_DISPLAY     = 3,
    HV_DEVICE_INPUT       = 4,
    HV_DEVICE_SERIAL      = 5,
    HV_DEVICE_PCI         = 6,
    HV_DEVICE_USB         = 7,
} HV_DEVICE_TYPE;

ANX_BEGIN_INTERFACE(IVirtualDevice, IUnknown, IID_IVirtualDevice, ANX_IID_IVirtualDevice)
    /**
     * Get device type.
     */
    ANX_IFACE_METHOD(HRESULT, GetDeviceType, (
        OUT HV_DEVICE_TYPE* pType))

    /**
     * Get device name.
     */
    ANX_IFACE_METHOD(HRESULT, GetDeviceName, (
        OUT CHAR8* Buffer,
        IN OUT UINT32* pSize))

    /**
     * Initialize device.
     */
    ANX_IFACE_METHOD(HRESULT, Initialize, (
        IN struct IVirtualMachine* pVM))

    /**
     * Shutdown device.
     */
    ANX_IFACE_METHOD(HRESULT, Shutdown, (VOID))

    /**
     * Reset device.
     */
    ANX_IFACE_METHOD(HRESULT, Reset, (VOID))

    /**
     * Read from device I/O port.
     */
    ANX_IFACE_METHOD(HRESULT, IORead, (
        IN UINT64 Port,
        IN UINT32 Size,
        OUT UINT64* pValue))

    /**
     * Write to device I/O port.
     */
    ANX_IFACE_METHOD(HRESULT, IOWrite, (
        IN UINT64 Port,
        IN UINT32 Size,
        IN UINT64 Value))

    /**
     * Read from device memory region.
     */
    ANX_IFACE_METHOD(HRESULT, MemoryRead, (
        IN UINT64 Offset,
        OUT VOID* Buffer,
        IN UINT64 Size))

    /**
     * Write to device memory region.
     */
    ANX_IFACE_METHOD(HRESULT, MemoryWrite, (
        IN UINT64 Offset,
        IN CONST VOID* Buffer,
        IN UINT64 Size))
ANX_END_INTERFACE(IVirtualDevice, IID_IVirtualDevice)

/* ======================================================================= */
/* Architecture-specific register IDs                                      */
/* ======================================================================= */

/* x86/x86_64 registers */
typedef enum HV_X86_REGISTER {
    /* General purpose registers */
    HV_X86_RAX = 0, HV_X86_RCX, HV_X86_RDX, HV_X86_RBX,
    HV_X86_RSP, HV_X86_RBP, HV_X86_RSI, HV_X86_RDI,
    HV_X86_R8,  HV_X86_R9,  HV_X86_R10, HV_X86_R11,
    HV_X86_R12, HV_X86_R13, HV_X86_R14, HV_X86_R15,

    /* Instruction pointer and flags */
    HV_X86_RIP = 16,
    HV_X86_RFLAGS = 17,

    /* Segment registers */
    HV_X86_CS = 18, HV_X86_DS, HV_X86_ES, HV_X86_FS, HV_X86_GS, HV_X86_SS,

    /* Control registers */
    HV_X86_CR0 = 24, HV_X86_CR2, HV_X86_CR3, HV_X86_CR4, HV_X86_CR8,

    /* Debug registers */
    HV_X86_DR0 = 30, HV_X86_DR1, HV_X86_DR2, HV_X86_DR3,
    HV_X86_DR6 = 34, HV_X86_DR7,

    /* Descriptor tables */
    HV_X86_GDTR = 40, HV_X86_IDTR, HV_X86_LDTR, HV_X86_TR,

    /* MSRs (Model-Specific Registers) */
    HV_X86_EFER = 50,
    HV_X86_KERNEL_GS_BASE = 51,
    HV_X86_APIC_BASE = 52,

    /* 286-specific */
    HV_X86_MSW = 60,  /* Machine Status Word (286) */
} HV_X86_REGISTER;

/* RISC-V registers */
typedef enum HV_RISCV_REGISTER {
    /* Integer registers */
    HV_RISCV_X0 = 0,  /* zero */
    HV_RISCV_X1,      /* ra */
    HV_RISCV_X2,      /* sp */
    /* ... x3-x31 */

    /* PC */
    HV_RISCV_PC = 32,

    /* CSRs */
    HV_RISCV_SSTATUS = 100,
    HV_RISCV_SIE,
    HV_RISCV_STVEC,
    HV_RISCV_SSCRATCH,
    HV_RISCV_SEPC,
    HV_RISCV_SCAUSE,
    HV_RISCV_STVAL,
    HV_RISCV_SIP,
    HV_RISCV_SATP,
} HV_RISCV_REGISTER;

/* Additional architecture register enums would go here... */

/* ======================================================================= */
/* Factory function                                                        */
/* ======================================================================= */

/**
 * Create the hypervisor instance.
 *
 * @param riid      Interface ID to query
 * @param ppvObject Receives the interface pointer
 * @return S_OK on success, error code otherwise
 */
HRESULT
HvCreateHypervisor(
    IN  REFIID riid,
    OUT VOID** ppvObject
);
