/** @file
  Platform Layer Interface

  Defines the COM-style Platform interface for platform-specific functionality
  including IRQ management, physical CPU control, timer operations, and
  interrupt handling.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __platform_platform_h__
#define __platform_platform_h__

#include <stdbool.h>
#include <nux/types.h>

//
// Platform Interface GUID
//

#define IID_IPLATFORM \
  { 0x8E91A5C2, 0x53F6, 0x4B7A, { 0xB4, 0x6C, 0x9D, 0x8E, 0x3F, 0x4A, 0x2B, 0x5C } }

//
// Forward Declarations
//

INTERFACE_DECL (IPlatform)
INTERFACE_DECL (IPlatformHardware)
INTERFACE_DECL (IPlatformIrq)
INTERFACE_DECL (IPlatformPcpu)
INTERFACE_DECL (IPlatformTimer)

//
// Platform Constants
//

#define PLATFORM_PCPU_INVALID ((UINTN)-1)

//
// IRQ Type Enumeration
//

typedef enum {
  PlatformIrqEdge    = 0,  ///< Edge-triggered interrupt
  PlatformIrqLvlLo   = 1,  ///< Level-triggered active-low interrupt
  PlatformIrqLvlHi   = 2,  ///< Level-triggered active-high interrupt
  PlatformIrqInvalid = 3   ///< Invalid IRQ type
} PLATFORM_IRQ_TYPE;

/** Legacy type alias for compatibility **/
enum platform_irq_type {
  PLATFORM_IRQ_EDGE,
  PLATFORM_IRQ_LVLLO,
  PLATFORM_IRQ_LVLHI,
  PLATFORM_IRQ_INVALID,
};

//
// IPlatformHardware Interface - Standard Hardware Operations
//

struct _IPlatformHardwareVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IPlatformHardware *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IPlatformHardware *This);
  ULONG   (*Release)(IN IPlatformHardware *This);

  //
  // IPlatformHardware Methods
  //

  /**
    Output a character to the console.

    @param[in]  This  Pointer to the IPlatformHardware instance.
    @param[in]  Char  Character to output.
  **/
  VOID (*PutChar)(IN IPlatformHardware *This, IN INT32 Char);
};

INTERFACE_INHERIT_IUNKNOWN (IPlatformHardware)

//
// IPlatformIrq Interface - IRQ Management
//

struct _IPlatformIrqVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IPlatformIrq *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IPlatformIrq *This);
  ULONG   (*Release)(IN IPlatformIrq *This);

  //
  // IPlatformIrq Methods
  //

  /**
    Get the type of an IRQ.

    @param[in]  This  Pointer to the IPlatformIrq instance.
    @param[in]  Irq   IRQ number.

    @return IRQ type (edge or level-triggered).
  **/
  PLATFORM_IRQ_TYPE (*GetType)(IN IPlatformIrq *This, IN UINTN Irq);

  /**
    Enable an IRQ.

    @param[in]  This  Pointer to the IPlatformIrq instance.
    @param[in]  Irq   IRQ number to enable.
  **/
  VOID (*Enable)(IN IPlatformIrq *This, IN UINTN Irq);

  /**
    Disable an IRQ.

    @param[in]  This  Pointer to the IPlatformIrq instance.
    @param[in]  Irq   IRQ number to disable.
  **/
  VOID (*Disable)(IN IPlatformIrq *This, IN UINTN Irq);

  /**
    Get the maximum IRQ number supported.

    @param[in]  This  Pointer to the IPlatformIrq instance.

    @return Maximum IRQ number.
  **/
  UINTN (*GetMaxIrq)(IN IPlatformIrq *This);

  /**
    Check if IRQ is level-triggered.

    @param[in]  This  Pointer to the IPlatformIrq instance.
    @param[in]  Irq   IRQ number.

    @retval TRUE   IRQ is level-triggered.
    @retval FALSE  IRQ is edge-triggered.
  **/
  BOOLEAN (*IsLevel)(IN IPlatformIrq *This, IN UINTN Irq);

  /**
    Send End-of-Interrupt for an IRQ.

    @param[in]  This  Pointer to the IPlatformIrq instance.
    @param[in]  Irq   IRQ number.
  **/
  VOID (*EndOfInterrupt)(IN IPlatformIrq *This, IN UINTN Irq);
};

INTERFACE_INHERIT_IUNKNOWN (IPlatformIrq)

//
// IPlatformPcpu Interface - Physical CPU Management
//

struct _IPlatformPcpuVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IPlatformPcpu *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IPlatformPcpu *This);
  ULONG   (*Release)(IN IPlatformPcpu *This);

  //
  // IPlatformPcpu Methods
  //

  /**
    Iterate through physical CPUs.

    @param[in]  This  Pointer to the IPlatformPcpu instance.

    @return Next PCPU ID, or PLATFORM_PCPU_INVALID if iteration complete.
  **/
  INTN (*Iterate)(IN IPlatformPcpu *This);

  /**
    Load platform-specific state for current CPU.

    @param[in]  This  Pointer to the IPlatformPcpu instance.
  **/
  VOID (*Enter)(IN IPlatformPcpu *This);

  /**
    Issue NMI to a specific CPU.

    @param[in]  This    Pointer to the IPlatformPcpu instance.
    @param[in]  PcpuId  Physical CPU identifier.
  **/
  VOID (*SendNmi)(IN IPlatformPcpu *This, IN INTN PcpuId);

  /**
    Broadcast NMI to all CPUs.

    @param[in]  This  Pointer to the IPlatformPcpu instance.
  **/
  VOID (*BroadcastNmi)(IN IPlatformPcpu *This);

  /**
    Issue IPI to a specific CPU.

    @param[in]  This    Pointer to the IPlatformPcpu instance.
    @param[in]  PcpuId  Physical CPU identifier.
  **/
  VOID (*SendIpi)(IN IPlatformPcpu *This, IN INTN PcpuId);

  /**
    Broadcast IPI to all CPUs.

    @param[in]  This  Pointer to the IPlatformPcpu instance.
  **/
  VOID (*BroadcastIpi)(IN IPlatformPcpu *This);

  /**
    Get current physical CPU ID.

    @param[in]  This  Pointer to the IPlatformPcpu instance.

    @return Current PCPU ID.
  **/
  UINTN (*GetId)(IN IPlatformPcpu *This);

  /**
    Start a remote CPU.

    @param[in]  This       Pointer to the IPlatformPcpu instance.
    @param[in]  PcpuId     Physical CPU identifier.
    @param[in]  StartAddr  Physical address to begin execution.
  **/
  VOID (*Start)(IN IPlatformPcpu *This, IN UINTN PcpuId, IN PHYSICAL_ADDRESS StartAddr);
};

INTERFACE_INHERIT_IUNKNOWN (IPlatformPcpu)

//
// IPlatformTimer Interface - Timer Operations
//

struct _IPlatformTimerVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IPlatformTimer *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IPlatformTimer *This);
  ULONG   (*Release)(IN IPlatformTimer *This);

  //
  // IPlatformTimer Methods
  //

  /**
    Read timer counter.

    @param[in]  This  Pointer to the IPlatformTimer instance.

    @return Current timer count.
  **/
  UINT64 (*GetCounter)(IN IPlatformTimer *This);

  /**
    Set timer counter.

    @param[in]  This     Pointer to the IPlatformTimer instance.
    @param[in]  Counter  New counter value.
  **/
  VOID (*SetCounter)(IN IPlatformTimer *This, IN UINT64 Counter);

  /**
    Get timer period in femtoseconds.

    @param[in]  This  Pointer to the IPlatformTimer instance.

    @return Timer period in femtoseconds.
  **/
  UINT64 (*GetPeriod)(IN IPlatformTimer *This);

  /**
    Set timer alarm.

    @param[in]  This   Pointer to the IPlatformTimer instance.
    @param[in]  Ticks  Number of ticks until alarm.
  **/
  VOID (*SetAlarm)(IN IPlatformTimer *This, IN UINT64 Ticks);

  /**
    Clear timer alarm.

    @param[in]  This  Pointer to the IPlatformTimer instance.
  **/
  VOID (*ClearAlarm)(IN IPlatformTimer *This);

  /**
    Send End-of-Interrupt for timer.

    @param[in]  This  Pointer to the IPlatformTimer instance.
  **/
  VOID (*EndOfInterrupt)(IN IPlatformTimer *This);
};

INTERFACE_INHERIT_IUNKNOWN (IPlatformTimer)

//
// IPlatform Main Interface - Aggregates all Platform functionality
//

struct _IPlatformVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IPlatform *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IPlatform *This);
  ULONG   (*Release)(IN IPlatform *This);

  //
  // IPlatform Methods
  //

  /**
    Initialize the platform layer.

    @param[in]  This  Pointer to the IPlatform instance.
  **/
  VOID (*Init)(IN IPlatform *This);

  /**
    Get the hardware interface.

    @param[in]  This        Pointer to the IPlatform instance.
    @param[out] ppHardware  Receives the IPlatformHardware interface pointer.

    @retval S_OK        Interface retrieved successfully.
    @retval E_POINTER   ppHardware is NULL.
  **/
  HRESULT (*GetHardwareInterface)(IN IPlatform *This, OUT IPlatformHardware **ppHardware);

  /**
    Get the IRQ management interface.

    @param[in]  This   Pointer to the IPlatform instance.
    @param[out] ppIrq  Receives the IPlatformIrq interface pointer.

    @retval S_OK        Interface retrieved successfully.
    @retval E_POINTER   ppIrq is NULL.
  **/
  HRESULT (*GetIrqInterface)(IN IPlatform *This, OUT IPlatformIrq **ppIrq);

  /**
    Get the physical CPU interface.

    @param[in]  This    Pointer to the IPlatform instance.
    @param[out] ppPcpu  Receives the IPlatformPcpu interface pointer.

    @retval S_OK        Interface retrieved successfully.
    @retval E_POINTER   ppPcpu is NULL.
  **/
  HRESULT (*GetPcpuInterface)(IN IPlatform *This, OUT IPlatformPcpu **ppPcpu);

  /**
    Get the timer interface.

    @param[in]  This     Pointer to the IPlatform instance.
    @param[out] ppTimer  Receives the IPlatformTimer interface pointer.

    @retval S_OK        Interface retrieved successfully.
    @retval E_POINTER   ppTimer is NULL.
  **/
  HRESULT (*GetTimerInterface)(IN IPlatform *This, OUT IPlatformTimer **ppTimer);

  /**
    Get the IPI end-of-interrupt interface.

    @param[in]  This  Pointer to the IPlatform instance.
  **/
  VOID (*EndOfInterruptIpi)(IN IPlatform *This);

  /**
    Handle platform interrupt.

    @param[in]  This   Pointer to the IPlatform instance.
    @param[in]  Vector Interrupt vector number.
    @param[in]  pFrame Current CPU frame.

    @return Updated frame pointer.
  **/
  struct hal_frame *(*Interrupt)(IN IPlatform *This, IN UINTN Vector,
                                  IN struct hal_frame *pFrame);
};

INTERFACE_INHERIT_IUNKNOWN (IPlatform)

//
// Legacy C Function Wrappers (for backward compatibility)
//

extern IPlatform *gpPlatform;

static inline void platform_init (void) {
  gpPlatform->lpVtbl->Init(gpPlatform);
}

static inline void platform_hw_putc (int c) {
  IPlatformHardware *pHw;
  gpPlatform->lpVtbl->GetHardwareInterface(gpPlatform, &pHw);
  pHw->lpVtbl->PutChar(pHw, c);
}

static inline enum platform_irq_type platform_irq_type (unsigned irq) {
  IPlatformIrq *pIrq;
  PLATFORM_IRQ_TYPE type;
  gpPlatform->lpVtbl->GetIrqInterface(gpPlatform, &pIrq);
  type = pIrq->lpVtbl->GetType(pIrq, irq);
  return (enum platform_irq_type)type;
}

static inline void platform_irq_enable (unsigned irq) {
  IPlatformIrq *pIrq;
  gpPlatform->lpVtbl->GetIrqInterface(gpPlatform, &pIrq);
  pIrq->lpVtbl->Enable(pIrq, irq);
}

static inline void platform_irq_disable (unsigned irq) {
  IPlatformIrq *pIrq;
  gpPlatform->lpVtbl->GetIrqInterface(gpPlatform, &pIrq);
  pIrq->lpVtbl->Disable(pIrq, irq);
}

static inline unsigned platform_irq_max (void) {
  IPlatformIrq *pIrq;
  gpPlatform->lpVtbl->GetIrqInterface(gpPlatform, &pIrq);
  return pIrq->lpVtbl->GetMaxIrq(pIrq);
}

static inline bool platform_irq_islevel (unsigned irq) {
  IPlatformIrq *pIrq;
  gpPlatform->lpVtbl->GetIrqInterface(gpPlatform, &pIrq);
  return pIrq->lpVtbl->IsLevel(pIrq, irq);
}

static inline int platform_pcpu_iterate (void) {
  IPlatformPcpu *pPcpu;
  gpPlatform->lpVtbl->GetPcpuInterface(gpPlatform, &pPcpu);
  return pPcpu->lpVtbl->Iterate(pPcpu);
}

static inline void platform_pcpu_enter (void) {
  IPlatformPcpu *pPcpu;
  gpPlatform->lpVtbl->GetPcpuInterface(gpPlatform, &pPcpu);
  pPcpu->lpVtbl->Enter(pPcpu);
}

static inline void platform_pcpu_nmi (int pcpuid) {
  IPlatformPcpu *pPcpu;
  gpPlatform->lpVtbl->GetPcpuInterface(gpPlatform, &pPcpu);
  pPcpu->lpVtbl->SendNmi(pPcpu, pcpuid);
}

static inline void platform_pcpu_nmiall (void) {
  IPlatformPcpu *pPcpu;
  gpPlatform->lpVtbl->GetPcpuInterface(gpPlatform, &pPcpu);
  pPcpu->lpVtbl->BroadcastNmi(pPcpu);
}

static inline void platform_pcpu_ipi (int pcpuid) {
  IPlatformPcpu *pPcpu;
  gpPlatform->lpVtbl->GetPcpuInterface(gpPlatform, &pPcpu);
  pPcpu->lpVtbl->SendIpi(pPcpu, pcpuid);
}

static inline void platform_pcpu_ipiall (void) {
  IPlatformPcpu *pPcpu;
  gpPlatform->lpVtbl->GetPcpuInterface(gpPlatform, &pPcpu);
  pPcpu->lpVtbl->BroadcastIpi(pPcpu);
}

static inline unsigned platform_pcpu_id (void) {
  IPlatformPcpu *pPcpu;
  gpPlatform->lpVtbl->GetPcpuInterface(gpPlatform, &pPcpu);
  return pPcpu->lpVtbl->GetId(pPcpu);
}

static inline void platform_pcpu_start (unsigned pcpuid, paddr_t start) {
  IPlatformPcpu *pPcpu;
  gpPlatform->lpVtbl->GetPcpuInterface(gpPlatform, &pPcpu);
  pPcpu->lpVtbl->Start(pPcpu, pcpuid, start);
}

static inline uint64_t platform_tmr_ctr (void) {
  IPlatformTimer *pTimer;
  gpPlatform->lpVtbl->GetTimerInterface(gpPlatform, &pTimer);
  return pTimer->lpVtbl->GetCounter(pTimer);
}

static inline void platform_tmr_setctr (uint64_t ctr) {
  IPlatformTimer *pTimer;
  gpPlatform->lpVtbl->GetTimerInterface(gpPlatform, &pTimer);
  pTimer->lpVtbl->SetCounter(pTimer, ctr);
}

static inline uint64_t platform_tmr_period (void) {
  IPlatformTimer *pTimer;
  gpPlatform->lpVtbl->GetTimerInterface(gpPlatform, &pTimer);
  return pTimer->lpVtbl->GetPeriod(pTimer);
}

static inline void platform_tmr_setalm (uint64_t alm) {
  IPlatformTimer *pTimer;
  gpPlatform->lpVtbl->GetTimerInterface(gpPlatform, &pTimer);
  pTimer->lpVtbl->SetAlarm(pTimer, alm);
}

static inline void platform_tmr_clralm (void) {
  IPlatformTimer *pTimer;
  gpPlatform->lpVtbl->GetTimerInterface(gpPlatform, &pTimer);
  pTimer->lpVtbl->ClearAlarm(pTimer);
}

static inline void platform_eoi_timer (void) {
  IPlatformTimer *pTimer;
  gpPlatform->lpVtbl->GetTimerInterface(gpPlatform, &pTimer);
  pTimer->lpVtbl->EndOfInterrupt(pTimer);
}

static inline void platform_eoi_irq (unsigned irq) {
  IPlatformIrq *pIrq;
  gpPlatform->lpVtbl->GetIrqInterface(gpPlatform, &pIrq);
  pIrq->lpVtbl->EndOfInterrupt(pIrq, irq);
}

static inline void platform_eoi_ipi (void) {
  gpPlatform->lpVtbl->EndOfInterruptIpi(gpPlatform);
}

static inline struct hal_frame *platform_interrupt (unsigned vect, struct hal_frame *f) {
  return gpPlatform->lpVtbl->Interrupt(gpPlatform, vect, f);
}

//
// Deprecated aliases for old plt_* names (for backward compatibility)
//

#define gpPlt gpPlatform
#define plt_init platform_init
#define plt_hw_putc platform_hw_putc
#define plt_irq_type platform_irq_type
#define plt_irq_enable platform_irq_enable
#define plt_irq_disable platform_irq_disable
#define plt_irq_max platform_irq_max
#define plt_irq_islevel platform_irq_islevel
#define plt_pcpu_iterate platform_pcpu_iterate
#define plt_pcpu_enter platform_pcpu_enter
#define plt_pcpu_nmi platform_pcpu_nmi
#define plt_pcpu_nmiall platform_pcpu_nmiall
#define plt_pcpu_ipi platform_pcpu_ipi
#define plt_pcpu_ipiall platform_pcpu_ipiall
#define plt_pcpu_id platform_pcpu_id
#define plt_pcpu_start platform_pcpu_start
#define plt_tmr_ctr platform_tmr_ctr
#define plt_tmr_setctr platform_tmr_setctr
#define plt_tmr_period platform_tmr_period
#define plt_tmr_setalm platform_tmr_setalm
#define plt_tmr_clralm platform_tmr_clralm
#define plt_eoi_timer platform_eoi_timer
#define plt_eoi_irq platform_eoi_irq
#define plt_eoi_ipi platform_eoi_ipi
#define plt_interrupt platform_interrupt

#define PLT_PCPU_INVALID PLATFORM_PCPU_INVALID
#define PLT_IRQ_EDGE PLATFORM_IRQ_EDGE
#define PLT_IRQ_LVLLO PLATFORM_IRQ_LVLLO
#define PLT_IRQ_LVLHI PLATFORM_IRQ_LVLHI
#define PLT_IRQ_INVALID PLATFORM_IRQ_INVALID

#endif // PLATFORM_H
