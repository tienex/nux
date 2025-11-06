/*++
    Module Name:

        x86_backend.c

    Abstract:

        x86 architecture virtualization backend (286, 386+, and x86_64).
        Supports both hardware-assisted (VT-x) and software virtualization
        (trap-and-emulate, binary translation).

--*/

#include "../hypervisor_impl.h"

/* ======================================================================= */
/* x86 CPU context                                                         */
/* ======================================================================= */

typedef struct HV_X86_CONTEXT {
    /* General purpose registers */
    UINT64 rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi;
    UINT64 r8, r9, r10, r11, r12, r13, r14, r15;

    /* Instruction pointer and flags */
    UINT64 rip;
    UINT64 rflags;

    /* Segment registers */
    UINT16 cs, ds, es, fs, gs, ss;
    UINT64 cs_base, ds_base, es_base, fs_base, gs_base, ss_base;
    UINT32 cs_limit, ds_limit, es_limit, fs_limit, gs_limit, ss_limit;
    UINT32 cs_attr, ds_attr, es_attr, fs_attr, gs_attr, ss_attr;

    /* Control registers */
    UINT64 cr0, cr2, cr3, cr4, cr8;

    /* Debug registers */
    UINT64 dr0, dr1, dr2, dr3, dr6, dr7;

    /* Descriptor tables */
    UINT64 gdtr_base, idtr_base;
    UINT16 gdtr_limit, idtr_limit;
    UINT16 ldtr, tr;
    UINT64 ldtr_base, tr_base;
    UINT32 ldtr_limit, tr_limit;
    UINT32 ldtr_attr, tr_attr;

    /* MSRs */
    UINT64 efer;
    UINT64 kernel_gs_base;
    UINT64 apic_base;

    /* 286-specific */
    UINT16 msw;  /* Machine Status Word (286) */

    /* CPU mode */
    UINT32 mode;  /* 0=real, 1=protected, 2=long */

    /* Hardware virtualization context */
    VOID* vmcs;  /* VT-x VMCS or AMD-V VMCB */
} HV_X86_CONTEXT;

/* ======================================================================= */
/* x86 backend operations                                                  */
/* ======================================================================= */

static HRESULT
HvX86_Initialize(
    HvVirtualCpu* pCpu
)
{
    HV_X86_CONTEXT* pContext;

    /* Allocate context */
    pContext = (HV_X86_CONTEXT*)RtlAllocateMemory(&pCpu->VM->Pool, sizeof(HV_X86_CONTEXT));
    if (pContext == NULL) {
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pContext, sizeof(HV_X86_CONTEXT));

    /* Initialize to real mode (for 286/386) */
    pContext->mode = 0;
    pContext->cs = 0xF000;
    pContext->cs_base = 0xF0000;
    pContext->rip = 0xFFF0;  /* Reset vector */

    /* Set CR0 for protected mode support */
    pContext->cr0 = 0x00000010;  /* ET bit */

    /* 286: Initialize MSW */
    pContext->msw = (UINT16)(pContext->cr0 & 0xFFFF);

    pCpu->HwContext = pContext;
    return S_OK;
}

static HRESULT
HvX86_Shutdown(
    HvVirtualCpu* pCpu
)
{
    if (pCpu->HwContext != NULL) {
        /* Free VMCS/VMCB if allocated */
        HV_X86_CONTEXT* pContext = (HV_X86_CONTEXT*)pCpu->HwContext;
        if (pContext->vmcs != NULL) {
            RtlFreeMemory(&pCpu->VM->Pool, pContext->vmcs);
        }

        RtlFreeMemory(&pCpu->VM->Pool, pCpu->HwContext);
        pCpu->HwContext = NULL;
    }

    return S_OK;
}

static HRESULT
HvX86_ReadRegister(
    HvVirtualCpu* pCpu,
    UINT32 RegId,
    HV_REGISTER_VALUE* pValue
)
{
    HV_X86_CONTEXT* pContext = (HV_X86_CONTEXT*)pCpu->HwContext;

    if (pContext == NULL || pValue == NULL) {
        return E_POINTER;
    }

    RtlZeroMemory(pValue, sizeof(HV_REGISTER_VALUE));

    switch (RegId) {
        /* General purpose registers */
        case HV_X86_RAX: pValue->u64 = pContext->rax; break;
        case HV_X86_RCX: pValue->u64 = pContext->rcx; break;
        case HV_X86_RDX: pValue->u64 = pContext->rdx; break;
        case HV_X86_RBX: pValue->u64 = pContext->rbx; break;
        case HV_X86_RSP: pValue->u64 = pContext->rsp; break;
        case HV_X86_RBP: pValue->u64 = pContext->rbp; break;
        case HV_X86_RSI: pValue->u64 = pContext->rsi; break;
        case HV_X86_RDI: pValue->u64 = pContext->rdi; break;
        case HV_X86_R8:  pValue->u64 = pContext->r8; break;
        case HV_X86_R9:  pValue->u64 = pContext->r9; break;
        case HV_X86_R10: pValue->u64 = pContext->r10; break;
        case HV_X86_R11: pValue->u64 = pContext->r11; break;
        case HV_X86_R12: pValue->u64 = pContext->r12; break;
        case HV_X86_R13: pValue->u64 = pContext->r13; break;
        case HV_X86_R14: pValue->u64 = pContext->r14; break;
        case HV_X86_R15: pValue->u64 = pContext->r15; break;

        /* Instruction pointer and flags */
        case HV_X86_RIP: pValue->u64 = pContext->rip; break;
        case HV_X86_RFLAGS: pValue->u64 = pContext->rflags; break;

        /* Segment registers */
        case HV_X86_CS: pValue->u16 = pContext->cs; break;
        case HV_X86_DS: pValue->u16 = pContext->ds; break;
        case HV_X86_ES: pValue->u16 = pContext->es; break;
        case HV_X86_FS: pValue->u16 = pContext->fs; break;
        case HV_X86_GS: pValue->u16 = pContext->gs; break;
        case HV_X86_SS: pValue->u16 = pContext->ss; break;

        /* Control registers */
        case HV_X86_CR0: pValue->u64 = pContext->cr0; break;
        case HV_X86_CR2: pValue->u64 = pContext->cr2; break;
        case HV_X86_CR3: pValue->u64 = pContext->cr3; break;
        case HV_X86_CR4: pValue->u64 = pContext->cr4; break;
        case HV_X86_CR8: pValue->u64 = pContext->cr8; break;

        /* Debug registers */
        case HV_X86_DR0: pValue->u64 = pContext->dr0; break;
        case HV_X86_DR1: pValue->u64 = pContext->dr1; break;
        case HV_X86_DR2: pValue->u64 = pContext->dr2; break;
        case HV_X86_DR3: pValue->u64 = pContext->dr3; break;
        case HV_X86_DR6: pValue->u64 = pContext->dr6; break;
        case HV_X86_DR7: pValue->u64 = pContext->dr7; break;

        /* MSRs */
        case HV_X86_EFER: pValue->u64 = pContext->efer; break;
        case HV_X86_KERNEL_GS_BASE: pValue->u64 = pContext->kernel_gs_base; break;
        case HV_X86_APIC_BASE: pValue->u64 = pContext->apic_base; break;

        /* 286-specific */
        case HV_X86_MSW: pValue->u16 = pContext->msw; break;

        default:
            return E_INVALIDARG;
    }

    return S_OK;
}

static HRESULT
HvX86_WriteRegister(
    HvVirtualCpu* pCpu,
    UINT32 RegId,
    CONST HV_REGISTER_VALUE* pValue
)
{
    HV_X86_CONTEXT* pContext = (HV_X86_CONTEXT*)pCpu->HwContext;

    if (pContext == NULL || pValue == NULL) {
        return E_POINTER;
    }

    switch (RegId) {
        /* General purpose registers */
        case HV_X86_RAX: pContext->rax = pValue->u64; break;
        case HV_X86_RCX: pContext->rcx = pValue->u64; break;
        case HV_X86_RDX: pContext->rdx = pValue->u64; break;
        case HV_X86_RBX: pContext->rbx = pValue->u64; break;
        case HV_X86_RSP: pContext->rsp = pValue->u64; break;
        case HV_X86_RBP: pContext->rbp = pValue->u64; break;
        case HV_X86_RSI: pContext->rsi = pValue->u64; break;
        case HV_X86_RDI: pContext->rdi = pValue->u64; break;
        case HV_X86_R8:  pContext->r8 = pValue->u64; break;
        case HV_X86_R9:  pContext->r9 = pValue->u64; break;
        case HV_X86_R10: pContext->r10 = pValue->u64; break;
        case HV_X86_R11: pContext->r11 = pValue->u64; break;
        case HV_X86_R12: pContext->r12 = pValue->u64; break;
        case HV_X86_R13: pContext->r13 = pValue->u64; break;
        case HV_X86_R14: pContext->r14 = pValue->u64; break;
        case HV_X86_R15: pContext->r15 = pValue->u64; break;

        /* Instruction pointer and flags */
        case HV_X86_RIP: pContext->rip = pValue->u64; break;
        case HV_X86_RFLAGS: pContext->rflags = pValue->u64; break;

        /* Segment registers */
        case HV_X86_CS: pContext->cs = pValue->u16; break;
        case HV_X86_DS: pContext->ds = pValue->u16; break;
        case HV_X86_ES: pContext->es = pValue->u16; break;
        case HV_X86_FS: pContext->fs = pValue->u16; break;
        case HV_X86_GS: pContext->gs = pValue->u16; break;
        case HV_X86_SS: pContext->ss = pValue->u16; break;

        /* Control registers */
        case HV_X86_CR0:
            pContext->cr0 = pValue->u64;
            pContext->msw = (UINT16)(pContext->cr0 & 0xFFFF);
            /* Update mode based on CR0 */
            if (pContext->cr0 & 0x80000000) {
                pContext->mode = 1;  /* Protected mode with paging */
            } else if (pContext->cr0 & 0x1) {
                pContext->mode = 1;  /* Protected mode */
            } else {
                pContext->mode = 0;  /* Real mode */
            }
            break;
        case HV_X86_CR2: pContext->cr2 = pValue->u64; break;
        case HV_X86_CR3: pContext->cr3 = pValue->u64; break;
        case HV_X86_CR4: pContext->cr4 = pValue->u64; break;
        case HV_X86_CR8: pContext->cr8 = pValue->u64; break;

        /* Debug registers */
        case HV_X86_DR0: pContext->dr0 = pValue->u64; break;
        case HV_X86_DR1: pContext->dr1 = pValue->u64; break;
        case HV_X86_DR2: pContext->dr2 = pValue->u64; break;
        case HV_X86_DR3: pContext->dr3 = pValue->u64; break;
        case HV_X86_DR6: pContext->dr6 = pValue->u64; break;
        case HV_X86_DR7: pContext->dr7 = pValue->u64; break;

        /* MSRs */
        case HV_X86_EFER:
            pContext->efer = pValue->u64;
            /* Update mode based on EFER.LME */
            if (pContext->efer & 0x100) {
                pContext->mode = 2;  /* Long mode */
            }
            break;
        case HV_X86_KERNEL_GS_BASE: pContext->kernel_gs_base = pValue->u64; break;
        case HV_X86_APIC_BASE: pContext->apic_base = pValue->u64; break;

        /* 286-specific */
        case HV_X86_MSW:
            pContext->msw = pValue->u16;
            /* Update CR0 from MSW */
            pContext->cr0 = (pContext->cr0 & 0xFFFFFFFFFFFF0000ULL) | pContext->msw;
            break;

        default:
            return E_INVALIDARG;
    }

    return S_OK;
}

static HRESULT
HvX86_Run(
    HvVirtualCpu* pCpu,
    HV_VM_EXIT_INFO* pExitInfo
)
{
    HV_X86_CONTEXT* pContext = (HV_X86_CONTEXT*)pCpu->HwContext;

    if (pContext == NULL || pExitInfo == NULL) {
        return E_POINTER;
    }

    RtlZeroMemory(pExitInfo, sizeof(HV_VM_EXIT_INFO));

    /* For software virtualization, we would:
     * 1. Check if instruction at RIP is in translation cache
     * 2. If not, translate it (binary translation)
     * 3. Execute translated code
     * 4. Handle any exceptions/VM exits
     *
     * For hardware virtualization (VT-x), we would:
     * 1. Enter VMX non-root mode via VMLAUNCH/VMRESUME
     * 2. Execute guest code
     * 3. Handle VM exits
     */

    /* Simplified stub: simulate HLT instruction exit */
    pExitInfo->Reason = HV_EXIT_HLT;
    pExitInfo->CpuId = pCpu->CpuId;
    pExitInfo->GuestLinearAddress = pContext->rip;

    /* Advance RIP (HLT is 1 byte) */
    pContext->rip++;

    return S_OK;
}

static HRESULT
HvX86_HandleExit(
    HvVirtualCpu* pCpu,
    HV_VM_EXIT_INFO* pExitInfo
)
{
    /* Handle VM exit based on reason */
    switch (pExitInfo->Reason) {
        case HV_EXIT_HLT:
            /* Halt CPU */
            pCpu->State = HV_VM_STATE_PAUSED;
            break;

        case HV_EXIT_IO_INSTRUCTION:
            /* Handle I/O port access */
            break;

        case HV_EXIT_RDMSR:
        case HV_EXIT_WRMSR:
            /* Handle MSR access */
            break;

        case HV_EXIT_EPT_VIOLATION:
        case HV_EXIT_PAGE_FAULT:
            /* Handle memory fault */
            break;

        default:
            return HV_UNSUPPORTED;
    }

    return S_OK;
}

static HRESULT
HvX86_TranslateInstruction(
    HvVirtualCpu* pCpu,
    UINT64 GuestAddr,
    VOID* HostAddr,
    UINT32* pSize
)
{
    /* Binary translation stub
     * This would translate guest instructions to host instructions
     * Similar to VMware's binary translation or Plex86 v1
     */

    (VOID)pCpu;
    (VOID)GuestAddr;
    (VOID)HostAddr;

    if (pSize != NULL) {
        *pSize = 0;
    }

    return HV_UNSUPPORTED;
}

/* Backend operations */
static HV_CPU_BACKEND_OPS g_X86BackendOps = {
    HvX86_Initialize,
    HvX86_Shutdown,
    HvX86_Run,
    HvX86_ReadRegister,
    HvX86_WriteRegister,
    HvX86_HandleExit,
    HvX86_TranslateInstruction
};

/* ======================================================================= */
/* Backend creation functions                                              */
/* ======================================================================= */

HRESULT
HvBackend_X86_Create(
    HV_VIRT_MODE Mode,
    HV_CPU_BACKEND** ppBackend
)
{
    HV_CPU_BACKEND* pBackend;

    pBackend = (HV_CPU_BACKEND*)RtlAllocateMemory(NULL, sizeof(HV_CPU_BACKEND));
    if (pBackend == NULL) {
        return HV_NO_RESOURCES;
    }

    pBackend->Architecture = HV_ARCH_X86_286;  /* Also supports 386+ */
    pBackend->Mode = Mode;
    pBackend->Ops = &g_X86BackendOps;
    pBackend->PrivateData = NULL;

    *ppBackend = pBackend;
    return S_OK;
}

HRESULT
HvBackend_X86_64_Create(
    HV_VIRT_MODE Mode,
    HV_CPU_BACKEND** ppBackend
)
{
    HV_CPU_BACKEND* pBackend;

    pBackend = (HV_CPU_BACKEND*)RtlAllocateMemory(NULL, sizeof(HV_CPU_BACKEND));
    if (pBackend == NULL) {
        return HV_NO_RESOURCES;
    }

    pBackend->Architecture = HV_ARCH_X86_64;
    pBackend->Mode = Mode;
    pBackend->Ops = &g_X86BackendOps;  /* Reuse x86 ops */
    pBackend->PrivateData = NULL;

    *ppBackend = pBackend;
    return S_OK;
}
