/*++
    Module Name:

        mol_drivers.c

    Abstract:

        Mac-on-Linux style paravirtual drivers for optimized guest-host
        communication. Provides hypercall interface and paravirtualized
        device drivers for improved performance.

--*/

#include "../hypervisor_impl.h"

/* ======================================================================= */
/* Hypercall interface (Mac-on-Linux style)                                */
/* ======================================================================= */

/* Hypercall numbers */
typedef enum HV_HYPERCALL {
    HV_HC_CONSOLE_WRITE       = 0x01,  /* Write to console */
    HV_HC_CONSOLE_READ        = 0x02,  /* Read from console */
    HV_HC_GET_TIME            = 0x10,  /* Get host time */
    HV_HC_YIELD               = 0x20,  /* Yield CPU */
    HV_HC_MAP_MEMORY          = 0x30,  /* Map shared memory */
    HV_HC_UNMAP_MEMORY        = 0x31,  /* Unmap shared memory */
    HV_HC_DISK_READ           = 0x40,  /* Optimized disk read */
    HV_HC_DISK_WRITE          = 0x41,  /* Optimized disk write */
    HV_HC_NETWORK_SEND        = 0x50,  /* Optimized network send */
    HV_HC_NETWORK_RECV        = 0x51,  /* Optimized network receive */
} HV_HYPERCALL;

/* Hypercall arguments structure */
typedef struct HV_HYPERCALL_ARGS {
    UINT64 Arg0;
    UINT64 Arg1;
    UINT64 Arg2;
    UINT64 Arg3;
    UINT64 Arg4;
    UINT64 Arg5;
} HV_HYPERCALL_ARGS;

/* ======================================================================= */
/* Hypercall handler                                                       */
/* ======================================================================= */

HRESULT
HvHandleHypercall(
    HvVirtualCpu* pCpu,
    UINT32 CallNumber,
    HV_HYPERCALL_ARGS* pArgs,
    UINT64* pResult
)
{
    HV_VM_EXIT_INFO exitInfo;

    if (pArgs == NULL || pResult == NULL) {
        return E_POINTER;
    }

    *pResult = 0;

    switch (CallNumber) {
        case HV_HC_CONSOLE_WRITE:
            /* Write to console
             * Arg0 = guest buffer address
             * Arg1 = length
             */
            {
                IVirtualMemory* pMem;
                CHAR8 buffer[256];
                UINT64 length = pArgs->Arg1;

                if (length > sizeof(buffer) - 1) {
                    length = sizeof(buffer) - 1;
                }

                /* Get memory interface */
                IVirtualMachine_GetVirtualMemory((IVirtualMachine*)pCpu->VM, &pMem);
                if (pMem != NULL) {
                    /* Read from guest memory */
                    IVirtualMemory_ReadMemory(pMem, pArgs->Arg0, buffer, length);
                    buffer[length] = '\0';

                    /* Output to host (simplified - would go to console) */
                    /* printf("%s", buffer); */

                    IVirtualMemory_Release(pMem);
                    *pResult = length;
                }
            }
            break;

        case HV_HC_CONSOLE_READ:
            /* Read from console
             * Arg0 = guest buffer address
             * Arg1 = max length
             */
            *pResult = 0;  /* No data available (stub) */
            break;

        case HV_HC_GET_TIME:
            /* Get host time (nanoseconds since epoch) */
            /* In a real implementation, would query host time */
            *pResult = 0;
            break;

        case HV_HC_YIELD:
            /* Yield CPU - cause VM exit and reschedule */
            exitInfo.Reason = HV_EXIT_SYSCALL;
            exitInfo.CpuId = pCpu->CpuId;
            pCpu->State = HV_VM_STATE_PAUSED;
            *pResult = 0;
            break;

        case HV_HC_MAP_MEMORY:
            /* Map shared memory region
             * Arg0 = host physical address
             * Arg1 = guest virtual address
             * Arg2 = size
             * Arg3 = flags
             */
            {
                IVirtualMemory* pMem;
                IVirtualMachine_GetVirtualMemory((IVirtualMachine*)pCpu->VM, &pMem);
                if (pMem != NULL) {
                    HRESULT hr = IVirtualMemory_MapMemory(pMem, pArgs->Arg0, pArgs->Arg1,
                        pArgs->Arg2, (UINT32)pArgs->Arg3);
                    *pResult = SUCCEEDED(hr) ? 0 : (UINT64)-1;
                    IVirtualMemory_Release(pMem);
                }
            }
            break;

        case HV_HC_UNMAP_MEMORY:
            /* Unmap shared memory region
             * Arg0 = guest physical address
             * Arg1 = size
             */
            {
                IVirtualMemory* pMem;
                IVirtualMachine_GetVirtualMemory((IVirtualMachine*)pCpu->VM, &pMem);
                if (pMem != NULL) {
                    HRESULT hr = IVirtualMemory_UnmapMemory(pMem, pArgs->Arg0, pArgs->Arg1);
                    *pResult = SUCCEEDED(hr) ? 0 : (UINT64)-1;
                    IVirtualMemory_Release(pMem);
                }
            }
            break;

        case HV_HC_DISK_READ:
            /* Optimized disk read
             * Arg0 = disk ID
             * Arg1 = sector LBA
             * Arg2 = sector count
             * Arg3 = guest buffer address
             */
            /* In a real implementation, would directly access disk device */
            *pResult = pArgs->Arg2;  /* Return sectors read */
            break;

        case HV_HC_DISK_WRITE:
            /* Optimized disk write
             * Arg0 = disk ID
             * Arg1 = sector LBA
             * Arg2 = sector count
             * Arg3 = guest buffer address
             */
            /* In a real implementation, would directly access disk device */
            *pResult = pArgs->Arg2;  /* Return sectors written */
            break;

        case HV_HC_NETWORK_SEND:
            /* Optimized network packet send
             * Arg0 = network device ID
             * Arg1 = guest buffer address
             * Arg2 = packet length
             */
            /* In a real implementation, would directly queue packet */
            *pResult = 0;
            break;

        case HV_HC_NETWORK_RECV:
            /* Optimized network packet receive
             * Arg0 = network device ID
             * Arg1 = guest buffer address
             * Arg2 = max length
             * Returns: actual length
             */
            /* In a real implementation, would directly dequeue packet */
            *pResult = 0;  /* No packets available */
            break;

        default:
            /* Unknown hypercall */
            return HV_UNSUPPORTED;
    }

    return S_OK;
}

/* ======================================================================= */
/* Paravirtual console device                                              */
/* ======================================================================= */

typedef struct HV_PARAVIRT_CONSOLE {
    /* Shared memory region */
    VOID* SharedBuffer;
    UINT32 BufferSize;
    volatile UINT32* WriteOffset;
    volatile UINT32* ReadOffset;

    /* Host-side handling */
    VOID (*OutputCallback)(VOID* Context, CONST CHAR8* Text, UINT32 Length);
    VOID* OutputContext;
} HV_PARAVIRT_CONSOLE;

HRESULT
HvParavirtConsole_Create(
    UINT32 BufferSize,
    HV_PARAVIRT_CONSOLE** ppConsole
)
{
    HV_PARAVIRT_CONSOLE* pConsole;

    if (ppConsole == NULL) {
        return E_POINTER;
    }

    pConsole = (HV_PARAVIRT_CONSOLE*)RtlAllocateMemory(NULL, sizeof(HV_PARAVIRT_CONSOLE));
    if (pConsole == NULL) {
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pConsole, sizeof(HV_PARAVIRT_CONSOLE));

    /* Allocate shared buffer */
    pConsole->BufferSize = BufferSize;
    pConsole->SharedBuffer = RtlAllocateMemory(NULL, BufferSize + 8);  /* +8 for offsets */
    if (pConsole->SharedBuffer == NULL) {
        RtlFreeMemory(NULL, pConsole);
        return HV_NO_RESOURCES;
    }

    /* Setup offset pointers */
    pConsole->WriteOffset = (volatile UINT32*)pConsole->SharedBuffer;
    pConsole->ReadOffset = (volatile UINT32*)((UINT8*)pConsole->SharedBuffer + 4);
    *pConsole->WriteOffset = 0;
    *pConsole->ReadOffset = 0;

    *ppConsole = pConsole;
    return S_OK;
}

HRESULT
HvParavirtConsole_Write(
    HV_PARAVIRT_CONSOLE* pConsole,
    CONST CHAR8* Text,
    UINT32 Length
)
{
    UINT8* buffer;
    UINT32 writePos;
    UINT32 readPos;
    UINT32 available;
    UINT32 i;

    if (pConsole == NULL || Text == NULL) {
        return E_POINTER;
    }

    buffer = (UINT8*)pConsole->SharedBuffer + 8;
    writePos = *pConsole->WriteOffset;
    readPos = *pConsole->ReadOffset;

    /* Calculate available space */
    if (writePos >= readPos) {
        available = pConsole->BufferSize - (writePos - readPos) - 1;
    } else {
        available = readPos - writePos - 1;
    }

    if (Length > available) {
        Length = available;
    }

    /* Copy data to circular buffer */
    for (i = 0; i < Length; i++) {
        buffer[writePos] = (UINT8)Text[i];
        writePos = (writePos + 1) % pConsole->BufferSize;
    }

    *pConsole->WriteOffset = writePos;

    /* Call output callback if set */
    if (pConsole->OutputCallback != NULL) {
        pConsole->OutputCallback(pConsole->OutputContext, Text, Length);
    }

    return S_OK;
}

HRESULT
HvParavirtConsole_Read(
    HV_PARAVIRT_CONSOLE* pConsole,
    CHAR8* Buffer,
    UINT32 MaxLength,
    UINT32* pActualLength
)
{
    UINT8* sharedBuf;
    UINT32 writePos;
    UINT32 readPos;
    UINT32 available;
    UINT32 length;
    UINT32 i;

    if (pConsole == NULL || Buffer == NULL || pActualLength == NULL) {
        return E_POINTER;
    }

    sharedBuf = (UINT8*)pConsole->SharedBuffer + 8;
    writePos = *pConsole->WriteOffset;
    readPos = *pConsole->ReadOffset;

    /* Calculate available data */
    if (writePos >= readPos) {
        available = writePos - readPos;
    } else {
        available = pConsole->BufferSize - readPos + writePos;
    }

    length = (available < MaxLength) ? available : MaxLength;

    /* Copy data from circular buffer */
    for (i = 0; i < length; i++) {
        Buffer[i] = (CHAR8)sharedBuf[readPos];
        readPos = (readPos + 1) % pConsole->BufferSize;
    }

    *pConsole->ReadOffset = readPos;
    *pActualLength = length;

    return S_OK;
}

HRESULT
HvParavirtConsole_Destroy(
    HV_PARAVIRT_CONSOLE* pConsole
)
{
    if (pConsole == NULL) {
        return E_POINTER;
    }

    if (pConsole->SharedBuffer != NULL) {
        RtlFreeMemory(NULL, pConsole->SharedBuffer);
    }

    RtlFreeMemory(NULL, pConsole);
    return S_OK;
}

/* ======================================================================= */
/* Paravirtual block device (virtio-blk style)                            */
/* ======================================================================= */

typedef struct HV_PARAVIRT_BLOCK {
    /* Virtqueue-like structure */
    VOID* DescriptorTable;
    VOID* AvailableRing;
    VOID* UsedRing;

    UINT32 QueueSize;
    UINT16 LastAvailIdx;
    UINT16 LastUsedIdx;

    /* Backing storage */
    IVirtualDevice* BackingDevice;
} HV_PARAVIRT_BLOCK;

/* Simplified virtio-blk request structure */
typedef struct HV_VIRTIO_BLK_REQ {
    UINT32 Type;      /* 0=read, 1=write, 4=flush */
    UINT32 Reserved;
    UINT64 Sector;    /* Starting sector */
    /* Followed by data buffer descriptors */
} HV_VIRTIO_BLK_REQ;

HRESULT
HvParavirtBlock_Create(
    IVirtualDevice* BackingDevice,
    UINT32 QueueSize,
    HV_PARAVIRT_BLOCK** ppBlock
)
{
    HV_PARAVIRT_BLOCK* pBlock;

    if (ppBlock == NULL) {
        return E_POINTER;
    }

    pBlock = (HV_PARAVIRT_BLOCK*)RtlAllocateMemory(NULL, sizeof(HV_PARAVIRT_BLOCK));
    if (pBlock == NULL) {
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pBlock, sizeof(HV_PARAVIRT_BLOCK));

    pBlock->QueueSize = QueueSize;
    pBlock->BackingDevice = BackingDevice;
    pBlock->LastAvailIdx = 0;
    pBlock->LastUsedIdx = 0;

    /* Allocate virtqueue structures */
    /* In a real implementation, these would be properly sized */
    pBlock->DescriptorTable = RtlAllocateMemory(NULL, QueueSize * 16);
    pBlock->AvailableRing = RtlAllocateMemory(NULL, 6 + QueueSize * 2);
    pBlock->UsedRing = RtlAllocateMemory(NULL, 6 + QueueSize * 8);

    if (pBlock->DescriptorTable == NULL ||
        pBlock->AvailableRing == NULL ||
        pBlock->UsedRing == NULL) {
        if (pBlock->DescriptorTable) RtlFreeMemory(NULL, pBlock->DescriptorTable);
        if (pBlock->AvailableRing) RtlFreeMemory(NULL, pBlock->AvailableRing);
        if (pBlock->UsedRing) RtlFreeMemory(NULL, pBlock->UsedRing);
        RtlFreeMemory(NULL, pBlock);
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pBlock->DescriptorTable, QueueSize * 16);
    RtlZeroMemory(pBlock->AvailableRing, 6 + QueueSize * 2);
    RtlZeroMemory(pBlock->UsedRing, 6 + QueueSize * 8);

    if (BackingDevice != NULL) {
        IVirtualDevice_AddRef(BackingDevice);
    }

    *ppBlock = pBlock;
    return S_OK;
}

HRESULT
HvParavirtBlock_Destroy(
    HV_PARAVIRT_BLOCK* pBlock
)
{
    if (pBlock == NULL) {
        return E_POINTER;
    }

    if (pBlock->DescriptorTable != NULL) {
        RtlFreeMemory(NULL, pBlock->DescriptorTable);
    }
    if (pBlock->AvailableRing != NULL) {
        RtlFreeMemory(NULL, pBlock->AvailableRing);
    }
    if (pBlock->UsedRing != NULL) {
        RtlFreeMemory(NULL, pBlock->UsedRing);
    }
    if (pBlock->BackingDevice != NULL) {
        IVirtualDevice_Release(pBlock->BackingDevice);
    }

    RtlFreeMemory(NULL, pBlock);
    return S_OK;
}
