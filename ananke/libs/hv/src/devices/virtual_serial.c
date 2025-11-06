/*++
    Module Name:

        virtual_serial.c

    Abstract:

        Virtual serial port device implementation (16550 UART style).

--*/

#include "../hypervisor_impl.h"

/* Virtual serial device structure */
typedef struct HV_VIRTUAL_SERIAL {
    IVirtualDevice Interface;
    UINT32 RefCount;
    IVirtualMachine* VM;

    /* 16550 UART registers */
    UINT8 RBR;  /* Receiver Buffer Register */
    UINT8 THR;  /* Transmitter Holding Register */
    UINT8 IER;  /* Interrupt Enable Register */
    UINT8 IIR;  /* Interrupt Identification Register */
    UINT8 FCR;  /* FIFO Control Register */
    UINT8 LCR;  /* Line Control Register */
    UINT8 MCR;  /* Modem Control Register */
    UINT8 LSR;  /* Line Status Register */
    UINT8 MSR;  /* Modem Status Register */
    UINT8 SCR;  /* Scratch Register */
    UINT16 DLL; /* Divisor Latch Low */
    UINT16 DLH; /* Divisor Latch High */

    /* FIFOs */
    UINT8 RxFIFO[16];
    UINT32 RxHead;
    UINT32 RxTail;

    UINT8 TxFIFO[16];
    UINT32 TxHead;
    UINT32 TxTail;

    /* Output callback (for connecting to console/file/etc) */
    VOID (*OutputCallback)(VOID* Context, UINT8 Data);
    VOID* OutputContext;
} HV_VIRTUAL_SERIAL;

/* 16550 UART Register Offsets */
#define UART_RBR    0  /* Receiver Buffer Register (read) */
#define UART_THR    0  /* Transmitter Holding Register (write) */
#define UART_IER    1  /* Interrupt Enable Register */
#define UART_IIR    2  /* Interrupt Identification Register (read) */
#define UART_FCR    2  /* FIFO Control Register (write) */
#define UART_LCR    3  /* Line Control Register */
#define UART_MCR    4  /* Modem Control Register */
#define UART_LSR    5  /* Line Status Register */
#define UART_MSR    6  /* Modem Status Register */
#define UART_SCR    7  /* Scratch Register */

/* Line Status Register bits */
#define LSR_DATA_READY      0x01
#define LSR_OVERRUN_ERROR   0x02
#define LSR_PARITY_ERROR    0x04
#define LSR_FRAMING_ERROR   0x08
#define LSR_BREAK_INTERRUPT 0x10
#define LSR_THR_EMPTY       0x20
#define LSR_THR_IDLE        0x40
#define LSR_FIFO_ERROR      0x80

/* ======================================================================= */
/* IVirtualDevice implementation for serial                                */
/* ======================================================================= */

static HRESULT STDMETHODCALLTYPE
HvSerial_QueryInterface(
    IVirtualDevice* This,
    REFIID riid,
    VOID** ppvObject
)
{
    HV_VIRTUAL_SERIAL* pSerial = (HV_VIRTUAL_SERIAL*)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (RtlIsEqualGUID(riid, &IID_IUnknown) ||
        RtlIsEqualGUID(riid, &IID_IVirtualDevice)) {
        *ppvObject = &pSerial->Interface;
        AnxInterlockedIncrement((volatile INT32*)&pSerial->RefCount);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
HvSerial_AddRef(
    IVirtualDevice* This
)
{
    HV_VIRTUAL_SERIAL* pSerial = (HV_VIRTUAL_SERIAL*)This;
    return (UINT32)AnxInterlockedIncrement((volatile INT32*)&pSerial->RefCount);
}

static UINT32 STDMETHODCALLTYPE
HvSerial_Release(
    IVirtualDevice* This
)
{
    HV_VIRTUAL_SERIAL* pSerial = (HV_VIRTUAL_SERIAL*)This;
    UINT32 refCount = (UINT32)AnxInterlockedDecrement((volatile INT32*)&pSerial->RefCount);

    if (refCount == 0) {
        RtlFreeMemory(NULL, pSerial);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
HvSerial_GetDeviceType(
    IVirtualDevice* This,
    HV_DEVICE_TYPE* pType
)
{
    (VOID)This;
    if (pType == NULL) {
        return E_POINTER;
    }
    *pType = HV_DEVICE_SERIAL;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvSerial_GetDeviceName(
    IVirtualDevice* This,
    CHAR8* Buffer,
    UINT32* pSize
)
{
    (VOID)This;
    CONST CHAR8* name = "VirtualSerial";
    UINT32 nameLen = 13;

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
HvSerial_Initialize(
    IVirtualDevice* This,
    IVirtualMachine* pVM
)
{
    HV_VIRTUAL_SERIAL* pSerial = (HV_VIRTUAL_SERIAL*)This;
    pSerial->VM = pVM;
    pSerial->LSR = LSR_THR_EMPTY | LSR_THR_IDLE;  /* Transmitter ready */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvSerial_Shutdown(
    IVirtualDevice* This
)
{
    (VOID)This;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvSerial_Reset(
    IVirtualDevice* This
)
{
    HV_VIRTUAL_SERIAL* pSerial = (HV_VIRTUAL_SERIAL*)This;

    pSerial->IER = 0;
    pSerial->IIR = 1;  /* No interrupt pending */
    pSerial->FCR = 0;
    pSerial->LCR = 0;
    pSerial->MCR = 0;
    pSerial->LSR = LSR_THR_EMPTY | LSR_THR_IDLE;
    pSerial->MSR = 0;
    pSerial->SCR = 0;
    pSerial->RxHead = 0;
    pSerial->RxTail = 0;
    pSerial->TxHead = 0;
    pSerial->TxTail = 0;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvSerial_IORead(
    IVirtualDevice* This,
    UINT64 Port,
    UINT32 Size,
    UINT64* pValue
)
{
    HV_VIRTUAL_SERIAL* pSerial = (HV_VIRTUAL_SERIAL*)This;
    UINT32 reg;

    if (pValue == NULL) {
        return E_POINTER;
    }

    (VOID)Size;
    reg = (UINT32)(Port & 0x7);

    switch (reg) {
        case UART_RBR:
            /* Read from RX FIFO */
            if (pSerial->RxHead != pSerial->RxTail) {
                *pValue = pSerial->RxFIFO[pSerial->RxHead];
                pSerial->RxHead = (pSerial->RxHead + 1) % 16;

                /* Update LSR */
                if (pSerial->RxHead == pSerial->RxTail) {
                    pSerial->LSR &= ~LSR_DATA_READY;
                }
            } else {
                *pValue = 0;
            }
            break;

        case UART_IER:
            *pValue = pSerial->IER;
            break;

        case UART_IIR:
            *pValue = pSerial->IIR;
            break;

        case UART_LCR:
            *pValue = pSerial->LCR;
            break;

        case UART_MCR:
            *pValue = pSerial->MCR;
            break;

        case UART_LSR:
            *pValue = pSerial->LSR;
            break;

        case UART_MSR:
            *pValue = pSerial->MSR;
            break;

        case UART_SCR:
            *pValue = pSerial->SCR;
            break;

        default:
            *pValue = 0xFF;
            break;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvSerial_IOWrite(
    IVirtualDevice* This,
    UINT64 Port,
    UINT32 Size,
    UINT64 Value
)
{
    HV_VIRTUAL_SERIAL* pSerial = (HV_VIRTUAL_SERIAL*)This;
    UINT32 reg;
    UINT32 nextTail;

    (VOID)Size;
    reg = (UINT32)(Port & 0x7);

    switch (reg) {
        case UART_THR:
            /* DLAB = 0: Write to transmit holding register */
            if (!(pSerial->LCR & 0x80)) {
                pSerial->THR = (UINT8)Value;

                /* Add to TX FIFO */
                nextTail = (pSerial->TxTail + 1) % 16;
                if (nextTail != pSerial->TxHead) {
                    pSerial->TxFIFO[pSerial->TxTail] = (UINT8)Value;
                    pSerial->TxTail = nextTail;
                }

                /* Call output callback if set */
                if (pSerial->OutputCallback != NULL) {
                    pSerial->OutputCallback(pSerial->OutputContext, (UINT8)Value);
                }

                /* Update LSR - transmitter is always ready */
                pSerial->LSR |= LSR_THR_EMPTY | LSR_THR_IDLE;
            } else {
                /* DLAB = 1: Write to divisor latch low */
                pSerial->DLL = (UINT8)Value;
            }
            break;

        case UART_IER:
            /* DLAB = 0: Interrupt Enable Register */
            if (!(pSerial->LCR & 0x80)) {
                pSerial->IER = (UINT8)Value;
            } else {
                /* DLAB = 1: Write to divisor latch high */
                pSerial->DLH = (UINT8)Value;
            }
            break;

        case UART_FCR:
            pSerial->FCR = (UINT8)Value;

            /* Reset FIFOs if requested */
            if (Value & 0x02) {
                pSerial->RxHead = 0;
                pSerial->RxTail = 0;
            }
            if (Value & 0x04) {
                pSerial->TxHead = 0;
                pSerial->TxTail = 0;
            }
            break;

        case UART_LCR:
            pSerial->LCR = (UINT8)Value;
            break;

        case UART_MCR:
            pSerial->MCR = (UINT8)Value;
            break;

        case UART_SCR:
            pSerial->SCR = (UINT8)Value;
            break;

        default:
            break;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HvSerial_MemoryRead(
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
HvSerial_MemoryWrite(
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

/* Virtual serial vtable */
static IVirtualDeviceVtbl g_VirtualSerialVtbl = {
    HvSerial_QueryInterface,
    HvSerial_AddRef,
    HvSerial_Release,
    HvSerial_GetDeviceType,
    HvSerial_GetDeviceName,
    HvSerial_Initialize,
    HvSerial_Shutdown,
    HvSerial_Reset,
    HvSerial_IORead,
    HvSerial_IOWrite,
    HvSerial_MemoryRead,
    HvSerial_MemoryWrite
};

/* ======================================================================= */
/* Virtual serial creation                                                 */
/* ======================================================================= */

HRESULT
HvCreateVirtualSerial(
    VOID (*OutputCallback)(VOID* Context, UINT8 Data),
    VOID* OutputContext,
    IVirtualDevice** ppDevice
)
{
    HV_VIRTUAL_SERIAL* pSerial;

    if (ppDevice == NULL) {
        return E_POINTER;
    }

    pSerial = (HV_VIRTUAL_SERIAL*)RtlAllocateMemory(NULL, sizeof(HV_VIRTUAL_SERIAL));
    if (pSerial == NULL) {
        return HV_NO_RESOURCES;
    }

    RtlZeroMemory(pSerial, sizeof(HV_VIRTUAL_SERIAL));

    /* Initialize */
    pSerial->Interface.lpVtbl = &g_VirtualSerialVtbl;
    pSerial->RefCount = 1;
    pSerial->LSR = LSR_THR_EMPTY | LSR_THR_IDLE;
    pSerial->IIR = 1;  /* No interrupt pending */
    pSerial->OutputCallback = OutputCallback;
    pSerial->OutputContext = OutputContext;

    *ppDevice = (IVirtualDevice*)pSerial;
    return S_OK;
}

/* ======================================================================= */
/* Inject received data into serial port                                   */
/* ======================================================================= */

HRESULT
HvSerialInjectData(
    IVirtualDevice* pDevice,
    CONST UINT8* Data,
    UINT32 Length
)
{
    HV_VIRTUAL_SERIAL* pSerial = (HV_VIRTUAL_SERIAL*)pDevice;
    UINT32 i;
    UINT32 nextTail;

    if (pDevice == NULL || Data == NULL) {
        return E_POINTER;
    }

    /* Add data to RX FIFO */
    for (i = 0; i < Length; i++) {
        nextTail = (pSerial->RxTail + 1) % 16;
        if (nextTail != pSerial->RxHead) {
            pSerial->RxFIFO[pSerial->RxTail] = Data[i];
            pSerial->RxTail = nextTail;
            pSerial->LSR |= LSR_DATA_READY;
        } else {
            /* FIFO full */
            pSerial->LSR |= LSR_OVERRUN_ERROR;
            break;
        }
    }

    return S_OK;
}
