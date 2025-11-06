/*++
    Module Name:

        backends_stub.c

    Abstract:

        Stub implementations for all architecture backends.
        These provide the framework for full implementations.

--*/

#include "../hypervisor_impl.h"

/* ======================================================================= */
/* Generic backend operations (used by most stub backends)                */
/* ======================================================================= */

static HRESULT
HvGeneric_Initialize(HvVirtualCpu* pCpu)
{
    /* Allocate context */
    VOID* pContext = RtlAllocateMemory(&pCpu->VM->Pool, 1024);
    if (pContext == NULL) {
        return HV_NO_RESOURCES;
    }
    RtlZeroMemory(pContext, 1024);
    pCpu->HwContext = pContext;
    return S_OK;
}

static HRESULT
HvGeneric_Shutdown(HvVirtualCpu* pCpu)
{
    if (pCpu->HwContext != NULL) {
        RtlFreeMemory(&pCpu->VM->Pool, pCpu->HwContext);
        pCpu->HwContext = NULL;
    }
    return S_OK;
}

static HRESULT
HvGeneric_Run(HvVirtualCpu* pCpu, HV_VM_EXIT_INFO* pExitInfo)
{
    if (pExitInfo == NULL) return E_POINTER;
    RtlZeroMemory(pExitInfo, sizeof(HV_VM_EXIT_INFO));
    pExitInfo->Reason = HV_EXIT_HLT;
    pExitInfo->CpuId = pCpu->CpuId;
    return S_OK;
}

static HRESULT
HvGeneric_ReadRegister(HvVirtualCpu* pCpu, UINT32 RegId, HV_REGISTER_VALUE* pValue)
{
    if (pValue == NULL) return E_POINTER;
    RtlZeroMemory(pValue, sizeof(HV_REGISTER_VALUE));
    return S_OK;
}

static HRESULT
HvGeneric_WriteRegister(HvVirtualCpu* pCpu, UINT32 RegId, CONST HV_REGISTER_VALUE* pValue)
{
    (VOID)pCpu; (VOID)RegId; (VOID)pValue;
    return S_OK;
}

static HRESULT
HvGeneric_HandleExit(HvVirtualCpu* pCpu, HV_VM_EXIT_INFO* pExitInfo)
{
    (VOID)pCpu; (VOID)pExitInfo;
    return S_OK;
}

static HRESULT
HvGeneric_TranslateInstruction(HvVirtualCpu* pCpu, UINT64 GuestAddr, VOID* HostAddr, UINT32* pSize)
{
    (VOID)pCpu; (VOID)GuestAddr; (VOID)HostAddr;
    if (pSize != NULL) *pSize = 0;
    return HV_UNSUPPORTED;
}

static HV_CPU_BACKEND_OPS g_GenericBackendOps = {
    HvGeneric_Initialize,
    HvGeneric_Shutdown,
    HvGeneric_Run,
    HvGeneric_ReadRegister,
    HvGeneric_WriteRegister,
    HvGeneric_HandleExit,
    HvGeneric_TranslateInstruction
};

/* ======================================================================= */
/* RISC-V backend                                                          */
/* ======================================================================= */

HRESULT
HvBackend_RISCV_Create(
    HV_VIRT_MODE Mode,
    HV_ARCHITECTURE Arch,
    HV_CPU_BACKEND** ppBackend
)
{
    HV_CPU_BACKEND* pBackend;

    pBackend = (HV_CPU_BACKEND*)RtlAllocateMemory(NULL, sizeof(HV_CPU_BACKEND));
    if (pBackend == NULL) {
        return HV_NO_RESOURCES;
    }

    pBackend->Architecture = Arch;
    pBackend->Mode = Mode;
    pBackend->Ops = &g_GenericBackendOps;
    pBackend->PrivateData = NULL;

    *ppBackend = pBackend;
    return S_OK;
}

/* ======================================================================= */
/* MIPS backend                                                            */
/* ======================================================================= */

HRESULT
HvBackend_MIPS_Create(
    HV_VIRT_MODE Mode,
    HV_ARCHITECTURE Arch,
    HV_CPU_BACKEND** ppBackend
)
{
    HV_CPU_BACKEND* pBackend;

    pBackend = (HV_CPU_BACKEND*)RtlAllocateMemory(NULL, sizeof(HV_CPU_BACKEND));
    if (pBackend == NULL) {
        return HV_NO_RESOURCES;
    }

    pBackend->Architecture = Arch;
    pBackend->Mode = Mode;
    pBackend->Ops = &g_GenericBackendOps;
    pBackend->PrivateData = NULL;

    *ppBackend = pBackend;
    return S_OK;
}

/* ======================================================================= */
/* SPARC backend                                                           */
/* ======================================================================= */

HRESULT
HvBackend_SPARC_Create(
    HV_VIRT_MODE Mode,
    HV_ARCHITECTURE Arch,
    HV_CPU_BACKEND** ppBackend
)
{
    HV_CPU_BACKEND* pBackend;

    pBackend = (HV_CPU_BACKEND*)RtlAllocateMemory(NULL, sizeof(HV_CPU_BACKEND));
    if (pBackend == NULL) {
        return HV_NO_RESOURCES;
    }

    pBackend->Architecture = Arch;
    pBackend->Mode = Mode;
    pBackend->Ops = &g_GenericBackendOps;
    pBackend->PrivateData = NULL;

    *ppBackend = pBackend;
    return S_OK;
}

/* ======================================================================= */
/* M68K backend                                                            */
/* ======================================================================= */

HRESULT
HvBackend_M68K_Create(
    HV_VIRT_MODE Mode,
    HV_CPU_BACKEND** ppBackend
)
{
    HV_CPU_BACKEND* pBackend;

    pBackend = (HV_CPU_BACKEND*)RtlAllocateMemory(NULL, sizeof(HV_CPU_BACKEND));
    if (pBackend == NULL) {
        return HV_NO_RESOURCES;
    }

    pBackend->Architecture = HV_ARCH_M68K;
    pBackend->Mode = Mode;
    pBackend->Ops = &g_GenericBackendOps;
    pBackend->PrivateData = NULL;

    *ppBackend = pBackend;
    return S_OK;
}

/* ======================================================================= */
/* VAX backend                                                             */
/* ======================================================================= */

HRESULT
HvBackend_VAX_Create(
    HV_VIRT_MODE Mode,
    HV_CPU_BACKEND** ppBackend
)
{
    HV_CPU_BACKEND* pBackend;

    pBackend = (HV_CPU_BACKEND*)RtlAllocateMemory(NULL, sizeof(HV_CPU_BACKEND));
    if (pBackend == NULL) {
        return HV_NO_RESOURCES;
    }

    pBackend->Architecture = HV_ARCH_VAX;
    pBackend->Mode = Mode;
    pBackend->Ops = &g_GenericBackendOps;
    pBackend->PrivateData = NULL;

    *ppBackend = pBackend;
    return S_OK;
}

/* ======================================================================= */
/* Alpha backend                                                           */
/* ======================================================================= */

HRESULT
HvBackend_Alpha_Create(
    HV_VIRT_MODE Mode,
    HV_CPU_BACKEND** ppBackend
)
{
    HV_CPU_BACKEND* pBackend;

    pBackend = (HV_CPU_BACKEND*)RtlAllocateMemory(NULL, sizeof(HV_CPU_BACKEND));
    if (pBackend == NULL) {
        return HV_NO_RESOURCES;
    }

    pBackend->Architecture = HV_ARCH_ALPHA;
    pBackend->Mode = Mode;
    pBackend->Ops = &g_GenericBackendOps;
    pBackend->PrivateData = NULL;

    *ppBackend = pBackend;
    return S_OK;
}

/* ======================================================================= */
/* IA-64 backend                                                           */
/* ======================================================================= */

HRESULT
HvBackend_IA64_Create(
    HV_VIRT_MODE Mode,
    HV_CPU_BACKEND** ppBackend
)
{
    HV_CPU_BACKEND* pBackend;

    pBackend = (HV_CPU_BACKEND*)RtlAllocateMemory(NULL, sizeof(HV_CPU_BACKEND));
    if (pBackend == NULL) {
        return HV_NO_RESOURCES;
    }

    pBackend->Architecture = HV_ARCH_IA64;
    pBackend->Mode = Mode;
    pBackend->Ops = &g_GenericBackendOps;
    pBackend->PrivateData = NULL;

    *ppBackend = pBackend;
    return S_OK;
}

/* ======================================================================= */
/* PowerPC backend                                                         */
/* ======================================================================= */

HRESULT
HvBackend_PPC_Create(
    HV_VIRT_MODE Mode,
    HV_ARCHITECTURE Arch,
    HV_CPU_BACKEND** ppBackend
)
{
    HV_CPU_BACKEND* pBackend;

    pBackend = (HV_CPU_BACKEND*)RtlAllocateMemory(NULL, sizeof(HV_CPU_BACKEND));
    if (pBackend == NULL) {
        return HV_NO_RESOURCES;
    }

    pBackend->Architecture = Arch;
    pBackend->Mode = Mode;
    pBackend->Ops = &g_GenericBackendOps;
    pBackend->PrivateData = NULL;

    *ppBackend = pBackend;
    return S_OK;
}

/* ======================================================================= */
/* LoongArch backend                                                       */
/* ======================================================================= */

HRESULT
HvBackend_LoongArch_Create(
    HV_VIRT_MODE Mode,
    HV_ARCHITECTURE Arch,
    HV_CPU_BACKEND** ppBackend
)
{
    HV_CPU_BACKEND* pBackend;

    pBackend = (HV_CPU_BACKEND*)RtlAllocateMemory(NULL, sizeof(HV_CPU_BACKEND));
    if (pBackend == NULL) {
        return HV_NO_RESOURCES;
    }

    pBackend->Architecture = Arch;
    pBackend->Mode = Mode;
    pBackend->Ops = &g_GenericBackendOps;
    pBackend->PrivateData = NULL;

    *ppBackend = pBackend;
    return S_OK;
}

/* ======================================================================= */
/* DLX backend                                                             */
/* ======================================================================= */

HRESULT
HvBackend_DLX_Create(
    HV_VIRT_MODE Mode,
    HV_CPU_BACKEND** ppBackend
)
{
    HV_CPU_BACKEND* pBackend;

    pBackend = (HV_CPU_BACKEND*)RtlAllocateMemory(NULL, sizeof(HV_CPU_BACKEND));
    if (pBackend == NULL) {
        return HV_NO_RESOURCES;
    }

    pBackend->Architecture = HV_ARCH_DLX;
    pBackend->Mode = Mode;
    pBackend->Ops = &g_GenericBackendOps;
    pBackend->PrivateData = NULL;

    *ppBackend = pBackend;
    return S_OK;
}

/* ======================================================================= */
/* MMIX backend                                                            */
/* ======================================================================= */

HRESULT
HvBackend_MMIX_Create(
    HV_VIRT_MODE Mode,
    HV_CPU_BACKEND** ppBackend
)
{
    HV_CPU_BACKEND* pBackend;

    pBackend = (HV_CPU_BACKEND*)RtlAllocateMemory(NULL, sizeof(HV_CPU_BACKEND));
    if (pBackend == NULL) {
        return HV_NO_RESOURCES;
    }

    pBackend->Architecture = HV_ARCH_MMIX;
    pBackend->Mode = Mode;
    pBackend->Ops = &g_GenericBackendOps;
    pBackend->PrivateData = NULL;

    *ppBackend = pBackend;
    return S_OK;
}
