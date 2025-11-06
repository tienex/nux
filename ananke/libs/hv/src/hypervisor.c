/*++
    Module Name:

        hypervisor.c

    Abstract:

        Main hypervisor implementation.

--*/

#include "hypervisor_impl.h"
#include <ananke/ntrtl.h>
#include <ananke/atomics.h>

/* ======================================================================= */
/* IHypervisor implementation                                              */
/* ======================================================================= */

static HRESULT STDMETHODCALLTYPE
HvHypervisor_QueryInterface(
    IHypervisor* This,
    REFIID riid,
    VOID** ppvObject
)
{
    HvHypervisor* pHv = (HvHypervisor*)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (RtlIsEqualGUID(riid, &IID_IUnknown) ||
        RtlIsEqualGUID(riid, &IID_IHypervisor)) {
        *ppvObject = &pHv->Interface;
        AnxInterlockedIncrement((volatile INT32*)&pHv->RefCount);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
HvHypervisor_AddRef(
    IHypervisor* This
)
{
    HvHypervisor* pHv = (HvHypervisor*)This;
    return (UINT32)AnxInterlockedIncrement((volatile INT32*)&pHv->RefCount);
}

static UINT32 STDMETHODCALLTYPE
HvHypervisor_Release(
    IHypervisor* This
)
{
    HvHypervisor* pHv = (HvHypervisor*)This;
    UINT32 refCount = (UINT32)AnxInterlockedDecrement((volatile INT32*)&pHv->RefCount);

    if (refCount == 0) {
        /* Cleanup */
        if (pHv->Initialized) {
            pHv->Interface.lpVtbl->Shutdown((IHypervisor*)pHv);
        }

        /* Free backends */
        if (pHv->Backends != NULL) {
            RtlFreeMemory(&pHv->Pool, pHv->Backends);
        }

        /* Free VMs */
        if (pHv->VMs != NULL) {
            RtlFreeMemory(&pHv->Pool, pHv->VMs);
        }

        /* Destroy memory pool */
        RtlDestroyMemoryPool(&pHv->Pool);

        /* Free the hypervisor */
        RtlFreeMemory(NULL, pHv);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
HvHypervisor_Initialize(
    IHypervisor* This
)
{
    HvHypervisor* pHv = (HvHypervisor*)This;
    HRESULT hr;
    HV_CPU_BACKEND* pBackend;

    if (pHv->Initialized) {
        return S_OK;
    }

    /* Register all architecture backends */

    /* x86 family */
    hr = HvBackend_X86_Create(HV_VIRT_SOFTWARE, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    hr = HvBackend_X86_64_Create(HV_VIRT_SOFTWARE, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    /* Try hardware-assisted for x86_64 */
    hr = HvBackend_X86_64_Create(HV_VIRT_HARDWARE, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    /* RISC-V */
    hr = HvBackend_RISCV_Create(HV_VIRT_SOFTWARE, HV_ARCH_RISCV32, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    hr = HvBackend_RISCV_Create(HV_VIRT_SOFTWARE, HV_ARCH_RISCV64, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    /* MIPS */
    hr = HvBackend_MIPS_Create(HV_VIRT_SOFTWARE, HV_ARCH_MIPS32, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    hr = HvBackend_MIPS_Create(HV_VIRT_SOFTWARE, HV_ARCH_MIPS64, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    /* SPARC */
    hr = HvBackend_SPARC_Create(HV_VIRT_SOFTWARE, HV_ARCH_SPARC32, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    hr = HvBackend_SPARC_Create(HV_VIRT_SOFTWARE, HV_ARCH_SPARC64, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    /* Other architectures */
    hr = HvBackend_M68K_Create(HV_VIRT_SOFTWARE, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    hr = HvBackend_VAX_Create(HV_VIRT_SOFTWARE, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    hr = HvBackend_Alpha_Create(HV_VIRT_SOFTWARE, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    hr = HvBackend_IA64_Create(HV_VIRT_SOFTWARE, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    hr = HvBackend_PPC_Create(HV_VIRT_SOFTWARE, HV_ARCH_PPC32, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    hr = HvBackend_PPC_Create(HV_VIRT_SOFTWARE, HV_ARCH_PPC64, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    hr = HvBackend_LoongArch_Create(HV_VIRT_SOFTWARE, HV_ARCH_LOONGARCH32, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    hr = HvBackend_LoongArch_Create(HV_VIRT_SOFTWARE, HV_ARCH_LOONGARCH64, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    hr = HvBackend_DLX_Create(HV_VIRT_SOFTWARE, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    hr = HvBackend_MMIX_Create(HV_VIRT_SOFTWARE, &pBackend);
    if (SUCCEEDED(hr)) {
        HvRegisterBackend(pHv, pBackend);
    }

    pHv->Initialized = TRUE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvHypervisor_Shutdown(
    IHypervisor* This
)
{
    HvHypervisor* pHv = (HvHypervisor*)This;
    UINT32 i;

    if (!pHv->Initialized) {
        return S_OK;
    }

    /* Shutdown all VMs */
    for (i = 0; i < pHv->VMCount; i++) {
        if (pHv->VMs[i] != NULL) {
            IVirtualMachine_Release((IVirtualMachine*)pHv->VMs[i]);
            pHv->VMs[i] = NULL;
        }
    }
    pHv->VMCount = 0;

    pHv->Initialized = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvHypervisor_IsSupported(
    IHypervisor* This,
    HV_ARCHITECTURE Arch,
    HV_VIRT_MODE Mode,
    BOOLEAN* pSupported
)
{
    HvHypervisor* pHv = (HvHypervisor*)This;
    HV_CPU_BACKEND* pBackend;

    if (pSupported == NULL) {
        return E_POINTER;
    }

    /* Auto mode - check if any mode is supported */
    if (Mode == HV_VIRT_AUTO) {
        /* Try hardware first, then software */
        pBackend = HvFindBackend(pHv, Arch, HV_VIRT_HARDWARE);
        if (pBackend != NULL) {
            *pSupported = TRUE;
            return S_OK;
        }

        pBackend = HvFindBackend(pHv, Arch, HV_VIRT_BINARY_TRANS);
        if (pBackend != NULL) {
            *pSupported = TRUE;
            return S_OK;
        }

        pBackend = HvFindBackend(pHv, Arch, HV_VIRT_SOFTWARE);
        if (pBackend != NULL) {
            *pSupported = TRUE;
            return S_OK;
        }

        *pSupported = FALSE;
        return S_OK;
    }

    /* Check specific mode */
    pBackend = HvFindBackend(pHv, Arch, Mode);
    *pSupported = (pBackend != NULL);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvHypervisor_GetCapabilities(
    IHypervisor* This,
    HV_ARCHITECTURE Arch,
    UINT32* pCapabilities
)
{
    HvHypervisor* pHv = (HvHypervisor*)This;
    UINT32 caps = 0;
    UINT32 i;

    if (pCapabilities == NULL) {
        return E_POINTER;
    }

    /* Check all modes for this architecture */
    for (i = 0; i < pHv->BackendCount; i++) {
        if (pHv->Backends[i].Architecture == Arch) {
            switch (pHv->Backends[i].Mode) {
                case HV_VIRT_HARDWARE:
                    caps |= 0x01;
                    break;
                case HV_VIRT_SOFTWARE:
                    caps |= 0x02;
                    break;
                case HV_VIRT_BINARY_TRANS:
                    caps |= 0x04;
                    break;
                case HV_VIRT_PARAVIRT:
                    caps |= 0x08;
                    break;
                default:
                    break;
            }
        }
    }

    *pCapabilities = caps;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvHypervisor_CreateVM(
    IHypervisor* This,
    CONST HV_VM_CONFIG* pConfig,
    IVirtualMachine** ppVM
)
{
    HvHypervisor* pHv = (HvHypervisor*)This;
    HvVirtualMachine* pVM;
    HV_CPU_BACKEND* pBackend;
    HRESULT hr;
    UINT32 i;

    if (pConfig == NULL || ppVM == NULL) {
        return E_POINTER;
    }

    if (!pHv->Initialized) {
        return HV_ERROR;
    }

    /* Find appropriate backend */
    if (pConfig->VirtMode == HV_VIRT_AUTO) {
        /* Try modes in order of preference */
        pBackend = HvFindBackend(pHv, pConfig->Architecture, HV_VIRT_HARDWARE);
        if (pBackend == NULL) {
            pBackend = HvFindBackend(pHv, pConfig->Architecture, HV_VIRT_BINARY_TRANS);
        }
        if (pBackend == NULL) {
            pBackend = HvFindBackend(pHv, pConfig->Architecture, HV_VIRT_SOFTWARE);
        }
    } else {
        pBackend = HvFindBackend(pHv, pConfig->Architecture, pConfig->VirtMode);
    }

    if (pBackend == NULL) {
        return HV_UNSUPPORTED;
    }

    /* Allocate VM structure */
    pVM = (HvVirtualMachine*)RtlAllocateMemory(&pHv->Pool, sizeof(HvVirtualMachine));
    if (pVM == NULL) {
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pVM, sizeof(HvVirtualMachine));

    /* Initialize VM */
    pVM->RefCount = 1;
    pVM->Hypervisor = pHv;
    RtlCopyMemory(&pVM->Config, pConfig, sizeof(HV_VM_CONFIG));
    pVM->State = HV_VM_STATE_STOPPED;
    pVM->Backend = pBackend;

    /* Initialize memory pool */
    RtlInitializeMemoryPool(&pVM->Pool, NULL);

    /* Create virtual memory interface (implemented in vmem.c) */
    extern HRESULT HvVirtualMemory_Create(HvVirtualMachine* pVM, HvVirtualMemory** ppMem);
    hr = HvVirtualMemory_Create(pVM, &pVM->Memory);
    if (FAILED(hr)) {
        RtlDestroyMemoryPool(&pVM->Pool);
        RtlFreeMemory(&pHv->Pool, pVM);
        return hr;
    }

    /* Create virtual CPUs (implemented in vcpu.c) */
    pVM->NumCpus = pConfig->NumCpus;
    pVM->VCpus = (HvVirtualCpu**)RtlAllocateMemory(&pVM->Pool, sizeof(HvVirtualCpu*) * pConfig->NumCpus);
    if (pVM->VCpus == NULL) {
        IVirtualMemory_Release((IVirtualMemory*)pVM->Memory);
        RtlDestroyMemoryPool(&pVM->Pool);
        RtlFreeMemory(&pHv->Pool, pVM);
        return HV_NO_RESOURCES;
    }

    extern HRESULT HvVirtualCpu_Create(HvVirtualMachine* pVM, UINT32 CpuId, HvVirtualCpu** ppCpu);
    for (i = 0; i < pConfig->NumCpus; i++) {
        hr = HvVirtualCpu_Create(pVM, i, &pVM->VCpus[i]);
        if (FAILED(hr)) {
            /* Cleanup */
            while (i > 0) {
                i--;
                IVirtualCpu_Release((IVirtualCpu*)pVM->VCpus[i]);
            }
            RtlFreeMemory(&pVM->Pool, pVM->VCpus);
            IVirtualMemory_Release((IVirtualMemory*)pVM->Memory);
            RtlDestroyMemoryPool(&pVM->Pool);
            RtlFreeMemory(&pHv->Pool, pVM);
            return hr;
        }
    }

    /* Add to hypervisor's VM list */
    AnxInterlockedIncrement((volatile INT32*)&pHv->RefCount);

    /* Setup vtable (defined in vm.c) */
    extern IVirtualMachineVtbl g_VirtualMachineVtbl;
    pVM->Interface.lpVtbl = &g_VirtualMachineVtbl;

    *ppVM = (IVirtualMachine*)pVM;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvHypervisor_EnumerateArchitectures(
    IHypervisor* This,
    HV_ARCHITECTURE* pArchitectures,
    UINT32* pCount
)
{
    HvHypervisor* pHv = (HvHypervisor*)This;
    UINT32 uniqueCount = 0;
    UINT32 i, j;
    BOOLEAN found;

    if (pCount == NULL) {
        return E_POINTER;
    }

    /* Count unique architectures */
    for (i = 0; i < pHv->BackendCount; i++) {
        found = FALSE;
        for (j = 0; j < uniqueCount; j++) {
            if (pArchitectures != NULL && pArchitectures[j] == pHv->Backends[i].Architecture) {
                found = TRUE;
                break;
            }
        }
        if (!found) {
            if (pArchitectures != NULL && uniqueCount < *pCount) {
                pArchitectures[uniqueCount] = pHv->Backends[i].Architecture;
            }
            uniqueCount++;
        }
    }

    if (pArchitectures == NULL || *pCount < uniqueCount) {
        *pCount = uniqueCount;
        return pArchitectures == NULL ? S_OK : HV_ERROR;
    }

    *pCount = uniqueCount;
    return S_OK;
}

/* VTable for IHypervisor */
static IHypervisorVtbl g_HypervisorVtbl = {
    HvHypervisor_QueryInterface,
    HvHypervisor_AddRef,
    HvHypervisor_Release,
    HvHypervisor_Initialize,
    HvHypervisor_Shutdown,
    HvHypervisor_IsSupported,
    HvHypervisor_GetCapabilities,
    HvHypervisor_CreateVM,
    HvHypervisor_EnumerateArchitectures
};

/* ======================================================================= */
/* Internal helper functions                                               */
/* ======================================================================= */

HRESULT
HvRegisterBackend(
    HvHypervisor* pHv,
    HV_CPU_BACKEND* pBackend
)
{
    HV_CPU_BACKEND* newBackends;
    UINT32 newCount = pHv->BackendCount + 1;

    newBackends = (HV_CPU_BACKEND*)RtlAllocateMemory(&pHv->Pool, sizeof(HV_CPU_BACKEND) * newCount);
    if (newBackends == NULL) {
        return HV_NO_RESOURCES;
    }

    if (pHv->Backends != NULL) {
        RtlCopyMemory(newBackends, pHv->Backends, sizeof(HV_CPU_BACKEND) * pHv->BackendCount);
        RtlFreeMemory(&pHv->Pool, pHv->Backends);
    }

    RtlCopyMemory(&newBackends[pHv->BackendCount], pBackend, sizeof(HV_CPU_BACKEND));
    pHv->Backends = newBackends;
    pHv->BackendCount = newCount;

    return S_OK;
}

HV_CPU_BACKEND*
HvFindBackend(
    HvHypervisor* pHv,
    HV_ARCHITECTURE Arch,
    HV_VIRT_MODE Mode
)
{
    UINT32 i;

    for (i = 0; i < pHv->BackendCount; i++) {
        if (pHv->Backends[i].Architecture == Arch &&
            pHv->Backends[i].Mode == Mode) {
            return &pHv->Backends[i];
        }
    }

    return NULL;
}

/* ======================================================================= */
/* Factory function                                                        */
/* ======================================================================= */

HRESULT
HvCreateHypervisor(
    REFIID riid,
    VOID** ppvObject
)
{
    HvHypervisor* pHv;
    HRESULT hr;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    /* Allocate hypervisor */
    pHv = (HvHypervisor*)RtlAllocateMemory(NULL, sizeof(HvHypervisor));
    if (pHv == NULL) {
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pHv, sizeof(HvHypervisor));

    /* Initialize */
    pHv->Interface.lpVtbl = &g_HypervisorVtbl;
    pHv->RefCount = 1;
    pHv->Initialized = FALSE;

    /* Initialize memory pool */
    RtlInitializeMemoryPool(&pHv->Pool, NULL);

    /* Query interface */
    hr = HvHypervisor_QueryInterface((IHypervisor*)pHv, riid, ppvObject);
    if (FAILED(hr)) {
        RtlDestroyMemoryPool(&pHv->Pool);
        RtlFreeMemory(NULL, pHv);
        return hr;
    }

    /* Release initial reference (QI added one) */
    HvHypervisor_Release((IHypervisor*)pHv);

    return S_OK;
}
