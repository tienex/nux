/*++
    Module Name:

        vm.c

    Abstract:

        Virtual machine implementation.

--*/

#include "hypervisor_impl.h"
#include <ananke/atomics.h>

/* ======================================================================= */
/* IVirtualMachine implementation                                          */
/* ======================================================================= */

static HRESULT STDMETHODCALLTYPE
HvVM_QueryInterface(
    IVirtualMachine* This,
    REFIID riid,
    VOID** ppvObject
)
{
    HvVirtualMachine* pVM = (HvVirtualMachine*)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (RtlIsEqualGUID(riid, &IID_IUnknown) ||
        RtlIsEqualGUID(riid, &IID_IVirtualMachine)) {
        *ppvObject = &pVM->Interface;
        AnxInterlockedIncrement((volatile INT32*)&pVM->RefCount);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
HvVM_AddRef(
    IVirtualMachine* This
)
{
    HvVirtualMachine* pVM = (HvVirtualMachine*)This;
    return (UINT32)AnxInterlockedIncrement((volatile INT32*)&pVM->RefCount);
}

static UINT32 STDMETHODCALLTYPE
HvVM_Release(
    IVirtualMachine* This
)
{
    HvVirtualMachine* pVM = (HvVirtualMachine*)This;
    UINT32 refCount = (UINT32)AnxInterlockedDecrement((volatile INT32*)&pVM->RefCount);
    UINT32 i;

    if (refCount == 0) {
        /* Stop VM if running */
        if (pVM->State == HV_VM_STATE_RUNNING) {
            pVM->Interface.lpVtbl->Stop(This);
        }

        /* Release virtual CPUs */
        if (pVM->VCpus != NULL) {
            for (i = 0; i < pVM->NumCpus; i++) {
                if (pVM->VCpus[i] != NULL) {
                    IVirtualCpu_Release((IVirtualCpu*)pVM->VCpus[i]);
                }
            }
            RtlFreeMemory(&pVM->Pool, pVM->VCpus);
        }

        /* Release virtual memory */
        if (pVM->Memory != NULL) {
            IVirtualMemory_Release((IVirtualMemory*)pVM->Memory);
        }

        /* Release devices */
        if (pVM->Devices != NULL) {
            for (i = 0; i < pVM->DeviceCount; i++) {
                if (pVM->Devices[i] != NULL) {
                    IVirtualDevice_Release(pVM->Devices[i]);
                }
            }
            RtlFreeMemory(&pVM->Pool, pVM->Devices);
        }

        /* Destroy memory pool */
        RtlDestroyMemoryPool(&pVM->Pool);

        /* Release hypervisor reference */
        IHypervisor_Release((IHypervisor*)pVM->Hypervisor);

        /* Free VM */
        RtlFreeMemory(&pVM->Hypervisor->Pool, pVM);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
HvVM_GetConfig(
    IVirtualMachine* This,
    HV_VM_CONFIG* pConfig
)
{
    HvVirtualMachine* pVM = (HvVirtualMachine*)This;

    if (pConfig == NULL) {
        return E_POINTER;
    }

    RtlCopyMemory(pConfig, &pVM->Config, sizeof(HV_VM_CONFIG));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVM_GetState(
    IVirtualMachine* This,
    HV_VM_STATE* pState
)
{
    HvVirtualMachine* pVM = (HvVirtualMachine*)This;

    if (pState == NULL) {
        return E_POINTER;
    }

    *pState = pVM->State;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVM_Start(
    IVirtualMachine* This
)
{
    HvVirtualMachine* pVM = (HvVirtualMachine*)This;

    if (pVM->State == HV_VM_STATE_RUNNING) {
        return S_OK;
    }

    if (pVM->State == HV_VM_STATE_ERROR) {
        return HV_ERROR;
    }

    pVM->State = HV_VM_STATE_RUNNING;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVM_Stop(
    IVirtualMachine* This
)
{
    HvVirtualMachine* pVM = (HvVirtualMachine*)This;

    if (pVM->State == HV_VM_STATE_STOPPED) {
        return S_OK;
    }

    pVM->State = HV_VM_STATE_STOPPED;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVM_Pause(
    IVirtualMachine* This
)
{
    HvVirtualMachine* pVM = (HvVirtualMachine*)This;

    if (pVM->State != HV_VM_STATE_RUNNING) {
        return HV_ERROR;
    }

    pVM->State = HV_VM_STATE_PAUSED;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVM_Resume(
    IVirtualMachine* This
)
{
    HvVirtualMachine* pVM = (HvVirtualMachine*)This;

    if (pVM->State != HV_VM_STATE_PAUSED) {
        return HV_ERROR;
    }

    pVM->State = HV_VM_STATE_RUNNING;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVM_Reset(
    IVirtualMachine* This
)
{
    HvVirtualMachine* pVM = (HvVirtualMachine*)This;
    UINT32 i;

    /* Reset all CPUs */
    for (i = 0; i < pVM->NumCpus; i++) {
        if (pVM->VCpus[i] != NULL) {
            /* Reset CPU state - set IP to 0 and clear registers */
            pVM->VCpus[i]->InstructionPointer = 0;
            RtlZeroMemory(pVM->VCpus[i]->Registers, sizeof(pVM->VCpus[i]->Registers));
            pVM->VCpus[i]->State = HV_VM_STATE_STOPPED;
        }
    }

    /* Reset devices */
    for (i = 0; i < pVM->DeviceCount; i++) {
        if (pVM->Devices[i] != NULL) {
            IVirtualDevice_Reset(pVM->Devices[i]);
        }
    }

    pVM->State = HV_VM_STATE_STOPPED;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVM_GetVirtualCpu(
    IVirtualMachine* This,
    UINT32 CpuId,
    IVirtualCpu** ppCpu
)
{
    HvVirtualMachine* pVM = (HvVirtualMachine*)This;

    if (ppCpu == NULL) {
        return E_POINTER;
    }

    if (CpuId >= pVM->NumCpus) {
        return E_INVALIDARG;
    }

    *ppCpu = (IVirtualCpu*)pVM->VCpus[CpuId];
    IVirtualCpu_AddRef(*ppCpu);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVM_GetVirtualMemory(
    IVirtualMachine* This,
    IVirtualMemory** ppMemory
)
{
    HvVirtualMachine* pVM = (HvVirtualMachine*)This;

    if (ppMemory == NULL) {
        return E_POINTER;
    }

    *ppMemory = (IVirtualMemory*)pVM->Memory;
    IVirtualMemory_AddRef(*ppMemory);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVM_AttachDevice(
    IVirtualMachine* This,
    IVirtualDevice* pDevice
)
{
    HvVirtualMachine* pVM = (HvVirtualMachine*)This;
    IVirtualDevice** newDevices;
    UINT32 newCapacity;

    if (pDevice == NULL) {
        return E_POINTER;
    }

    /* Expand device array if needed */
    if (pVM->DeviceCount >= pVM->DeviceCapacity) {
        newCapacity = pVM->DeviceCapacity == 0 ? 4 : pVM->DeviceCapacity * 2;
        newDevices = (IVirtualDevice**)RtlAllocateMemory(&pVM->Pool, sizeof(IVirtualDevice*) * newCapacity);
        if (newDevices == NULL) {
            return HV_NO_RESOURCES;
        }

        if (pVM->Devices != NULL) {
            RtlCopyMemory(newDevices, pVM->Devices, sizeof(IVirtualDevice*) * pVM->DeviceCount);
            RtlFreeMemory(&pVM->Pool, pVM->Devices);
        }

        pVM->Devices = newDevices;
        pVM->DeviceCapacity = newCapacity;
    }

    /* Initialize device */
    HRESULT hr = IVirtualDevice_Initialize(pDevice, This);
    if (FAILED(hr)) {
        return hr;
    }

    /* Add device */
    pVM->Devices[pVM->DeviceCount++] = pDevice;
    IVirtualDevice_AddRef(pDevice);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVM_DetachDevice(
    IVirtualMachine* This,
    IVirtualDevice* pDevice
)
{
    HvVirtualMachine* pVM = (HvVirtualMachine*)This;
    UINT32 i, j;

    if (pDevice == NULL) {
        return E_POINTER;
    }

    /* Find and remove device */
    for (i = 0; i < pVM->DeviceCount; i++) {
        if (pVM->Devices[i] == pDevice) {
            /* Shutdown device */
            IVirtualDevice_Shutdown(pDevice);
            IVirtualDevice_Release(pDevice);

            /* Remove from array */
            for (j = i; j < pVM->DeviceCount - 1; j++) {
                pVM->Devices[j] = pVM->Devices[j + 1];
            }
            pVM->DeviceCount--;

            return S_OK;
        }
    }

    return HV_NO_DEVICE;
}

/* VTable for IVirtualMachine */
IVirtualMachineVtbl g_VirtualMachineVtbl = {
    HvVM_QueryInterface,
    HvVM_AddRef,
    HvVM_Release,
    HvVM_GetConfig,
    HvVM_GetState,
    HvVM_Start,
    HvVM_Stop,
    HvVM_Pause,
    HvVM_Resume,
    HvVM_Reset,
    HvVM_GetVirtualCpu,
    HvVM_GetVirtualMemory,
    HvVM_AttachDevice,
    HvVM_DetachDevice
};
