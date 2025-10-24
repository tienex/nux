/** @file
  Platform Layer Interface

  Defines the COM-style PLT interface for platform-specific functionality
  including IRQ management, physical CPU control, timer operations, and
  interrupt handling.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>
#include <types.h>

//
// PLT Interface GUID
//

#define IID_IPLT \
  { 0x8E91A5C2, 0x53F6, 0x4B7A, { 0xB4, 0x6C, 0x9D, 0x8E, 0x3F, 0x4A, 0x2B, 0x5C } }

//
// Forward Declarations
//

INTERFACE_DECL (IPlt)
INTERFACE_DECL (IPltHardware)
INTERFACE_DECL (IPltIrq)
INTERFACE_DECL (IPltPcpu)
INTERFACE_DECL (IPltTimer)

//
// PLT Constants
//

#define PLT_PCPU_INVALID ((UINTN)-1)

//
// IRQ Type Enumeration
//

typedef enum {
  PltIrqEdge    = 0,  ///< Edge-triggered interrupt
  PltIrqLvlLo   = 1,  ///< Level-triggered active-low interrupt
  PltIrqLvlHi   = 2,  ///< Level-triggered active-high interrupt
  PltIrqInvalid = 3   ///< Invalid IRQ type
} PLT_IRQ_TYPE;

/** Legacy type alias for compatibility **/
enum plt_irq_type {
  PLT_IRQ_EDGE,
  PLT_IRQ_LVLLO,
  PLT_IRQ_LVLHI,
  PLT_IRQ_INVALID,
};

//
// IPltHardware Interface - Standard Hardware Operations
//

struct _IPltHardwareVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IPltHardware *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IPltHardware *This);
  ULONG   (*Release)(IN IPltHardware *This);

  //
  // IPltHardware Methods
  //

  /**
    Output a character to the console.

    @param[in]  This  Pointer to the IPltHardware instance.
    @param[in]  Char  Character to output.
  **/
  VOID (*PutChar)(IN IPltHardware *This, IN INT32 Char);
};

INTERFACE_INHERIT_IUNKNOWN (IPltHardware)

//
// IPltIrq Interface - IRQ Management
//

struct _IPltIrqVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IPltIrq *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IPltIrq *This);
  ULONG   (*Release)(IN IPltIrq *This);

  //
  // IPltIrq Methods
  //

  /**
    Get the type of an IRQ.

    @param[in]  This  Pointer to the IPltIrq instance.
    @param[in]  Irq   IRQ number.

    @return IRQ type (edge or level-triggered).
  **/
  PLT_IRQ_TYPE (*GetType)(IN IPltIrq *This, IN UINTN Irq);

  /**
    Enable an IRQ.

    @param[in]  This  Pointer to the IPltIrq instance.
    @param[in]  Irq   IRQ number to enable.
  **/
  VOID (*Enable)(IN IPltIrq *This, IN UINTN Irq);

  /**
    Disable an IRQ.

    @param[in]  This  Pointer to the IPltIrq instance.
    @param[in]  Irq   IRQ number to disable.
  **/
  VOID (*Disable)(IN IPltIrq *This, IN UINTN Irq);

  /**
    Get the maximum IRQ number supported.

    @param[in]  This  Pointer to the IPltIrq instance.

    @return Maximum IRQ number.
  **/
  UINTN (*GetMaxIrq)(IN IPltIrq *This);

  /**
    Check if IRQ is level-triggered.

    @param[in]  This  Pointer to the IPltIrq instance.
    @param[in]  Irq   IRQ number.

    @retval TRUE   IRQ is level-triggered.
    @retval FALSE  IRQ is edge-triggered.
  **/
  BOOLEAN (*IsLevel)(IN IPltIrq *This, IN UINTN Irq);

  /**
    Send End-of-Interrupt for an IRQ.

    @param[in]  This  Pointer to the IPltIrq instance.
    @param[in]  Irq   IRQ number.
  **/
  VOID (*EndOfInterrupt)(IN IPltIrq *This, IN UINTN Irq);
};

INTERFACE_INHERIT_IUNKNOWN (IPltIrq)

//
// IPltPcpu Interface - Physical CPU Management
//

struct _IPltPcpuVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IPltPcpu *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IPltPcpu *This);
  ULONG   (*Release)(IN IPltPcpu *This);

  //
  // IPltPcpu Methods
  //

  /**
    Iterate through physical CPUs.

    @param[in]  This  Pointer to the IPltPcpu instance.

    @return Next PCPU ID, or PLT_PCPU_INVALID if iteration complete.
  **/
  INTN (*Iterate)(IN IPltPcpu *This);

  /**
    Load platform-specific state for current CPU.

    @param[in]  This  Pointer to the IPltPcpu instance.
  **/
  VOID (*Enter)(IN IPltPcpu *This);

  /**
    Issue NMI to a specific CPU.

    @param[in]  This    Pointer to the IPltPcpu instance.
    @param[in]  PcpuId  Physical CPU identifier.
  **/
  VOID (*SendNmi)(IN IPltPcpu *This, IN INTN PcpuId);

  /**
    Broadcast NMI to all CPUs.

    @param[in]  This  Pointer to the IPltPcpu instance.
  **/
  VOID (*BroadcastNmi)(IN IPltPcpu *This);

  /**
    Issue IPI to a specific CPU.

    @param[in]  This    Pointer to the IPltPcpu instance.
    @param[in]  PcpuId  Physical CPU identifier.
  **/
  VOID (*SendIpi)(IN IPltPcpu *This, IN INTN PcpuId);

  /**
    Broadcast IPI to all CPUs.

    @param[in]  This  Pointer to the IPltPcpu instance.
  **/
  VOID (*BroadcastIpi)(IN IPltPcpu *This);

  /**
    Get current physical CPU ID.

    @param[in]  This  Pointer to the IPltPcpu instance.

    @return Current PCPU ID.
  **/
  UINTN (*GetId)(IN IPltPcpu *This);

  /**
    Start a remote CPU.

    @param[in]  This       Pointer to the IPltPcpu instance.
    @param[in]  PcpuId     Physical CPU identifier.
    @param[in]  StartAddr  Physical address to begin execution.
  **/
  VOID (*Start)(IN IPltPcpu *This, IN UINTN PcpuId, IN PHYSICAL_ADDRESS StartAddr);
};

INTERFACE_INHERIT_IUNKNOWN (IPltPcpu)

//
// IPltTimer Interface - Timer Operations
//

struct _IPltTimerVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IPltTimer *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IPltTimer *This);
  ULONG   (*Release)(IN IPltTimer *This);

  //
  // IPltTimer Methods
  //

  /**
    Read timer counter.

    @param[in]  This  Pointer to the IPltTimer instance.

    @return Current timer count.
  **/
  UINT64 (*GetCounter)(IN IPltTimer *This);

  /**
    Set timer counter.

    @param[in]  This     Pointer to the IPltTimer instance.
    @param[in]  Counter  New counter value.
  **/
  VOID (*SetCounter)(IN IPltTimer *This, IN UINT64 Counter);

  /**
    Get timer period in femtoseconds.

    @param[in]  This  Pointer to the IPltTimer instance.

    @return Timer period in femtoseconds.
  **/
  UINT64 (*GetPeriod)(IN IPltTimer *This);

  /**
    Set timer alarm.

    @param[in]  This   Pointer to the IPltTimer instance.
    @param[in]  Ticks  Number of ticks until alarm.
  **/
  VOID (*SetAlarm)(IN IPltTimer *This, IN UINT64 Ticks);

  /**
    Clear timer alarm.

    @param[in]  This  Pointer to the IPltTimer instance.
  **/
  VOID (*ClearAlarm)(IN IPltTimer *This);

  /**
    Send End-of-Interrupt for timer.

    @param[in]  This  Pointer to the IPltTimer instance.
  **/
  VOID (*EndOfInterrupt)(IN IPltTimer *This);
};

INTERFACE_INHERIT_IUNKNOWN (IPltTimer)

//
// IPlt Main Interface - Aggregates all PLT functionality
//

struct _IPltVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IPlt *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IPlt *This);
  ULONG   (*Release)(IN IPlt *This);

  //
  // IPlt Methods
  //

  /**
    Initialize the platform layer.

    @param[in]  This  Pointer to the IPlt instance.
  **/
  VOID (*Init)(IN IPlt *This);

  /**
    Get the hardware interface.

    @param[in]  This        Pointer to the IPlt instance.
    @param[out] ppHardware  Receives the IPltHardware interface pointer.

    @retval S_OK        Interface retrieved successfully.
    @retval E_POINTER   ppHardware is NULL.
  **/
  HRESULT (*GetHardwareInterface)(IN IPlt *This, OUT IPltHardware **ppHardware);

  /**
    Get the IRQ management interface.

    @param[in]  This   Pointer to the IPlt instance.
    @param[out] ppIrq  Receives the IPltIrq interface pointer.

    @retval S_OK        Interface retrieved successfully.
    @retval E_POINTER   ppIrq is NULL.
  **/
  HRESULT (*GetIrqInterface)(IN IPlt *This, OUT IPltIrq **ppIrq);

  /**
    Get the physical CPU interface.

    @param[in]  This    Pointer to the IPlt instance.
    @param[out] ppPcpu  Receives the IPltPcpu interface pointer.

    @retval S_OK        Interface retrieved successfully.
    @retval E_POINTER   ppPcpu is NULL.
  **/
  HRESULT (*GetPcpuInterface)(IN IPlt *This, OUT IPltPcpu **ppPcpu);

  /**
    Get the timer interface.

    @param[in]  This     Pointer to the IPlt instance.
    @param[out] ppTimer  Receives the IPltTimer interface pointer.

    @retval S_OK        Interface retrieved successfully.
    @retval E_POINTER   ppTimer is NULL.
  **/
  HRESULT (*GetTimerInterface)(IN IPlt *This, OUT IPltTimer **ppTimer);

  /**
    Get the IPI end-of-interrupt interface.

    @param[in]  This  Pointer to the IPlt instance.
  **/
  VOID (*EndOfInterruptIpi)(IN IPlt *This);

  /**
    Handle platform interrupt.

    @param[in]  This   Pointer to the IPlt instance.
    @param[in]  Vector Interrupt vector number.
    @param[in]  pFrame Current CPU frame.

    @return Updated frame pointer.
  **/
  struct hal_frame *(*Interrupt)(IN IPlt *This, IN UINTN Vector,
                                  IN struct hal_frame *pFrame);
};

INTERFACE_INHERIT_IUNKNOWN (IPlt)

//
// Legacy C Function Wrappers (for backward compatibility)
//

extern IPlt *gpPlt;

static inline void plt_init (void) {
  gpPlt->lpVtbl->Init(gpPlt);
}

static inline void plt_hw_putc (int c) {
  IPltHardware *pHw;
  gpPlt->lpVtbl->GetHardwareInterface(gpPlt, &pHw);
  pHw->lpVtbl->PutChar(pHw, c);
}

static inline enum plt_irq_type plt_irq_type (unsigned irq) {
  IPltIrq *pIrq;
  PLT_IRQ_TYPE type;
  gpPlt->lpVtbl->GetIrqInterface(gpPlt, &pIrq);
  type = pIrq->lpVtbl->GetType(pIrq, irq);
  return (enum plt_irq_type)type;
}

static inline void plt_irq_enable (unsigned irq) {
  IPltIrq *pIrq;
  gpPlt->lpVtbl->GetIrqInterface(gpPlt, &pIrq);
  pIrq->lpVtbl->Enable(pIrq, irq);
}

static inline void plt_irq_disable (unsigned irq) {
  IPltIrq *pIrq;
  gpPlt->lpVtbl->GetIrqInterface(gpPlt, &pIrq);
  pIrq->lpVtbl->Disable(pIrq, irq);
}

static inline unsigned plt_irq_max (void) {
  IPltIrq *pIrq;
  gpPlt->lpVtbl->GetIrqInterface(gpPlt, &pIrq);
  return pIrq->lpVtbl->GetMaxIrq(pIrq);
}

static inline bool plt_irq_islevel (unsigned irq) {
  IPltIrq *pIrq;
  gpPlt->lpVtbl->GetIrqInterface(gpPlt, &pIrq);
  return pIrq->lpVtbl->IsLevel(pIrq, irq);
}

static inline int plt_pcpu_iterate (void) {
  IPltPcpu *pPcpu;
  gpPlt->lpVtbl->GetPcpuInterface(gpPlt, &pPcpu);
  return pPcpu->lpVtbl->Iterate(pPcpu);
}

static inline void plt_pcpu_enter (void) {
  IPltPcpu *pPcpu;
  gpPlt->lpVtbl->GetPcpuInterface(gpPlt, &pPcpu);
  pPcpu->lpVtbl->Enter(pPcpu);
}

static inline void plt_pcpu_nmi (int pcpuid) {
  IPltPcpu *pPcpu;
  gpPlt->lpVtbl->GetPcpuInterface(gpPlt, &pPcpu);
  pPcpu->lpVtbl->SendNmi(pPcpu, pcpuid);
}

static inline void plt_pcpu_nmiall (void) {
  IPltPcpu *pPcpu;
  gpPlt->lpVtbl->GetPcpuInterface(gpPlt, &pPcpu);
  pPcpu->lpVtbl->BroadcastNmi(pPcpu);
}

static inline void plt_pcpu_ipi (int pcpuid) {
  IPltPcpu *pPcpu;
  gpPlt->lpVtbl->GetPcpuInterface(gpPlt, &pPcpu);
  pPcpu->lpVtbl->SendIpi(pPcpu, pcpuid);
}

static inline void plt_pcpu_ipiall (void) {
  IPltPcpu *pPcpu;
  gpPlt->lpVtbl->GetPcpuInterface(gpPlt, &pPcpu);
  pPcpu->lpVtbl->BroadcastIpi(pPcpu);
}

static inline unsigned plt_pcpu_id (void) {
  IPltPcpu *pPcpu;
  gpPlt->lpVtbl->GetPcpuInterface(gpPlt, &pPcpu);
  return pPcpu->lpVtbl->GetId(pPcpu);
}

static inline void plt_pcpu_start (unsigned pcpuid, paddr_t start) {
  IPltPcpu *pPcpu;
  gpPlt->lpVtbl->GetPcpuInterface(gpPlt, &pPcpu);
  pPcpu->lpVtbl->Start(pPcpu, pcpuid, start);
}

static inline uint64_t plt_tmr_ctr (void) {
  IPltTimer *pTimer;
  gpPlt->lpVtbl->GetTimerInterface(gpPlt, &pTimer);
  return pTimer->lpVtbl->GetCounter(pTimer);
}

static inline void plt_tmr_setctr (uint64_t ctr) {
  IPltTimer *pTimer;
  gpPlt->lpVtbl->GetTimerInterface(gpPlt, &pTimer);
  pTimer->lpVtbl->SetCounter(pTimer, ctr);
}

static inline uint64_t plt_tmr_period (void) {
  IPltTimer *pTimer;
  gpPlt->lpVtbl->GetTimerInterface(gpPlt, &pTimer);
  return pTimer->lpVtbl->GetPeriod(pTimer);
}

static inline void plt_tmr_setalm (uint64_t alm) {
  IPltTimer *pTimer;
  gpPlt->lpVtbl->GetTimerInterface(gpPlt, &pTimer);
  pTimer->lpVtbl->SetAlarm(pTimer, alm);
}

static inline void plt_tmr_clralm (void) {
  IPltTimer *pTimer;
  gpPlt->lpVtbl->GetTimerInterface(gpPlt, &pTimer);
  pTimer->lpVtbl->ClearAlarm(pTimer);
}

static inline void plt_eoi_timer (void) {
  IPltTimer *pTimer;
  gpPlt->lpVtbl->GetTimerInterface(gpPlt, &pTimer);
  pTimer->lpVtbl->EndOfInterrupt(pTimer);
}

static inline void plt_eoi_irq (unsigned irq) {
  IPltIrq *pIrq;
  gpPlt->lpVtbl->GetIrqInterface(gpPlt, &pIrq);
  pIrq->lpVtbl->EndOfInterrupt(pIrq, irq);
}

static inline void plt_eoi_ipi (void) {
  gpPlt->lpVtbl->EndOfInterruptIpi(gpPlt);
}

static inline struct hal_frame *plt_interrupt (unsigned vect, struct hal_frame *f) {
  return gpPlt->lpVtbl->Interrupt(gpPlt, vect, f);
}

#endif // PLATFORM_H
