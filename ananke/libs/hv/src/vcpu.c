/*++
    Module Name:

        vcpu.c

    Abstract:

        Virtual CPU implementation.

--*/

#include "hypervisor_impl.h"
#include <ananke/atomics.h>

/* ======================================================================= */
/* IVirtualCpu implementation                                              */
/* ======================================================================= */

static HRESULT STDMETHODCALLTYPE
HvVCpu_QueryInterface(
    IVirtualCpu* This,
    REFIID riid,
    VOID** ppvObject
)
{
    HvVirtualCpu* pCpu = (HvVirtualCpu*)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (RtlIsEqualGUID(riid, &IID_IUnknown) ||
        RtlIsEqualGUID(riid, &IID_IVirtualCpu)) {
        *ppvObject = &pCpu->Interface;
        AnxInterlockedIncrement((volatile INT32*)&pCpu->RefCount);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
HvVCpu_AddRef(
    IVirtualCpu* This
)
{
    HvVirtualCpu* pCpu = (HvVirtualCpu*)This;
    return (UINT32)AnxInterlockedIncrement((volatile INT32*)&pCpu->RefCount);
}

static UINT32 STDMETHODCALLTYPE
HvVCpu_Release(
    IVirtualCpu* This
)
{
    HvVirtualCpu* pCpu = (HvVirtualCpu*)This;
    UINT32 refCount = (UINT32)AnxInterlockedDecrement((volatile INT32*)&pCpu->RefCount);

    if (refCount == 0) {
        /* Cleanup backend */
        if (pCpu->Backend != NULL && pCpu->Backend->Ops->Shutdown != NULL) {
            pCpu->Backend->Ops->Shutdown(pCpu);
        }

        /* Cleanup translation cache */
        if (pCpu->TransCache != NULL) {
            HvTC_Shutdown(pCpu);
        }

        /* Free CPU */
        RtlFreeMemory(&pCpu->VM->Pool, pCpu);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
HvVCpu_GetId(
    IVirtualCpu* This,
    UINT32* pCpuId
)
{
    HvVirtualCpu* pCpu = (HvVirtualCpu*)This;

    if (pCpuId == NULL) {
        return E_POINTER;
    }

    *pCpuId = pCpu->CpuId;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVCpu_Run(
    IVirtualCpu* This,
    HV_VM_EXIT_INFO* pExitInfo
)
{
    HvVirtualCpu* pCpu = (HvVirtualCpu*)This;

    if (pExitInfo == NULL) {
        return E_POINTER;
    }

    if (pCpu->VM->State != HV_VM_STATE_RUNNING) {
        return HV_ERROR;
    }

    if (pCpu->Backend == NULL || pCpu->Backend->Ops->Run == NULL) {
        return HV_UNSUPPORTED;
    }

    pCpu->State = HV_VM_STATE_RUNNING;
    return pCpu->Backend->Ops->Run(pCpu, pExitInfo);
}

static HRESULT STDMETHODCALLTYPE
HvVCpu_Interrupt(
    IVirtualCpu* This,
    UINT32 Vector
)
{
    HvVirtualCpu* pCpu = (HvVirtualCpu*)This;

    /* Interrupt handling is architecture-specific */
    /* For now, we'll set a flag that the backend can check */
    (VOID)pCpu;
    (VOID)Vector;

    /* TODO: Implement interrupt injection via backend */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVCpu_ReadRegister(
    IVirtualCpu* This,
    UINT32 RegisterId,
    HV_REGISTER_VALUE* pValue
)
{
    HvVirtualCpu* pCpu = (HvVirtualCpu*)This;

    if (pValue == NULL) {
        return E_POINTER;
    }

    if (pCpu->Backend == NULL || pCpu->Backend->Ops->ReadRegister == NULL) {
        /* Fallback to internal register array */
        if (RegisterId >= 128) {
            return E_INVALIDARG;
        }
        RtlCopyMemory(pValue, &pCpu->Registers[RegisterId], sizeof(HV_REGISTER_VALUE));
        return S_OK;
    }

    return pCpu->Backend->Ops->ReadRegister(pCpu, RegisterId, pValue);
}

static HRESULT STDMETHODCALLTYPE
HvVCpu_WriteRegister(
    IVirtualCpu* This,
    UINT32 RegisterId,
    CONST HV_REGISTER_VALUE* pValue
)
{
    HvVirtualCpu* pCpu = (HvVirtualCpu*)This;

    if (pValue == NULL) {
        return E_POINTER;
    }

    if (pCpu->Backend == NULL || pCpu->Backend->Ops->WriteRegister == NULL) {
        /* Fallback to internal register array */
        if (RegisterId >= 128) {
            return E_INVALIDARG;
        }
        RtlCopyMemory(&pCpu->Registers[RegisterId], pValue, sizeof(HV_REGISTER_VALUE));
        return S_OK;
    }

    return pCpu->Backend->Ops->WriteRegister(pCpu, RegisterId, pValue);
}

static HRESULT STDMETHODCALLTYPE
HvVCpu_ReadRegisters(
    IVirtualCpu* This,
    UINT32 Count,
    HV_REGISTER* pRegisters
)
{
    UINT32 i;
    HRESULT hr;

    if (pRegisters == NULL) {
        return E_POINTER;
    }

    for (i = 0; i < Count; i++) {
        hr = HvVCpu_ReadRegister(This, pRegisters[i].Id, &pRegisters[i].Value);
        if (FAILED(hr)) {
            return hr;
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVCpu_WriteRegisters(
    IVirtualCpu* This,
    UINT32 Count,
    CONST HV_REGISTER* pRegisters
)
{
    UINT32 i;
    HRESULT hr;

    if (pRegisters == NULL) {
        return E_POINTER;
    }

    for (i = 0; i < Count; i++) {
        hr = HvVCpu_WriteRegister(This, pRegisters[i].Id, &pRegisters[i].Value);
        if (FAILED(hr)) {
            return hr;
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVCpu_GetInstructionPointer(
    IVirtualCpu* This,
    UINT64* pIP
)
{
    HvVirtualCpu* pCpu = (HvVirtualCpu*)This;

    if (pIP == NULL) {
        return E_POINTER;
    }

    *pIP = pCpu->InstructionPointer;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVCpu_SetInstructionPointer(
    IVirtualCpu* This,
    UINT64 IP
)
{
    HvVirtualCpu* pCpu = (HvVirtualCpu*)This;

    pCpu->InstructionPointer = IP;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVCpu_SingleStep(
    IVirtualCpu* This,
    HV_VM_EXIT_INFO* pExitInfo
)
{
    HvVirtualCpu* pCpu = (HvVirtualCpu*)This;

    if (pExitInfo == NULL) {
        return E_POINTER;
    }

    /* TODO: Implement single-step via backend */
    (VOID)pCpu;
    return HV_UNSUPPORTED;
}

/* VTable for IVirtualCpu */
static IVirtualCpuVtbl g_VirtualCpuVtbl = {
    HvVCpu_QueryInterface,
    HvVCpu_AddRef,
    HvVCpu_Release,
    HvVCpu_GetId,
    HvVCpu_Run,
    HvVCpu_Interrupt,
    HvVCpu_ReadRegister,
    HvVCpu_WriteRegister,
    HvVCpu_ReadRegisters,
    HvVCpu_WriteRegisters,
    HvVCpu_GetInstructionPointer,
    HvVCpu_SetInstructionPointer,
    HvVCpu_SingleStep
};

/* ======================================================================= */
/* Virtual CPU creation                                                    */
/* ======================================================================= */

HRESULT
HvVirtualCpu_Create(
    HvVirtualMachine* pVM,
    UINT32 CpuId,
    HvVirtualCpu** ppCpu
)
{
    HvVirtualCpu* pCpu;
    HRESULT hr;

    if (ppCpu == NULL) {
        return E_POINTER;
    }

    /* Allocate CPU */
    pCpu = (HvVirtualCpu*)RtlAllocateMemory(&pVM->Pool, sizeof(HvVirtualCpu));
    if (pCpu == NULL) {
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pCpu, sizeof(HvVirtualCpu));

    /* Initialize */
    pCpu->Interface.lpVtbl = &g_VirtualCpuVtbl;
    pCpu->RefCount = 1;
    pCpu->VM = pVM;
    pCpu->CpuId = CpuId;
    pCpu->State = HV_VM_STATE_STOPPED;
    pCpu->Backend = pVM->Backend;

    /* Initialize backend */
    if (pCpu->Backend != NULL && pCpu->Backend->Ops->Initialize != NULL) {
        hr = pCpu->Backend->Ops->Initialize(pCpu);
        if (FAILED(hr)) {
            RtlFreeMemory(&pVM->Pool, pCpu);
            return hr;
        }
    }

    /* Initialize translation cache if binary translation is enabled */
    if (pVM->Config.EnableBinaryTranslation) {
        hr = HvTC_Initialize(pCpu);
        if (FAILED(hr)) {
            if (pCpu->Backend != NULL && pCpu->Backend->Ops->Shutdown != NULL) {
                pCpu->Backend->Ops->Shutdown(pCpu);
            }
            RtlFreeMemory(&pVM->Pool, pCpu);
            return hr;
        }
    }

    *ppCpu = pCpu;
    return S_OK;
}
