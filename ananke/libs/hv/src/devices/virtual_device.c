/*++
    Module Name:

        virtual_device.c

    Abstract:

        Base virtual device implementation and device management.

--*/

#include "../hypervisor_impl.h"

/* ======================================================================= */
/* Generic virtual device base implementation                              */
/* ======================================================================= */

typedef struct HV_VIRTUAL_DEVICE_BASE {
    IVirtualDevice Interface;
    UINT32 RefCount;
    HV_DEVICE_TYPE Type;
    CHAR8 Name[64];
    IVirtualMachine* VM;
} HV_VIRTUAL_DEVICE_BASE;

/* ======================================================================= */
/* IVirtualDevice base implementation                                      */
/* ======================================================================= */

static HRESULT STDMETHODCALLTYPE
HvDevice_QueryInterface(
    IVirtualDevice* This,
    REFIID riid,
    VOID** ppvObject
)
{
    HV_VIRTUAL_DEVICE_BASE* pDevice = (HV_VIRTUAL_DEVICE_BASE*)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (RtlIsEqualGUID(riid, &IID_IUnknown) ||
        RtlIsEqualGUID(riid, &IID_IVirtualDevice)) {
        *ppvObject = &pDevice->Interface;
        AnxInterlockedIncrement((volatile INT32*)&pDevice->RefCount);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
HvDevice_AddRef(
    IVirtualDevice* This
)
{
    HV_VIRTUAL_DEVICE_BASE* pDevice = (HV_VIRTUAL_DEVICE_BASE*)This;
    return (UINT32)AnxInterlockedIncrement((volatile INT32*)&pDevice->RefCount);
}

static UINT32 STDMETHODCALLTYPE
HvDevice_Release(
    IVirtualDevice* This
)
{
    HV_VIRTUAL_DEVICE_BASE* pDevice = (HV_VIRTUAL_DEVICE_BASE*)This;
    UINT32 refCount = (UINT32)AnxInterlockedDecrement((volatile INT32*)&pDevice->RefCount);

    if (refCount == 0) {
        RtlFreeMemory(NULL, pDevice);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
HvDevice_GetDeviceType(
    IVirtualDevice* This,
    HV_DEVICE_TYPE* pType
)
{
    HV_VIRTUAL_DEVICE_BASE* pDevice = (HV_VIRTUAL_DEVICE_BASE*)This;

    if (pType == NULL) {
        return E_POINTER;
    }

    *pType = pDevice->Type;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvDevice_GetDeviceName(
    IVirtualDevice* This,
    CHAR8* Buffer,
    UINT32* pSize
)
{
    HV_VIRTUAL_DEVICE_BASE* pDevice = (HV_VIRTUAL_DEVICE_BASE*)This;
    UINT32 nameLen;

    if (pSize == NULL) {
        return E_POINTER;
    }

    nameLen = (UINT32)RtlStringLength((const char*)pDevice->Name);

    if (Buffer == NULL || *pSize < nameLen + 1) {
        *pSize = nameLen + 1;
        return HV_ERROR;
    }

    RtlCopyMemory(Buffer, pDevice->Name, nameLen + 1);
    *pSize = nameLen;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvDevice_Initialize(
    IVirtualDevice* This,
    IVirtualMachine* pVM
)
{
    HV_VIRTUAL_DEVICE_BASE* pDevice = (HV_VIRTUAL_DEVICE_BASE*)This;
    pDevice->VM = pVM;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvDevice_Shutdown(
    IVirtualDevice* This
)
{
    (VOID)This;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvDevice_Reset(
    IVirtualDevice* This
)
{
    (VOID)This;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvDevice_IORead(
    IVirtualDevice* This,
    UINT64 Port,
    UINT32 Size,
    UINT64* pValue
)
{
    (VOID)This;
    (VOID)Port;
    (VOID)Size;

    if (pValue == NULL) {
        return E_POINTER;
    }

    *pValue = 0xFFFFFFFF;  /* Default: all bits set */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvDevice_IOWrite(
    IVirtualDevice* This,
    UINT64 Port,
    UINT32 Size,
    UINT64 Value
)
{
    (VOID)This;
    (VOID)Port;
    (VOID)Size;
    (VOID)Value;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvDevice_MemoryRead(
    IVirtualDevice* This,
    UINT64 Offset,
    VOID* Buffer,
    UINT64 Size
)
{
    (VOID)This;
    (VOID)Offset;

    if (Buffer == NULL) {
        return E_POINTER;
    }

    RtlZeroMemory(Buffer, (SIZE_T)Size);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvDevice_MemoryWrite(
    IVirtualDevice* This,
    UINT64 Offset,
    CONST VOID* Buffer,
    UINT64 Size
)
{
    (VOID)This;
    (VOID)Offset;
    (VOID)Buffer;
    (VOID)Size;
    return S_OK;
}

/* Base device vtable */
static IVirtualDeviceVtbl g_VirtualDeviceVtbl = {
    HvDevice_QueryInterface,
    HvDevice_AddRef,
    HvDevice_Release,
    HvDevice_GetDeviceType,
    HvDevice_GetDeviceName,
    HvDevice_Initialize,
    HvDevice_Shutdown,
    HvDevice_Reset,
    HvDevice_IORead,
    HvDevice_IOWrite,
    HvDevice_MemoryRead,
    HvDevice_MemoryWrite
};

/* ======================================================================= */
/* Device creation helper                                                  */
/* ======================================================================= */

HRESULT
HvCreateVirtualDevice(
    HV_DEVICE_TYPE Type,
    CONST CHAR8* Name,
    IVirtualDevice** ppDevice
)
{
    HV_VIRTUAL_DEVICE_BASE* pDevice;

    if (ppDevice == NULL) {
        return E_POINTER;
    }

    pDevice = (HV_VIRTUAL_DEVICE_BASE*)RtlAllocateMemory(NULL, sizeof(HV_VIRTUAL_DEVICE_BASE));
    if (pDevice == NULL) {
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pDevice, sizeof(HV_VIRTUAL_DEVICE_BASE));

    pDevice->Interface.lpVtbl = &g_VirtualDeviceVtbl;
    pDevice->RefCount = 1;
    pDevice->Type = Type;

    if (Name != NULL) {
        RtlStringCopy((char*)pDevice->Name, sizeof(pDevice->Name), (const char*)Name);
    }

    *ppDevice = (IVirtualDevice*)pDevice;
    return S_OK;
}
