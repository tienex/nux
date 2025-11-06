/*++
    Module Name:

        vmem.c

    Abstract:

        Virtual memory management with shadow page tables and nested paging.

--*/

#include "hypervisor_impl.h"
#include <ananke/atomics.h>

/* ======================================================================= */
/* IVirtualMemory implementation                                           */
/* ======================================================================= */

static HRESULT STDMETHODCALLTYPE
HvVMem_QueryInterface(
    IVirtualMemory* This,
    REFIID riid,
    VOID** ppvObject
)
{
    HvVirtualMemory* pMem = (HvVirtualMemory*)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (RtlIsEqualGUID(riid, &IID_IUnknown) ||
        RtlIsEqualGUID(riid, &IID_IVirtualMemory)) {
        *ppvObject = &pMem->Interface;
        AnxInterlockedIncrement((volatile INT32*)&pMem->RefCount);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
HvVMem_AddRef(
    IVirtualMemory* This
)
{
    HvVirtualMemory* pMem = (HvVirtualMemory*)This;
    return (UINT32)AnxInterlockedIncrement((volatile INT32*)&pMem->RefCount);
}

static UINT32 STDMETHODCALLTYPE
HvVMem_Release(
    IVirtualMemory* This
)
{
    HvVirtualMemory* pMem = (HvVirtualMemory*)This;
    UINT32 refCount = (UINT32)AnxInterlockedDecrement((volatile INT32*)&pMem->RefCount);

    if (refCount == 0) {
        HV_MEM_REGION* pRegion;
        HV_MEM_REGION* pNext;

        /* Free all memory regions */
        pRegion = pMem->Regions;
        while (pRegion != NULL) {
            pNext = pRegion->Next;
            RtlFreeMemory(&pMem->Pool, pRegion);
            pRegion = pNext;
        }

        /* Shutdown shadow page tables */
        if (pMem->ShadowPageTables != NULL) {
            HvShadowPT_Shutdown(pMem);
        }

        /* Destroy memory pool */
        RtlDestroyMemoryPool(&pMem->Pool);

        /* Free memory structure */
        RtlFreeMemory(&pMem->VM->Pool, pMem);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
HvVMem_MapMemory(
    IVirtualMemory* This,
    UINT64 GuestPhysicalAddress,
    UINT64 HostVirtualAddress,
    UINT64 Size,
    UINT32 Flags
)
{
    HvVirtualMemory* pMem = (HvVirtualMemory*)This;
    HV_MEM_REGION* pRegion;

    if (Size == 0) {
        return E_INVALIDARG;
    }

    /* Allocate region */
    pRegion = (HV_MEM_REGION*)RtlAllocateMemory(&pMem->Pool, sizeof(HV_MEM_REGION));
    if (pRegion == NULL) {
        return HV_NO_RESOURCES;
    }

    /* Initialize region */
    pRegion->GuestPhysicalAddress = GuestPhysicalAddress;
    pRegion->HostVirtualAddress = HostVirtualAddress;
    pRegion->Size = Size;
    pRegion->Flags = Flags;
    pRegion->RefCount = 1;

    /* Add to list */
    pRegion->Next = pMem->Regions;
    pMem->Regions = pRegion;
    pMem->RegionCount++;

    /* Map in shadow page tables if enabled */
    if (!pMem->NestedPagingEnabled && pMem->VM->Config.EnableShadowPageTables) {
        return HvShadowPT_Map(pMem, GuestPhysicalAddress, HostVirtualAddress, Size, Flags);
    }

    /* For nested paging, the mapping is managed by hardware */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVMem_UnmapMemory(
    IVirtualMemory* This,
    UINT64 GuestPhysicalAddress,
    UINT64 Size
)
{
    HvVirtualMemory* pMem = (HvVirtualMemory*)This;
    HV_MEM_REGION* pRegion;
    HV_MEM_REGION* pPrev;

    /* Find and remove region */
    pPrev = NULL;
    pRegion = pMem->Regions;
    while (pRegion != NULL) {
        if (pRegion->GuestPhysicalAddress == GuestPhysicalAddress &&
            pRegion->Size == Size) {
            /* Remove from list */
            if (pPrev == NULL) {
                pMem->Regions = pRegion->Next;
            } else {
                pPrev->Next = pRegion->Next;
            }
            pMem->RegionCount--;

            /* Unmap from shadow page tables if enabled */
            if (!pMem->NestedPagingEnabled && pMem->VM->Config.EnableShadowPageTables) {
                HvShadowPT_Unmap(pMem, GuestPhysicalAddress, Size);
            }

            /* Free region */
            RtlFreeMemory(&pMem->Pool, pRegion);
            return S_OK;
        }

        pPrev = pRegion;
        pRegion = pRegion->Next;
    }

    return HV_NO_DEVICE;
}

static HRESULT STDMETHODCALLTYPE
HvVMem_ProtectMemory(
    IVirtualMemory* This,
    UINT64 GuestPhysicalAddress,
    UINT64 Size,
    UINT32 Flags
)
{
    HvVirtualMemory* pMem = (HvVirtualMemory*)This;
    HV_MEM_REGION* pRegion;

    /* Find region */
    pRegion = pMem->Regions;
    while (pRegion != NULL) {
        if (GuestPhysicalAddress >= pRegion->GuestPhysicalAddress &&
            GuestPhysicalAddress + Size <= pRegion->GuestPhysicalAddress + pRegion->Size) {
            /* Update flags */
            pRegion->Flags = Flags;

            /* Update shadow page tables if enabled */
            if (!pMem->NestedPagingEnabled && pMem->VM->Config.EnableShadowPageTables) {
                /* Re-map with new flags */
                HvShadowPT_Unmap(pMem, GuestPhysicalAddress, Size);
                return HvShadowPT_Map(pMem, GuestPhysicalAddress,
                    pRegion->HostVirtualAddress + (GuestPhysicalAddress - pRegion->GuestPhysicalAddress),
                    Size, Flags);
            }

            return S_OK;
        }

        pRegion = pRegion->Next;
    }

    return HV_NO_DEVICE;
}

static HRESULT STDMETHODCALLTYPE
HvVMem_ReadMemory(
    IVirtualMemory* This,
    UINT64 GuestPhysicalAddress,
    VOID* Buffer,
    UINT64 Size
)
{
    HvVirtualMemory* pMem = (HvVirtualMemory*)This;
    HV_MEM_REGION* pRegion;
    UINT64 offset;

    if (Buffer == NULL) {
        return E_POINTER;
    }

    /* Find region containing this address */
    pRegion = pMem->Regions;
    while (pRegion != NULL) {
        if (GuestPhysicalAddress >= pRegion->GuestPhysicalAddress &&
            GuestPhysicalAddress + Size <= pRegion->GuestPhysicalAddress + pRegion->Size) {
            /* Calculate offset */
            offset = GuestPhysicalAddress - pRegion->GuestPhysicalAddress;

            /* Copy from host memory */
            RtlCopyMemory(Buffer, (VOID*)(UINTN)(pRegion->HostVirtualAddress + offset), (SIZE_T)Size);
            return S_OK;
        }

        pRegion = pRegion->Next;
    }

    return HV_NO_DEVICE;
}

static HRESULT STDMETHODCALLTYPE
HvVMem_WriteMemory(
    IVirtualMemory* This,
    UINT64 GuestPhysicalAddress,
    CONST VOID* Buffer,
    UINT64 Size
)
{
    HvVirtualMemory* pMem = (HvVirtualMemory*)This;
    HV_MEM_REGION* pRegion;
    UINT64 offset;

    if (Buffer == NULL) {
        return E_POINTER;
    }

    /* Find region containing this address */
    pRegion = pMem->Regions;
    while (pRegion != NULL) {
        if (GuestPhysicalAddress >= pRegion->GuestPhysicalAddress &&
            GuestPhysicalAddress + Size <= pRegion->GuestPhysicalAddress + pRegion->Size) {
            /* Check write permission */
            if (!(pRegion->Flags & HV_MEMORY_WRITE)) {
                return HV_DENIED;
            }

            /* Calculate offset */
            offset = GuestPhysicalAddress - pRegion->GuestPhysicalAddress;

            /* Copy to host memory */
            RtlCopyMemory((VOID*)(UINTN)(pRegion->HostVirtualAddress + offset), Buffer, (SIZE_T)Size);
            return S_OK;
        }

        pRegion = pRegion->Next;
    }

    return HV_NO_DEVICE;
}

static HRESULT STDMETHODCALLTYPE
HvVMem_TranslateGVA(
    IVirtualMemory* This,
    UINT32 CpuId,
    UINT64 GuestVirtualAddress,
    UINT64* pGuestPhysicalAddress
)
{
    HvVirtualMemory* pMem = (HvVirtualMemory*)This;

    if (pGuestPhysicalAddress == NULL) {
        return E_POINTER;
    }

    /* Use shadow page tables if enabled */
    if (!pMem->NestedPagingEnabled && pMem->VM->Config.EnableShadowPageTables) {
        return HvShadowPT_Translate(pMem, GuestVirtualAddress, pGuestPhysicalAddress);
    }

    /* For now, identity mapping for nested paging */
    *pGuestPhysicalAddress = GuestVirtualAddress;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVMem_QueryMemoryRegions(
    IVirtualMemory* This,
    HV_MEMORY_REGION* pRegions,
    UINT32* pCount
)
{
    HvVirtualMemory* pMem = (HvVirtualMemory*)This;
    HV_MEM_REGION* pRegion;
    UINT32 count = 0;

    if (pCount == NULL) {
        return E_POINTER;
    }

    /* Count regions */
    pRegion = pMem->Regions;
    while (pRegion != NULL) {
        if (pRegions != NULL && count < *pCount) {
            pRegions[count].GuestPhysicalAddress = pRegion->GuestPhysicalAddress;
            pRegions[count].HostVirtualAddress = pRegion->HostVirtualAddress;
            pRegions[count].Size = pRegion->Size;
            pRegions[count].Flags = pRegion->Flags;
        }
        count++;
        pRegion = pRegion->Next;
    }

    if (pRegions == NULL || *pCount < count) {
        *pCount = count;
        return pRegions == NULL ? S_OK : HV_ERROR;
    }

    *pCount = count;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvVMem_FlushTLB(
    IVirtualMemory* This,
    UINT32 CpuId
)
{
    HvVirtualMemory* pMem = (HvVirtualMemory*)This;

    /* Flush TLB for the CPU */
    (VOID)pMem;
    (VOID)CpuId;

    /* TODO: Implement TLB flush via backend */
    return S_OK;
}

/* VTable for IVirtualMemory */
static IVirtualMemoryVtbl g_VirtualMemoryVtbl = {
    HvVMem_QueryInterface,
    HvVMem_AddRef,
    HvVMem_Release,
    HvVMem_MapMemory,
    HvVMem_UnmapMemory,
    HvVMem_ProtectMemory,
    HvVMem_ReadMemory,
    HvVMem_WriteMemory,
    HvVMem_TranslateGVA,
    HvVMem_QueryMemoryRegions,
    HvVMem_FlushTLB
};

/* ======================================================================= */
/* Virtual memory creation                                                 */
/* ======================================================================= */

HRESULT
HvVirtualMemory_Create(
    HvVirtualMachine* pVM,
    HvVirtualMemory** ppMem
)
{
    HvVirtualMemory* pMem;
    HRESULT hr;

    if (ppMem == NULL) {
        return E_POINTER;
    }

    /* Allocate memory structure */
    pMem = (HvVirtualMemory*)RtlAllocateMemory(&pVM->Pool, sizeof(HvVirtualMemory));
    if (pMem == NULL) {
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pMem, sizeof(HvVirtualMemory));

    /* Initialize */
    pMem->Interface.lpVtbl = &g_VirtualMemoryVtbl;
    pMem->RefCount = 1;
    pMem->VM = pVM;
    pMem->NestedPagingEnabled = pVM->Config.EnableNestedPaging;

    /* Initialize memory pool */
    RtlInitializeMemoryPool(&pMem->Pool, NULL);

    /* Initialize shadow page tables if enabled */
    if (!pMem->NestedPagingEnabled && pVM->Config.EnableShadowPageTables) {
        hr = HvShadowPT_Initialize(pMem);
        if (FAILED(hr)) {
            RtlDestroyMemoryPool(&pMem->Pool);
            RtlFreeMemory(&pVM->Pool, pMem);
            return hr;
        }
    }

    *ppMem = pMem;
    return S_OK;
}

/* ======================================================================= */
/* Shadow page table implementation                                        */
/* ======================================================================= */

HRESULT
HvShadowPT_Initialize(
    HvVirtualMemory* pMem
)
{
    /* Allocate shadow page table structures */
    pMem->ShadowPageTables = (HV_SHADOW_PT*)RtlAllocateMemory(&pMem->Pool, sizeof(HV_SHADOW_PT) * 4);
    if (pMem->ShadowPageTables == NULL) {
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pMem->ShadowPageTables, sizeof(HV_SHADOW_PT) * 4);
    pMem->ShadowPTCount = 4;  /* 4-level page table */

    return S_OK;
}

HRESULT
HvShadowPT_Shutdown(
    HvVirtualMemory* pMem
)
{
    UINT32 i;

    if (pMem->ShadowPageTables != NULL) {
        for (i = 0; i < pMem->ShadowPTCount; i++) {
            if (pMem->ShadowPageTables[i].Entries != NULL) {
                RtlFreeMemory(&pMem->Pool, pMem->ShadowPageTables[i].Entries);
            }
        }
        RtlFreeMemory(&pMem->Pool, pMem->ShadowPageTables);
        pMem->ShadowPageTables = NULL;
    }

    return S_OK;
}

HRESULT
HvShadowPT_Map(
    HvVirtualMemory* pMem,
    UINT64 GuestPhys,
    UINT64 HostVirt,
    UINT64 Size,
    UINT32 Flags
)
{
    HV_SHADOW_PT* pPT;
    HV_SHADOW_PTE* pNewEntries;
    UINT64 pageCount;
    UINT64 i;

    /* Calculate number of pages */
    pageCount = (Size + 0xFFF) / 0x1000;

    /* Use level 0 (4KB pages) for simplicity */
    pPT = &pMem->ShadowPageTables[0];

    /* Expand entry array */
    pNewEntries = (HV_SHADOW_PTE*)RtlAllocateMemory(&pMem->Pool,
        sizeof(HV_SHADOW_PTE) * (pPT->EntryCount + (UINT32)pageCount));
    if (pNewEntries == NULL) {
        return HV_NO_RESOURCES;
    }

    /* Copy existing entries */
    if (pPT->Entries != NULL) {
        RtlCopyMemory(pNewEntries, pPT->Entries, sizeof(HV_SHADOW_PTE) * pPT->EntryCount);
        RtlFreeMemory(&pMem->Pool, pPT->Entries);
    }

    /* Add new entries */
    for (i = 0; i < pageCount; i++) {
        pNewEntries[pPT->EntryCount + i].GuestPhysical = GuestPhys + (i * 0x1000);
        pNewEntries[pPT->EntryCount + i].HostPhysical = HostVirt + (i * 0x1000);
        pNewEntries[pPT->EntryCount + i].Flags = Flags;
        pNewEntries[pPT->EntryCount + i].AccessCount = 0;

        /* Build page table entries (simplified) */
        pNewEntries[pPT->EntryCount + i].GuestPTE = GuestPhys + (i * 0x1000) | Flags;
        pNewEntries[pPT->EntryCount + i].HostPTE = HostVirt + (i * 0x1000) | Flags;
    }

    pPT->Entries = pNewEntries;
    pPT->EntryCount += (UINT32)pageCount;

    return S_OK;
}

HRESULT
HvShadowPT_Unmap(
    HvVirtualMemory* pMem,
    UINT64 GuestPhys,
    UINT64 Size
)
{
    HV_SHADOW_PT* pPT;
    UINT32 i, j;
    UINT64 endAddr = GuestPhys + Size;

    pPT = &pMem->ShadowPageTables[0];

    /* Remove entries in range */
    for (i = 0; i < pPT->EntryCount; ) {
        if (pPT->Entries[i].GuestPhysical >= GuestPhys &&
            pPT->Entries[i].GuestPhysical < endAddr) {
            /* Remove entry */
            for (j = i; j < pPT->EntryCount - 1; j++) {
                pPT->Entries[j] = pPT->Entries[j + 1];
            }
            pPT->EntryCount--;
        } else {
            i++;
        }
    }

    return S_OK;
}

HRESULT
HvShadowPT_Translate(
    HvVirtualMemory* pMem,
    UINT64 GuestVirt,
    UINT64* pGuestPhys
)
{
    HV_SHADOW_PT* pPT;
    UINT32 i;
    UINT64 pageBase;

    if (pGuestPhys == NULL) {
        return E_POINTER;
    }

    /* Simple page table walk */
    pageBase = GuestVirt & ~0xFFFULL;
    pPT = &pMem->ShadowPageTables[0];

    for (i = 0; i < pPT->EntryCount; i++) {
        if ((pPT->Entries[i].GuestPhysical & ~0xFFFULL) == pageBase) {
            *pGuestPhys = pPT->Entries[i].GuestPhysical | (GuestVirt & 0xFFF);
            pPT->Entries[i].AccessCount++;
            return S_OK;
        }
    }

    /* Page not found - identity mapping as fallback */
    *pGuestPhys = GuestVirt;
    return S_OK;
}
