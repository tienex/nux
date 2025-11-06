/*++
    Module Name:

        virtual_network.c

    Abstract:

        Virtual network device implementation (NE2000/RTL8139/virtio-net style).

--*/

#include "../hypervisor_impl.h"

/* Virtual network device structure */
typedef struct HV_VIRTUAL_NETWORK {
    IVirtualDevice Interface;
    UINT32 RefCount;
    IVirtualMachine* VM;

    /* Network properties */
    UINT8 MacAddress[6];
    BOOLEAN LinkUp;
    UINT32 Speed;  /* Mbps */

    /* Packet queues */
    struct {
        UINT8* Data;
        UINT32 Length;
    } RxQueue[16];
    UINT32 RxHead;
    UINT32 RxTail;

    struct {
        UINT8* Data;
        UINT32 Length;
    } TxQueue[16];
    UINT32 TxHead;
    UINT32 TxTail;

    /* NE2000-style registers */
    UINT8 Command;
    UINT8 PageStart;
    UINT8 PageStop;
    UINT8 Boundary;
    UINT8 TransmitStatus;
    UINT8 ReceiveStatus;
    UINT16 RemoteByteCount;
    UINT16 LocalDMA;
} HV_VIRTUAL_NETWORK;

/* NE2000 Commands */
#define NE2K_CMD_STOP           0x01
#define NE2K_CMD_START          0x02
#define NE2K_CMD_TRANSMIT       0x04
#define NE2K_CMD_REMOTE_READ    0x08
#define NE2K_CMD_REMOTE_WRITE   0x10

/* NE2000 Registers */
#define NE2K_REG_COMMAND        0x00
#define NE2K_REG_PAGE_START     0x01
#define NE2K_REG_PAGE_STOP      0x02
#define NE2K_REG_BOUNDARY       0x03
#define NE2K_REG_TX_STATUS      0x04
#define NE2K_REG_TX_PAGE        0x04
#define NE2K_REG_TX_BYTE_COUNT0 0x05
#define NE2K_REG_TX_BYTE_COUNT1 0x06
#define NE2K_REG_RX_STATUS      0x0C
#define NE2K_REG_DATA           0x10

/* ======================================================================= */
/* IVirtualDevice implementation for network                               */
/* ======================================================================= */

static HRESULT STDMETHODCALLTYPE
HvNetwork_QueryInterface(
    IVirtualDevice* This,
    REFIID riid,
    VOID** ppvObject
)
{
    HV_VIRTUAL_NETWORK* pNet = (HV_VIRTUAL_NETWORK*)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (RtlIsEqualGUID(riid, &IID_IUnknown) ||
        RtlIsEqualGUID(riid, &IID_IVirtualDevice)) {
        *ppvObject = &pNet->Interface;
        AnxInterlockedIncrement((volatile INT32*)&pNet->RefCount);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
HvNetwork_AddRef(
    IVirtualDevice* This
)
{
    HV_VIRTUAL_NETWORK* pNet = (HV_VIRTUAL_NETWORK*)This;
    return (UINT32)AnxInterlockedIncrement((volatile INT32*)&pNet->RefCount);
}

static UINT32 STDMETHODCALLTYPE
HvNetwork_Release(
    IVirtualDevice* This
)
{
    HV_VIRTUAL_NETWORK* pNet = (HV_VIRTUAL_NETWORK*)This;
    UINT32 refCount = (UINT32)AnxInterlockedDecrement((volatile INT32*)&pNet->RefCount);
    UINT32 i;

    if (refCount == 0) {
        /* Free packet queues */
        for (i = 0; i < 16; i++) {
            if (pNet->RxQueue[i].Data != NULL) {
                RtlFreeMemory(NULL, pNet->RxQueue[i].Data);
            }
            if (pNet->TxQueue[i].Data != NULL) {
                RtlFreeMemory(NULL, pNet->TxQueue[i].Data);
            }
        }
        RtlFreeMemory(NULL, pNet);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
HvNetwork_GetDeviceType(
    IVirtualDevice* This,
    HV_DEVICE_TYPE* pType
)
{
    (VOID)This;
    if (pType == NULL) {
        return E_POINTER;
    }
    *pType = HV_DEVICE_NETWORK;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvNetwork_GetDeviceName(
    IVirtualDevice* This,
    CHAR8* Buffer,
    UINT32* pSize
)
{
    (VOID)This;
    CONST CHAR8* name = "VirtualNetwork";
    UINT32 nameLen = 14;

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
HvNetwork_Initialize(
    IVirtualDevice* This,
    IVirtualMachine* pVM
)
{
    HV_VIRTUAL_NETWORK* pNet = (HV_VIRTUAL_NETWORK*)This;
    pNet->VM = pVM;
    pNet->LinkUp = TRUE;
    pNet->Command = NE2K_CMD_STOP;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvNetwork_Shutdown(
    IVirtualDevice* This
)
{
    (VOID)This;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvNetwork_Reset(
    IVirtualDevice* This
)
{
    HV_VIRTUAL_NETWORK* pNet = (HV_VIRTUAL_NETWORK*)This;
    pNet->Command = NE2K_CMD_STOP;
    pNet->RxHead = 0;
    pNet->RxTail = 0;
    pNet->TxHead = 0;
    pNet->TxTail = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvNetwork_IORead(
    IVirtualDevice* This,
    UINT64 Port,
    UINT32 Size,
    UINT64* pValue
)
{
    HV_VIRTUAL_NETWORK* pNet = (HV_VIRTUAL_NETWORK*)This;

    if (pValue == NULL) {
        return E_POINTER;
    }

    (VOID)Size;

    switch (Port & 0xFF) {
        case NE2K_REG_COMMAND:
            *pValue = pNet->Command;
            break;

        case NE2K_REG_PAGE_START:
            *pValue = pNet->PageStart;
            break;

        case NE2K_REG_PAGE_STOP:
            *pValue = pNet->PageStop;
            break;

        case NE2K_REG_BOUNDARY:
            *pValue = pNet->Boundary;
            break;

        case NE2K_REG_TX_STATUS:
            *pValue = pNet->TransmitStatus;
            break;

        case NE2K_REG_RX_STATUS:
            *pValue = pNet->ReceiveStatus;
            break;

        case NE2K_REG_DATA:
            /* Read data from RX queue */
            if (pNet->RxHead != pNet->RxTail) {
                UINT32 idx = pNet->RxHead % 16;
                if (pNet->RxQueue[idx].Data != NULL && pNet->LocalDMA < pNet->RxQueue[idx].Length) {
                    *pValue = pNet->RxQueue[idx].Data[pNet->LocalDMA++];
                } else {
                    *pValue = 0;
                }
            } else {
                *pValue = 0;
            }
            break;

        default:
            *pValue = 0xFF;
            break;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvNetwork_IOWrite(
    IVirtualDevice* This,
    UINT64 Port,
    UINT32 Size,
    UINT64 Value
)
{
    HV_VIRTUAL_NETWORK* pNet = (HV_VIRTUAL_NETWORK*)This;

    (VOID)Size;

    switch (Port & 0xFF) {
        case NE2K_REG_COMMAND:
            pNet->Command = (UINT8)Value;
            if (Value & NE2K_CMD_START) {
                pNet->LinkUp = TRUE;
            }
            if (Value & NE2K_CMD_STOP) {
                pNet->LinkUp = FALSE;
            }
            if (Value & NE2K_CMD_TRANSMIT) {
                /* Transmit packet from TX queue */
                if (pNet->TxHead != pNet->TxTail) {
                    /* Packet would be sent here */
                    pNet->TransmitStatus = 0x01;  /* Success */
                    pNet->TxHead = (pNet->TxHead + 1) % 16;
                }
            }
            break;

        case NE2K_REG_PAGE_START:
            pNet->PageStart = (UINT8)Value;
            break;

        case NE2K_REG_PAGE_STOP:
            pNet->PageStop = (UINT8)Value;
            break;

        case NE2K_REG_BOUNDARY:
            pNet->Boundary = (UINT8)Value;
            break;

        case NE2K_REG_TX_PAGE:
            /* TX page set */
            break;

        case NE2K_REG_TX_BYTE_COUNT0:
            pNet->RemoteByteCount = (pNet->RemoteByteCount & 0xFF00) | (UINT8)Value;
            break;

        case NE2K_REG_TX_BYTE_COUNT1:
            pNet->RemoteByteCount = (pNet->RemoteByteCount & 0x00FF) | ((UINT8)Value << 8);
            break;

        case NE2K_REG_DATA:
            /* Write data to TX queue */
            if (pNet->TxTail < 16) {
                UINT32 idx = pNet->TxTail;
                if (pNet->TxQueue[idx].Data == NULL) {
                    pNet->TxQueue[idx].Data = (UINT8*)RtlAllocateMemory(NULL, 1518);  /* Max Ethernet frame */
                    pNet->TxQueue[idx].Length = 0;
                }
                if (pNet->TxQueue[idx].Data != NULL && pNet->TxQueue[idx].Length < 1518) {
                    pNet->TxQueue[idx].Data[pNet->TxQueue[idx].Length++] = (UINT8)Value;
                }
            }
            break;

        default:
            break;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvNetwork_MemoryRead(
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
HvNetwork_MemoryWrite(
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

/* Virtual network vtable */
static IVirtualDeviceVtbl g_VirtualNetworkVtbl = {
    HvNetwork_QueryInterface,
    HvNetwork_AddRef,
    HvNetwork_Release,
    HvNetwork_GetDeviceType,
    HvNetwork_GetDeviceName,
    HvNetwork_Initialize,
    HvNetwork_Shutdown,
    HvNetwork_Reset,
    HvNetwork_IORead,
    HvNetwork_IOWrite,
    HvNetwork_MemoryRead,
    HvNetwork_MemoryWrite
};

/* ======================================================================= */
/* Virtual network creation                                                */
/* ======================================================================= */

HRESULT
HvCreateVirtualNetwork(
    CONST UINT8 MacAddress[6],
    IVirtualDevice** ppDevice
)
{
    HV_VIRTUAL_NETWORK* pNet;

    if (ppDevice == NULL) {
        return E_POINTER;
    }

    pNet = (HV_VIRTUAL_NETWORK*)RtlAllocateMemory(NULL, sizeof(HV_VIRTUAL_NETWORK));
    if (pNet == NULL) {
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pNet, sizeof(HV_VIRTUAL_NETWORK));

    /* Initialize */
    pNet->Interface.lpVtbl = &g_VirtualNetworkVtbl;
    pNet->RefCount = 1;
    pNet->LinkUp = FALSE;
    pNet->Speed = 1000;  /* 1 Gbps */

    /* Set MAC address */
    if (MacAddress != NULL) {
        RtlCopyMemory(pNet->MacAddress, MacAddress, 6);
    } else {
        /* Generate default MAC: 52:54:00:xx:xx:xx (QEMU range) */
        pNet->MacAddress[0] = 0x52;
        pNet->MacAddress[1] = 0x54;
        pNet->MacAddress[2] = 0x00;
        pNet->MacAddress[3] = 0x12;
        pNet->MacAddress[4] = 0x34;
        pNet->MacAddress[5] = 0x56;
    }

    *ppDevice = (IVirtualDevice*)pNet;
    return S_OK;
}
