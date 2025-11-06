/*++
    Module Name:

        virtual_disk.c

    Abstract:

        Virtual disk device implementation (IDE/SATA/virtio-blk style).

--*/

#include "../hypervisor_impl.h"

/* Virtual disk device structure */
typedef struct HV_VIRTUAL_DISK {
    IVirtualDevice Interface;
    UINT32 RefCount;
    IVirtualMachine* VM;

    /* Disk properties */
    UINT64 SizeBytes;
    UINT32 SectorSize;
    UINT64 SectorCount;
    BOOLEAN ReadOnly;

    /* Backing storage */
    VOID* ImageBuffer;  /* In-memory image */
    CHAR8 ImagePath[256];  /* File path (if backed by file) */

    /* Controller state */
    UINT32 Command;
    UINT32 Status;
    UINT64 LBA;
    UINT32 SectorCountReg;
    UINT8* DataBuffer;
    UINT32 DataBufferSize;
    UINT32 DataOffset;
} HV_VIRTUAL_DISK;

/* IDE/ATA Commands */
#define ATA_CMD_READ_SECTORS        0x20
#define ATA_CMD_WRITE_SECTORS       0x30
#define ATA_CMD_IDENTIFY            0xEC
#define ATA_CMD_SET_FEATURES        0xEF

/* IDE/ATA Status */
#define ATA_STATUS_BUSY             0x80
#define ATA_STATUS_READY            0x40
#define ATA_STATUS_DRQ              0x08
#define ATA_STATUS_ERROR            0x01

/* IDE I/O Ports */
#define IDE_PORT_DATA               0x1F0
#define IDE_PORT_ERROR              0x1F1
#define IDE_PORT_SECTOR_COUNT       0x1F2
#define IDE_PORT_LBA_LOW            0x1F3
#define IDE_PORT_LBA_MID            0x1F4
#define IDE_PORT_LBA_HIGH           0x1F5
#define IDE_PORT_DEVICE             0x1F6
#define IDE_PORT_COMMAND            0x1F7
#define IDE_PORT_STATUS             0x1F7

/* ======================================================================= */
/* IVirtualDevice implementation for disk                                  */
/* ======================================================================= */

static HRESULT STDMETHODCALLTYPE
HvDisk_QueryInterface(
    IVirtualDevice* This,
    REFIID riid,
    VOID** ppvObject
)
{
    HV_VIRTUAL_DISK* pDisk = (HV_VIRTUAL_DISK*)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (RtlIsEqualGUID(riid, &IID_IUnknown) ||
        RtlIsEqualGUID(riid, &IID_IVirtualDevice)) {
        *ppvObject = &pDisk->Interface;
        AnxInterlockedIncrement((volatile INT32*)&pDisk->RefCount);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
HvDisk_AddRef(
    IVirtualDevice* This
)
{
    HV_VIRTUAL_DISK* pDisk = (HV_VIRTUAL_DISK*)This;
    return (UINT32)AnxInterlockedIncrement((volatile INT32*)&pDisk->RefCount);
}

static UINT32 STDMETHODCALLTYPE
HvDisk_Release(
    IVirtualDevice* This
)
{
    HV_VIRTUAL_DISK* pDisk = (HV_VIRTUAL_DISK*)This;
    UINT32 refCount = (UINT32)AnxInterlockedDecrement((volatile INT32*)&pDisk->RefCount);

    if (refCount == 0) {
        if (pDisk->ImageBuffer != NULL) {
            RtlFreeMemory(NULL, pDisk->ImageBuffer);
        }
        if (pDisk->DataBuffer != NULL) {
            RtlFreeMemory(NULL, pDisk->DataBuffer);
        }
        RtlFreeMemory(NULL, pDisk);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
HvDisk_GetDeviceType(
    IVirtualDevice* This,
    HV_DEVICE_TYPE* pType
)
{
    (VOID)This;
    if (pType == NULL) {
        return E_POINTER;
    }
    *pType = HV_DEVICE_DISK;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvDisk_GetDeviceName(
    IVirtualDevice* This,
    CHAR8* Buffer,
    UINT32* pSize
)
{
    (VOID)This;
    CONST CHAR8* name = "VirtualDisk";
    UINT32 nameLen = 11;

    if (pSize == NULL) {
        return E_POINTER;
    }

    if (Buffer == NULL || *pSize < nameLen + 1) {
        *pSize = nameLen + 1;
        return HV_ERROR;
    }

    RtlCopyMemory(Buffer, name, nameLen + 1);
    *pSize = nameLen;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvDisk_Initialize(
    IVirtualDevice* This,
    IVirtualMachine* pVM
)
{
    HV_VIRTUAL_DISK* pDisk = (HV_VIRTUAL_DISK*)This;
    pDisk->VM = pVM;
    pDisk->Status = ATA_STATUS_READY;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvDisk_Shutdown(
    IVirtualDevice* This
)
{
    (VOID)This;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvDisk_Reset(
    IVirtualDevice* This
)
{
    HV_VIRTUAL_DISK* pDisk = (HV_VIRTUAL_DISK*)This;
    pDisk->Status = ATA_STATUS_READY;
    pDisk->Command = 0;
    pDisk->LBA = 0;
    pDisk->SectorCountReg = 0;
    pDisk->DataOffset = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvDisk_IORead(
    IVirtualDevice* This,
    UINT64 Port,
    UINT32 Size,
    UINT64* pValue
)
{
    HV_VIRTUAL_DISK* pDisk = (HV_VIRTUAL_DISK*)This;

    if (pValue == NULL) {
        return E_POINTER;
    }

    switch (Port) {
        case IDE_PORT_DATA:
            /* Read data from buffer */
            if (pDisk->DataBuffer != NULL && pDisk->DataOffset < pDisk->DataBufferSize) {
                if (Size == 2) {
                    *pValue = *(UINT16*)(pDisk->DataBuffer + pDisk->DataOffset);
                    pDisk->DataOffset += 2;
                } else if (Size == 4) {
                    *pValue = *(UINT32*)(pDisk->DataBuffer + pDisk->DataOffset);
                    pDisk->DataOffset += 4;
                } else {
                    *pValue = pDisk->DataBuffer[pDisk->DataOffset++];
                }
            } else {
                *pValue = 0;
            }
            break;

        case IDE_PORT_STATUS:
            *pValue = pDisk->Status;
            break;

        case IDE_PORT_ERROR:
            *pValue = 0;  /* No errors */
            break;

        case IDE_PORT_SECTOR_COUNT:
            *pValue = pDisk->SectorCountReg;
            break;

        case IDE_PORT_LBA_LOW:
            *pValue = (pDisk->LBA >> 0) & 0xFF;
            break;

        case IDE_PORT_LBA_MID:
            *pValue = (pDisk->LBA >> 8) & 0xFF;
            break;

        case IDE_PORT_LBA_HIGH:
            *pValue = (pDisk->LBA >> 16) & 0xFF;
            break;

        case IDE_PORT_DEVICE:
            *pValue = (pDisk->LBA >> 24) & 0x0F;
            break;

        default:
            *pValue = 0xFF;
            break;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvDisk_IOWrite(
    IVirtualDevice* This,
    UINT64 Port,
    UINT32 Size,
    UINT64 Value
)
{
    HV_VIRTUAL_DISK* pDisk = (HV_VIRTUAL_DISK*)This;
    UINT64 offset;

    (VOID)Size;

    switch (Port) {
        case IDE_PORT_DATA:
            /* Write data to buffer */
            if (pDisk->DataBuffer != NULL && pDisk->DataOffset < pDisk->DataBufferSize) {
                pDisk->DataBuffer[pDisk->DataOffset++] = (UINT8)Value;
            }
            break;

        case IDE_PORT_SECTOR_COUNT:
            pDisk->SectorCountReg = (UINT8)Value;
            break;

        case IDE_PORT_LBA_LOW:
            pDisk->LBA = (pDisk->LBA & 0xFFFFFF00) | (Value & 0xFF);
            break;

        case IDE_PORT_LBA_MID:
            pDisk->LBA = (pDisk->LBA & 0xFFFF00FF) | ((Value & 0xFF) << 8);
            break;

        case IDE_PORT_LBA_HIGH:
            pDisk->LBA = (pDisk->LBA & 0xFF00FFFF) | ((Value & 0xFF) << 16);
            break;

        case IDE_PORT_DEVICE:
            pDisk->LBA = (pDisk->LBA & 0x00FFFFFF) | ((Value & 0x0F) << 24);
            break;

        case IDE_PORT_COMMAND:
            pDisk->Command = (UINT8)Value;
            pDisk->Status = ATA_STATUS_BUSY;

            switch (pDisk->Command) {
                case ATA_CMD_READ_SECTORS:
                    /* Read sectors from disk */
                    if (pDisk->ImageBuffer != NULL) {
                        offset = pDisk->LBA * pDisk->SectorSize;
                        if (offset + pDisk->SectorSize <= pDisk->SizeBytes) {
                            pDisk->DataBuffer = (UINT8*)pDisk->ImageBuffer + offset;
                            pDisk->DataBufferSize = pDisk->SectorSize * pDisk->SectorCountReg;
                            pDisk->DataOffset = 0;
                            pDisk->Status = ATA_STATUS_READY | ATA_STATUS_DRQ;
                        } else {
                            pDisk->Status = ATA_STATUS_READY | ATA_STATUS_ERROR;
                        }
                    }
                    break;

                case ATA_CMD_WRITE_SECTORS:
                    /* Write sectors to disk */
                    if (!pDisk->ReadOnly && pDisk->ImageBuffer != NULL) {
                        offset = pDisk->LBA * pDisk->SectorSize;
                        if (offset + pDisk->SectorSize <= pDisk->SizeBytes) {
                            pDisk->DataBuffer = (UINT8*)pDisk->ImageBuffer + offset;
                            pDisk->DataBufferSize = pDisk->SectorSize * pDisk->SectorCountReg;
                            pDisk->DataOffset = 0;
                            pDisk->Status = ATA_STATUS_READY | ATA_STATUS_DRQ;
                        } else {
                            pDisk->Status = ATA_STATUS_READY | ATA_STATUS_ERROR;
                        }
                    } else {
                        pDisk->Status = ATA_STATUS_READY | ATA_STATUS_ERROR;
                    }
                    break;

                case ATA_CMD_IDENTIFY:
                    /* Return identify data */
                    /* TODO: Build proper IDENTIFY structure */
                    pDisk->Status = ATA_STATUS_READY;
                    break;

                default:
                    pDisk->Status = ATA_STATUS_READY;
                    break;
            }
            break;

        default:
            break;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvDisk_MemoryRead(
    IVirtualDevice* This,
    UINT64 Offset,
    VOID* Buffer,
    UINT64 Size
)
{
    HV_VIRTUAL_DISK* pDisk = (HV_VIRTUAL_DISK*)This;

    if (Buffer == NULL) {
        return E_POINTER;
    }

    if (Offset + Size > pDisk->SizeBytes) {
        return HV_ERROR;
    }

    if (pDisk->ImageBuffer != NULL) {
        RtlCopyMemory(Buffer, (UINT8*)pDisk->ImageBuffer + Offset, (SIZE_T)Size);
    } else {
        RtlZeroMemory(Buffer, (SIZE_T)Size);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvDisk_MemoryWrite(
    IVirtualDevice* This,
    UINT64 Offset,
    CONST VOID* Buffer,
    UINT64 Size
)
{
    HV_VIRTUAL_DISK* pDisk = (HV_VIRTUAL_DISK*)This;

    if (Buffer == NULL) {
        return E_POINTER;
    }

    if (pDisk->ReadOnly) {
        return HV_DENIED;
    }

    if (Offset + Size > pDisk->SizeBytes) {
        return HV_ERROR;
    }

    if (pDisk->ImageBuffer != NULL) {
        RtlCopyMemory((UINT8*)pDisk->ImageBuffer + Offset, Buffer, (SIZE_T)Size);
    }

    return S_OK;
}

/* Virtual disk vtable */
static IVirtualDeviceVtbl g_VirtualDiskVtbl = {
    HvDisk_QueryInterface,
    HvDisk_AddRef,
    HvDisk_Release,
    HvDisk_GetDeviceType,
    HvDisk_GetDeviceName,
    HvDisk_Initialize,
    HvDisk_Shutdown,
    HvDisk_Reset,
    HvDisk_IORead,
    HvDisk_IOWrite,
    HvDisk_MemoryRead,
    HvDisk_MemoryWrite
};

/* ======================================================================= */
/* Virtual disk creation                                                   */
/* ======================================================================= */

HRESULT
HvCreateVirtualDisk(
    UINT64 SizeBytes,
    BOOLEAN ReadOnly,
    IVirtualDevice** ppDevice
)
{
    HV_VIRTUAL_DISK* pDisk;

    if (ppDevice == NULL) {
        return E_POINTER;
    }

    pDisk = (HV_VIRTUAL_DISK*)RtlAllocateMemory(NULL, sizeof(HV_VIRTUAL_DISK));
    if (pDisk == NULL) {
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pDisk, sizeof(HV_VIRTUAL_DISK));

    /* Initialize */
    pDisk->Interface.lpVtbl = &g_VirtualDiskVtbl;
    pDisk->RefCount = 1;
    pDisk->SizeBytes = SizeBytes;
    pDisk->SectorSize = 512;
    pDisk->SectorCount = SizeBytes / 512;
    pDisk->ReadOnly = ReadOnly;
    pDisk->Status = ATA_STATUS_READY;

    /* Allocate image buffer */
    pDisk->ImageBuffer = RtlAllocateMemory(NULL, (SIZE_T)SizeBytes);
    if (pDisk->ImageBuffer == NULL) {
        RtlFreeMemory(NULL, pDisk);
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pDisk->ImageBuffer, (SIZE_T)SizeBytes);

    *ppDevice = (IVirtualDevice*)pDisk;
    return S_OK;
}
