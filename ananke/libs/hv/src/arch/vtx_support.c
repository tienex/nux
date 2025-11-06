/*++
    Module Name:

        vtx_support.c

    Abstract:

        Intel VT-x hardware virtualization support.
        Implements VMCS management, VM entry/exit handling, and EPT.

--*/

#include "../hypervisor_impl.h"

/* ======================================================================= */
/* Intel VT-x structures and constants                                     */
/* ======================================================================= */

/* VMCS encoding */
#define VMCS_GUEST_ES_SELECTOR          0x00000800
#define VMCS_GUEST_CS_SELECTOR          0x00000802
#define VMCS_GUEST_SS_SELECTOR          0x00000804
#define VMCS_GUEST_DS_SELECTOR          0x00000806
#define VMCS_GUEST_FS_SELECTOR          0x00000808
#define VMCS_GUEST_GS_SELECTOR          0x0000080A
#define VMCS_GUEST_LDTR_SELECTOR        0x0000080C
#define VMCS_GUEST_TR_SELECTOR          0x0000080E

#define VMCS_GUEST_CR0                  0x00006800
#define VMCS_GUEST_CR3                  0x00006802
#define VMCS_GUEST_CR4                  0x00006804
#define VMCS_GUEST_RIP                  0x0000681E
#define VMCS_GUEST_RSP                  0x0000681C
#define VMCS_GUEST_RFLAGS               0x00006820

#define VMCS_CTRL_PIN_BASED             0x00004000
#define VMCS_CTRL_PROC_BASED            0x00004002
#define VMCS_CTRL_PROC_BASED2           0x0000401E
#define VMCS_CTRL_EXIT                  0x0000400C
#define VMCS_CTRL_ENTRY                 0x00004012

#define VMCS_EXIT_REASON                0x00004402
#define VMCS_EXIT_QUALIFICATION         0x00006400
#define VMCS_EXIT_INSTRUCTION_LENGTH    0x0000440C

#define VMCS_EPT_POINTER                0x0000201A

/* VM exit reasons */
#define EXIT_REASON_EXCEPTION_NMI       0
#define EXIT_REASON_EXTERNAL_INTERRUPT  1
#define EXIT_REASON_TRIPLE_FAULT        2
#define EXIT_REASON_INIT                3
#define EXIT_REASON_SIPI                4
#define EXIT_REASON_IO_INSTRUCTION      30
#define EXIT_REASON_MSR_READ            31
#define EXIT_REASON_MSR_WRITE           32
#define EXIT_REASON_EPT_VIOLATION       48
#define EXIT_REASON_EPT_MISCONFIGURATION 49

/* Processor-based VM-execution controls */
#define CPU_BASED_VIRTUAL_INTR_PENDING  (1 << 2)
#define CPU_BASED_USE_TSC_OFFSETING     (1 << 3)
#define CPU_BASED_HLT_EXITING           (1 << 7)
#define CPU_BASED_INVLPG_EXITING        (1 << 9)
#define CPU_BASED_MWAIT_EXITING         (1 << 10)
#define CPU_BASED_RDPMC_EXITING         (1 << 11)
#define CPU_BASED_RDTSC_EXITING         (1 << 12)
#define CPU_BASED_CR3_LOAD_EXITING      (1 << 15)
#define CPU_BASED_CR3_STORE_EXITING     (1 << 16)
#define CPU_BASED_CR8_LOAD_EXITING      (1 << 19)
#define CPU_BASED_CR8_STORE_EXITING     (1 << 20)
#define CPU_BASED_MOV_DR_EXITING        (1 << 23)
#define CPU_BASED_UNCOND_IO_EXITING     (1 << 24)
#define CPU_BASED_USE_IO_BITMAPS        (1 << 25)
#define CPU_BASED_USE_MSR_BITMAPS       (1 << 28)
#define CPU_BASED_MONITOR_EXITING       (1 << 29)
#define CPU_BASED_PAUSE_EXITING         (1 << 30)
#define CPU_BASED_ACTIVATE_SECONDARY_CONTROLS (1 << 31)

/* Secondary processor-based VM-execution controls */
#define SECONDARY_EXEC_VIRTUALIZE_APIC_ACCESSES (1 << 0)
#define SECONDARY_EXEC_ENABLE_EPT       (1 << 1)
#define SECONDARY_EXEC_RDTSCP           (1 << 3)
#define SECONDARY_EXEC_ENABLE_VPID      (1 << 5)
#define SECONDARY_EXEC_WBINVD_EXITING   (1 << 6)
#define SECONDARY_EXEC_UNRESTRICTED_GUEST (1 << 7)
#define SECONDARY_EXEC_PAUSE_LOOP_EXITING (1 << 10)

/* EPT page table entry flags */
#define EPT_READ        (1 << 0)
#define EPT_WRITE       (1 << 1)
#define EPT_EXECUTE     (1 << 2)
#define EPT_MEMORY_TYPE_WB (6 << 3)  /* Write-back */

/* ======================================================================= */
/* VMCS structure                                                          */
/* ======================================================================= */

typedef struct HV_VMCS {
    UINT32 Revision;
    UINT32 AbortIndicator;
    UINT8  Data[4088];  /* Rest of 4KB VMCS */
} HV_VMCS;

/* VT-x context per virtual CPU */
typedef struct HV_VTX_CONTEXT {
    HV_VMCS* Vmcs;
    BOOLEAN VmcsLaunched;

    /* EPT structures */
    UINT64* EptPml4;
    UINT64* EptPdpt;
    UINT64* EptPd;
    UINT64* EptPt;

    /* MSR bitmaps */
    UINT8* MsrBitmap;

    /* I/O bitmaps */
    UINT8* IoBitmapA;  /* Ports 0x0000-0x7FFF */
    UINT8* IoBitmapB;  /* Ports 0x8000-0xFFFF */

    /* Host state (saved before VM entry) */
    UINT64 HostCr0;
    UINT64 HostCr3;
    UINT64 HostCr4;
    UINT64 HostRsp;
    UINT64 HostRip;
} HV_VTX_CONTEXT;

/* ======================================================================= */
/* VT-x capability detection                                               */
/* ======================================================================= */

BOOLEAN
HvVtxIsSupported(VOID)
{
    /* Check CPUID for VMX support */
    /* CPUID.1:ECX.VMX[bit 5] = 1 */

    /* In a real implementation:
     * 1. Execute CPUID with EAX=1
     * 2. Check ECX bit 5 for VMX support
     * 3. Check IA32_FEATURE_CONTROL MSR for lock and VMX enable
     */

    return FALSE;  /* Stub - would check actual CPU capabilities */
}

HRESULT
HvVtxGetCapabilities(
    UINT32* pPinCtls,
    UINT32* pProcCtls,
    UINT32* pProc2Ctls,
    UINT32* pExitCtls,
    UINT32* pEntryCtls
)
{
    /* Read VMX capability MSRs */
    /* IA32_VMX_PINBASED_CTLS, IA32_VMX_PROCBASED_CTLS, etc. */

    if (pPinCtls) *pPinCtls = 0;
    if (pProcCtls) *pProcCtls = 0;
    if (pProc2Ctls) *pProc2Ctls = 0;
    if (pExitCtls) *pExitCtls = 0;
    if (pEntryCtls) *pEntryCtls = 0;

    return HV_UNSUPPORTED;  /* Stub */
}

/* ======================================================================= */
/* VMCS operations (stubs - would use VMREAD/VMWRITE instructions)        */
/* ======================================================================= */

static HRESULT
VmcsRead(
    HV_VMCS* Vmcs,
    UINT64 Field,
    UINT64* pValue
)
{
    /* In real implementation, execute VMREAD instruction */
    (VOID)Vmcs;
    (VOID)Field;
    if (pValue) *pValue = 0;
    return HV_UNSUPPORTED;
}

static HRESULT
VmcsWrite(
    HV_VMCS* Vmcs,
    UINT64 Field,
    UINT64 Value
)
{
    /* In real implementation, execute VMWRITE instruction */
    (VOID)Vmcs;
    (VOID)Field;
    (VOID)Value;
    return HV_UNSUPPORTED;
}

static HRESULT
VmcsClear(
    HV_VMCS* Vmcs
)
{
    /* In real implementation, execute VMCLEAR instruction */
    (VOID)Vmcs;
    return HV_UNSUPPORTED;
}

static HRESULT
VmptrldVMCS(
    HV_VMCS* Vmcs
)
{
    /* In real implementation, execute VMPTRLD instruction */
    (VOID)Vmcs;
    return HV_UNSUPPORTED;
}

/* ======================================================================= */
/* EPT (Extended Page Tables) management                                   */
/* ======================================================================= */

HRESULT
HvVtxEptInitialize(
    HV_VTX_CONTEXT* pContext
)
{
    if (pContext == NULL) {
        return E_POINTER;
    }

    /* Allocate EPT page tables (4-level) */
    pContext->EptPml4 = (UINT64*)RtlAllocateMemory(NULL, 4096);
    pContext->EptPdpt = (UINT64*)RtlAllocateMemory(NULL, 4096);
    pContext->EptPd = (UINT64*)RtlAllocateMemory(NULL, 4096);
    pContext->EptPt = (UINT64*)RtlAllocateMemory(NULL, 4096);

    if (!pContext->EptPml4 || !pContext->EptPdpt ||
        !pContext->EptPd || !pContext->EptPt) {
        if (pContext->EptPml4) RtlFreeMemory(NULL, pContext->EptPml4);
        if (pContext->EptPdpt) RtlFreeMemory(NULL, pContext->EptPdpt);
        if (pContext->EptPd) RtlFreeMemory(NULL, pContext->EptPd);
        if (pContext->EptPt) RtlFreeMemory(NULL, pContext->EptPt);
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pContext->EptPml4, 4096);
    RtlZeroMemory(pContext->EptPdpt, 4096);
    RtlZeroMemory(pContext->EptPd, 4096);
    RtlZeroMemory(pContext->EptPt, 4096);

    /* Setup PML4 -> PDPT -> PD -> PT hierarchy */
    pContext->EptPml4[0] = (UINT64)(UINTN)pContext->EptPdpt |
        (EPT_READ | EPT_WRITE | EPT_EXECUTE);

    pContext->EptPdpt[0] = (UINT64)(UINTN)pContext->EptPd |
        (EPT_READ | EPT_WRITE | EPT_EXECUTE);

    pContext->EptPd[0] = (UINT64)(UINTN)pContext->EptPt |
        (EPT_READ | EPT_WRITE | EPT_EXECUTE);

    /* Identity map first 2MB (512 * 4KB pages) */
    for (UINT32 i = 0; i < 512; i++) {
        pContext->EptPt[i] = (i * 4096) |
            (EPT_READ | EPT_WRITE | EPT_EXECUTE | EPT_MEMORY_TYPE_WB);
    }

    return S_OK;
}

HRESULT
HvVtxEptMapPage(
    HV_VTX_CONTEXT* pContext,
    UINT64 GuestPhys,
    UINT64 HostPhys,
    UINT32 Flags
)
{
    UINT64 pml4Idx, pdptIdx, pdIdx, ptIdx;
    UINT64 eptFlags = 0;

    if (pContext == NULL || pContext->EptPt == NULL) {
        return E_POINTER;
    }

    /* Calculate indices */
    pml4Idx = (GuestPhys >> 39) & 0x1FF;
    pdptIdx = (GuestPhys >> 30) & 0x1FF;
    pdIdx = (GuestPhys >> 21) & 0x1FF;
    ptIdx = (GuestPhys >> 12) & 0x1FF;

    /* Convert flags */
    if (Flags & HV_MEMORY_READ) eptFlags |= EPT_READ;
    if (Flags & HV_MEMORY_WRITE) eptFlags |= EPT_WRITE;
    if (Flags & HV_MEMORY_EXEC) eptFlags |= EPT_EXECUTE;
    eptFlags |= EPT_MEMORY_TYPE_WB;

    /* For simplicity, only handle first page table */
    if (pml4Idx == 0 && pdptIdx == 0 && pdIdx == 0 && ptIdx < 512) {
        pContext->EptPt[ptIdx] = HostPhys | eptFlags;
        return S_OK;
    }

    return HV_UNSUPPORTED;
}

/* ======================================================================= */
/* VMCS setup                                                              */
/* ======================================================================= */

HRESULT
HvVtxSetupVmcs(
    HV_VTX_CONTEXT* pContext,
    HvVirtualCpu* pCpu
)
{
    UINT32 pinCtls, procCtls, proc2Ctls, exitCtls, entryCtls;
    HV_X86_CONTEXT* pX86Ctx;

    if (pContext == NULL || pCpu == NULL) {
        return E_POINTER;
    }

    pX86Ctx = (HV_X86_CONTEXT*)pCpu->HwContext;

    /* Get VMX capabilities */
    HvVtxGetCapabilities(&pinCtls, &procCtls, &proc2Ctls, &exitCtls, &entryCtls);

    /* Clear VMCS */
    VmcsClear(pContext->Vmcs);

    /* Load VMCS pointer */
    VmptrldVMCS(pContext->Vmcs);

    /* Configure pin-based controls */
    VmcsWrite(pContext->Vmcs, VMCS_CTRL_PIN_BASED, pinCtls);

    /* Configure processor-based controls */
    procCtls |= CPU_BASED_HLT_EXITING;
    procCtls |= CPU_BASED_USE_MSR_BITMAPS;
    procCtls |= CPU_BASED_ACTIVATE_SECONDARY_CONTROLS;
    VmcsWrite(pContext->Vmcs, VMCS_CTRL_PROC_BASED, procCtls);

    /* Configure secondary controls */
    proc2Ctls |= SECONDARY_EXEC_ENABLE_EPT;
    proc2Ctls |= SECONDARY_EXEC_UNRESTRICTED_GUEST;
    VmcsWrite(pContext->Vmcs, VMCS_CTRL_PROC_BASED2, proc2Ctls);

    /* Configure exit controls */
    VmcsWrite(pContext->Vmcs, VMCS_CTRL_EXIT, exitCtls);

    /* Configure entry controls */
    VmcsWrite(pContext->Vmcs, VMCS_CTRL_ENTRY, entryCtls);

    /* Setup EPT pointer */
    UINT64 eptPtr = (UINT64)(UINTN)pContext->EptPml4;
    eptPtr |= (3 << 3);  /* Page-walk length = 4 */
    eptPtr |= (6 << 0);  /* Memory type = WB */
    VmcsWrite(pContext->Vmcs, VMCS_EPT_POINTER, eptPtr);

    /* Setup guest state */
    VmcsWrite(pContext->Vmcs, VMCS_GUEST_CR0, pX86Ctx->cr0);
    VmcsWrite(pContext->Vmcs, VMCS_GUEST_CR3, pX86Ctx->cr3);
    VmcsWrite(pContext->Vmcs, VMCS_GUEST_CR4, pX86Ctx->cr4);
    VmcsWrite(pContext->Vmcs, VMCS_GUEST_RIP, pX86Ctx->rip);
    VmcsWrite(pContext->Vmcs, VMCS_GUEST_RSP, pX86Ctx->rsp);
    VmcsWrite(pContext->Vmcs, VMCS_GUEST_RFLAGS, pX86Ctx->rflags);

    /* Setup segment selectors */
    VmcsWrite(pContext->Vmcs, VMCS_GUEST_CS_SELECTOR, pX86Ctx->cs);
    VmcsWrite(pContext->Vmcs, VMCS_GUEST_DS_SELECTOR, pX86Ctx->ds);
    VmcsWrite(pContext->Vmcs, VMCS_GUEST_ES_SELECTOR, pX86Ctx->es);
    VmcsWrite(pContext->Vmcs, VMCS_GUEST_FS_SELECTOR, pX86Ctx->fs);
    VmcsWrite(pContext->Vmcs, VMCS_GUEST_GS_SELECTOR, pX86Ctx->gs);
    VmcsWrite(pContext->Vmcs, VMCS_GUEST_SS_SELECTOR, pX86Ctx->ss);

    return S_OK;
}

/* ======================================================================= */
/* VM entry/exit operations                                                */
/* ======================================================================= */

HRESULT
HvVtxVmLaunch(
    HV_VTX_CONTEXT* pContext
)
{
    /* In real implementation, execute VMLAUNCH instruction */
    (VOID)pContext;
    return HV_UNSUPPORTED;
}

HRESULT
HvVtxVmResume(
    HV_VTX_CONTEXT* pContext
)
{
    /* In real implementation, execute VMRESUME instruction */
    (VOID)pContext;
    return HV_UNSUPPORTED;
}

HRESULT
HvVtxHandleVmExit(
    HV_VTX_CONTEXT* pContext,
    HvVirtualCpu* pCpu,
    HV_VM_EXIT_INFO* pExitInfo
)
{
    UINT64 exitReason, exitQual, instrLen;

    if (pContext == NULL || pExitInfo == NULL) {
        return E_POINTER;
    }

    /* Read exit information from VMCS */
    VmcsRead(pContext->Vmcs, VMCS_EXIT_REASON, &exitReason);
    VmcsRead(pContext->Vmcs, VMCS_EXIT_QUALIFICATION, &exitQual);
    VmcsRead(pContext->Vmcs, VMCS_EXIT_INSTRUCTION_LENGTH, &instrLen);

    /* Fill exit info structure */
    pExitInfo->CpuId = pCpu->CpuId;
    pExitInfo->ExitQualification = exitQual;
    pExitInfo->InstructionLength = instrLen;

    /* Map VT-x exit reason to generic HV exit reason */
    switch (exitReason & 0xFFFF) {
        case EXIT_REASON_EXCEPTION_NMI:
            pExitInfo->Reason = HV_EXIT_EXCEPTION_NMI;
            break;
        case EXIT_REASON_EXTERNAL_INTERRUPT:
            pExitInfo->Reason = HV_EXIT_EXTERNAL_INTERRUPT;
            break;
        case EXIT_REASON_TRIPLE_FAULT:
            pExitInfo->Reason = HV_EXIT_TRIPLE_FAULT;
            break;
        case EXIT_REASON_INIT:
            pExitInfo->Reason = HV_EXIT_INIT;
            break;
        case EXIT_REASON_IO_INSTRUCTION:
            pExitInfo->Reason = HV_EXIT_IO_INSTRUCTION;
            break;
        case EXIT_REASON_MSR_READ:
            pExitInfo->Reason = HV_EXIT_RDMSR;
            break;
        case EXIT_REASON_MSR_WRITE:
            pExitInfo->Reason = HV_EXIT_WRMSR;
            break;
        case EXIT_REASON_EPT_VIOLATION:
            pExitInfo->Reason = HV_EXIT_EPT_VIOLATION;
            break;
        case EXIT_REASON_EPT_MISCONFIGURATION:
            pExitInfo->Reason = HV_EXIT_EPT_MISCONFIGURATION;
            break;
        default:
            pExitInfo->Reason = (HV_VM_EXIT_REASON)exitReason;
            break;
    }

    return S_OK;
}

/* ======================================================================= */
/* VT-x context lifecycle                                                  */
/* ======================================================================= */

HRESULT
HvVtxCreateContext(
    HvVirtualCpu* pCpu,
    HV_VTX_CONTEXT** ppContext
)
{
    HV_VTX_CONTEXT* pContext;
    HRESULT hr;

    if (ppContext == NULL) {
        return E_POINTER;
    }

    /* Check if VT-x is supported */
    if (!HvVtxIsSupported()) {
        return HV_UNSUPPORTED;
    }

    pContext = (HV_VTX_CONTEXT*)RtlAllocateMemory(NULL, sizeof(HV_VTX_CONTEXT));
    if (pContext == NULL) {
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pContext, sizeof(HV_VTX_CONTEXT));

    /* Allocate VMCS (must be 4KB aligned) */
    pContext->Vmcs = (HV_VMCS*)RtlAllocateMemory(NULL, sizeof(HV_VMCS));
    if (pContext->Vmcs == NULL) {
        RtlFreeMemory(NULL, pContext);
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pContext->Vmcs, sizeof(HV_VMCS));

    /* Initialize EPT */
    hr = HvVtxEptInitialize(pContext);
    if (FAILED(hr)) {
        RtlFreeMemory(NULL, pContext->Vmcs);
        RtlFreeMemory(NULL, pContext);
        return hr;
    }

    /* Allocate MSR bitmap */
    pContext->MsrBitmap = (UINT8*)RtlAllocateMemory(NULL, 4096);
    if (pContext->MsrBitmap != NULL) {
        RtlZeroMemory(pContext->MsrBitmap, 4096);
    }

    /* Allocate I/O bitmaps */
    pContext->IoBitmapA = (UINT8*)RtlAllocateMemory(NULL, 4096);
    pContext->IoBitmapB = (UINT8*)RtlAllocateMemory(NULL, 4096);
    if (pContext->IoBitmapA != NULL && pContext->IoBitmapB != NULL) {
        RtlZeroMemory(pContext->IoBitmapA, 4096);
        RtlZeroMemory(pContext->IoBitmapB, 4096);
    }

    /* Setup VMCS */
    hr = HvVtxSetupVmcs(pContext, pCpu);
    if (FAILED(hr)) {
        if (pContext->MsrBitmap) RtlFreeMemory(NULL, pContext->MsrBitmap);
        if (pContext->IoBitmapA) RtlFreeMemory(NULL, pContext->IoBitmapA);
        if (pContext->IoBitmapB) RtlFreeMemory(NULL, pContext->IoBitmapB);
        if (pContext->EptPml4) RtlFreeMemory(NULL, pContext->EptPml4);
        if (pContext->EptPdpt) RtlFreeMemory(NULL, pContext->EptPdpt);
        if (pContext->EptPd) RtlFreeMemory(NULL, pContext->EptPd);
        if (pContext->EptPt) RtlFreeMemory(NULL, pContext->EptPt);
        RtlFreeMemory(NULL, pContext->Vmcs);
        RtlFreeMemory(NULL, pContext);
        return hr;
    }

    *ppContext = pContext;
    return S_OK;
}

HRESULT
HvVtxDestroyContext(
    HV_VTX_CONTEXT* pContext
)
{
    if (pContext == NULL) {
        return S_OK;
    }

    if (pContext->Vmcs) {
        VmcsClear(pContext->Vmcs);
        RtlFreeMemory(NULL, pContext->Vmcs);
    }

    if (pContext->EptPml4) RtlFreeMemory(NULL, pContext->EptPml4);
    if (pContext->EptPdpt) RtlFreeMemory(NULL, pContext->EptPdpt);
    if (pContext->EptPd) RtlFreeMemory(NULL, pContext->EptPd);
    if (pContext->EptPt) RtlFreeMemory(NULL, pContext->EptPt);

    if (pContext->MsrBitmap) RtlFreeMemory(NULL, pContext->MsrBitmap);
    if (pContext->IoBitmapA) RtlFreeMemory(NULL, pContext->IoBitmapA);
    if (pContext->IoBitmapB) RtlFreeMemory(NULL, pContext->IoBitmapB);

    RtlFreeMemory(NULL, pContext);
    return S_OK;
}
