/*++
    Module Name:

        tc_cache.c

    Abstract:

        Translation cache implementation for binary translation (VMware/Plex86 style).

--*/

#include "hypervisor_impl.h"

#define TC_HASH_SIZE 1024
#define TC_CODE_CACHE_SIZE (4 * 1024 * 1024)  /* 4MB code cache */

/* ======================================================================= */
/* Hash function                                                           */
/* ======================================================================= */

static UINT32
HvTC_Hash(UINT64 GuestAddr)
{
    return (UINT32)((GuestAddr >> 2) % TC_HASH_SIZE);
}

/* ======================================================================= */
/* Translation cache functions                                             */
/* ======================================================================= */

HRESULT
HvTC_Initialize(
    HvVirtualCpu* pCpu
)
{
    HV_TC_CACHE* pCache;

    /* Allocate cache structure */
    pCache = (HV_TC_CACHE*)RtlAllocateMemory(&pCpu->VM->Pool, sizeof(HV_TC_CACHE));
    if (pCache == NULL) {
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pCache, sizeof(HV_TC_CACHE));

    /* Allocate hash table */
    pCache->HashTable = (HV_TC_BLOCK**)RtlAllocateMemory(&pCpu->VM->Pool, sizeof(HV_TC_BLOCK*) * TC_HASH_SIZE);
    if (pCache->HashTable == NULL) {
        RtlFreeMemory(&pCpu->VM->Pool, pCache);
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pCache->HashTable, sizeof(HV_TC_BLOCK*) * TC_HASH_SIZE);

    /* Allocate code cache (executable memory) */
    /* TODO: Allocate executable memory pages */
    pCache->CodeMemory = RtlAllocateMemory(&pCpu->VM->Pool, TC_CODE_CACHE_SIZE);
    if (pCache->CodeMemory == NULL) {
        RtlFreeMemory(&pCpu->VM->Pool, pCache->HashTable);
        RtlFreeMemory(&pCpu->VM->Pool, pCache);
        return HV_NO_RESOURCES;
    }

    pCache->HashSize = TC_HASH_SIZE;
    pCache->CacheSize = TC_CODE_CACHE_SIZE;
    pCache->CacheUsed = 0;
    pCache->BlockCount = 0;

    /* Initialize memory pool */
    RtlInitializeMemoryPool(&pCache->Pool, NULL);

    pCpu->TransCache = pCache;
    return S_OK;
}

HRESULT
HvTC_Shutdown(
    HvVirtualCpu* pCpu
)
{
    HV_TC_CACHE* pCache = pCpu->TransCache;
    UINT32 i;
    HV_TC_BLOCK* pBlock;
    HV_TC_BLOCK* pNext;

    if (pCache == NULL) {
        return S_OK;
    }

    /* Free all blocks */
    for (i = 0; i < pCache->HashSize; i++) {
        pBlock = pCache->HashTable[i];
        while (pBlock != NULL) {
            pNext = pBlock->Next;
            RtlFreeMemory(&pCache->Pool, pBlock);
            pBlock = pNext;
        }
    }

    /* Free hash table */
    if (pCache->HashTable != NULL) {
        RtlFreeMemory(&pCpu->VM->Pool, pCache->HashTable);
    }

    /* Free code cache */
    if (pCache->CodeMemory != NULL) {
        RtlFreeMemory(&pCpu->VM->Pool, pCache->CodeMemory);
    }

    /* Destroy memory pool */
    RtlDestroyMemoryPool(&pCache->Pool);

    /* Free cache */
    RtlFreeMemory(&pCpu->VM->Pool, pCache);
    pCpu->TransCache = NULL;

    return S_OK;
}

HRESULT
HvTC_Lookup(
    HvVirtualCpu* pCpu,
    UINT64 GuestAddr,
    HV_TC_BLOCK** ppBlock
)
{
    HV_TC_CACHE* pCache = pCpu->TransCache;
    UINT32 hash;
    HV_TC_BLOCK* pBlock;

    if (pCache == NULL || ppBlock == NULL) {
        return E_POINTER;
    }

    hash = HvTC_Hash(GuestAddr);
    pBlock = pCache->HashTable[hash];

    while (pBlock != NULL) {
        if (pBlock->GuestAddress == GuestAddr) {
            pBlock->ExecutionCount++;
            *ppBlock = pBlock;
            return S_OK;
        }
        pBlock = pBlock->Next;
    }

    *ppBlock = NULL;
    return HV_NO_DEVICE;
}

HRESULT
HvTC_Insert(
    HvVirtualCpu* pCpu,
    UINT64 GuestAddr,
    VOID* HostCode,
    UINT32 GuestSize,
    UINT32 HostSize
)
{
    HV_TC_CACHE* pCache = pCpu->TransCache;
    UINT32 hash;
    HV_TC_BLOCK* pBlock;

    if (pCache == NULL) {
        return E_POINTER;
    }

    /* Check if cache is full */
    if (pCache->CacheUsed + HostSize > pCache->CacheSize) {
        /* Flush cache and start over */
        HvTC_Flush(pCpu);
    }

    /* Allocate block */
    pBlock = (HV_TC_BLOCK*)RtlAllocateMemory(&pCache->Pool, sizeof(HV_TC_BLOCK));
    if (pBlock == NULL) {
        return HV_NO_RESOURCES;
    }

    /* Initialize block */
    pBlock->GuestAddress = GuestAddr;
    pBlock->HostAddress = (UINT64)(UINTN)HostCode;
    pBlock->GuestSize = GuestSize;
    pBlock->HostSize = HostSize;
    pBlock->ExecutionCount = 0;
    pBlock->Flags = 0;

    /* Add to hash table */
    hash = HvTC_Hash(GuestAddr);
    pBlock->Next = pCache->HashTable[hash];
    pCache->HashTable[hash] = pBlock;

    pCache->BlockCount++;
    pCache->CacheUsed += HostSize;

    return S_OK;
}

HRESULT
HvTC_Invalidate(
    HvVirtualCpu* pCpu,
    UINT64 GuestAddr
)
{
    HV_TC_CACHE* pCache = pCpu->TransCache;
    UINT32 hash;
    HV_TC_BLOCK* pBlock;
    HV_TC_BLOCK* pPrev;

    if (pCache == NULL) {
        return E_POINTER;
    }

    hash = HvTC_Hash(GuestAddr);
    pPrev = NULL;
    pBlock = pCache->HashTable[hash];

    while (pBlock != NULL) {
        if (pBlock->GuestAddress == GuestAddr) {
            /* Remove from hash table */
            if (pPrev == NULL) {
                pCache->HashTable[hash] = pBlock->Next;
            } else {
                pPrev->Next = pBlock->Next;
            }

            /* Free block */
            pCache->CacheUsed -= pBlock->HostSize;
            pCache->BlockCount--;
            RtlFreeMemory(&pCache->Pool, pBlock);

            return S_OK;
        }

        pPrev = pBlock;
        pBlock = pBlock->Next;
    }

    return HV_NO_DEVICE;
}

HRESULT
HvTC_Flush(
    HvVirtualCpu* pCpu
)
{
    HV_TC_CACHE* pCache = pCpu->TransCache;
    UINT32 i;
    HV_TC_BLOCK* pBlock;
    HV_TC_BLOCK* pNext;

    if (pCache == NULL) {
        return E_POINTER;
    }

    /* Free all blocks */
    for (i = 0; i < pCache->HashSize; i++) {
        pBlock = pCache->HashTable[i];
        while (pBlock != NULL) {
            pNext = pBlock->Next;
            RtlFreeMemory(&pCache->Pool, pBlock);
            pBlock = pNext;
        }
        pCache->HashTable[i] = NULL;
    }

    pCache->BlockCount = 0;
    pCache->CacheUsed = 0;

    return S_OK;
}
