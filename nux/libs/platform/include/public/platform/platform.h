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
                                  IN struct hal_frame *Frame);
};

INTERFACE_INHERIT_IUNKNOWN (IPlatform)

//
// C Function Wrappers for Platform Interface
//

extern IPlatform *gpPlatform;

static INLINE VOID PlatformInit (VOID) {
  gpPlatform->lpVtbl->Init(gpPlatform);
}

static INLINE VOID PlatformHwPutc (INT32 C) {
  IPlatformHardware *Hw;
  gpPlatform->lpVtbl->GetHardwareInterface(gpPlatform, &Hw);
  Hw->lpVtbl->PutChar(Hw, C);
}

static INLINE PLATFORM_IRQ_TYPE PlatformIrqType (IN UINTN Irq) {
  IPlatformIrq *IrqInterface;
  gpPlatform->lpVtbl->GetIrqInterface(gpPlatform, &IrqInterface);
  return IrqInterface->lpVtbl->GetType(IrqInterface, Irq);
}

static INLINE VOID PlatformIrqEnable (IN UINTN Irq) {
  IPlatformIrq *IrqInterface;
  gpPlatform->lpVtbl->GetIrqInterface(gpPlatform, &IrqInterface);
  IrqInterface->lpVtbl->Enable(IrqInterface, Irq);
}

static INLINE VOID PlatformIrqDisable (IN UINTN Irq) {
  IPlatformIrq *IrqInterface;
  gpPlatform->lpVtbl->GetIrqInterface(gpPlatform, &IrqInterface);
  IrqInterface->lpVtbl->Disable(IrqInterface, Irq);
}

static INLINE UINTN PlatformIrqMax (VOID) {
  IPlatformIrq *IrqInterface;
  gpPlatform->lpVtbl->GetIrqInterface(gpPlatform, &IrqInterface);
  return IrqInterface->lpVtbl->GetMaxIrq(IrqInterface);
}

static INLINE BOOLEAN PlatformIrqIsLevel (IN UINTN Irq) {
  IPlatformIrq *IrqInterface;
  gpPlatform->lpVtbl->GetIrqInterface(gpPlatform, &IrqInterface);
  return IrqInterface->lpVtbl->IsLevel(IrqInterface, Irq);
}

static INLINE INTN PlatformPcpuIterate (VOID) {
  IPlatformPcpu *PcpuInterface;
  gpPlatform->lpVtbl->GetPcpuInterface(gpPlatform, &PcpuInterface);
  return PcpuInterface->lpVtbl->Iterate(PcpuInterface);
}

static INLINE VOID PlatformPcpuEnter (VOID) {
  IPlatformPcpu *PcpuInterface;
  gpPlatform->lpVtbl->GetPcpuInterface(gpPlatform, &PcpuInterface);
  PcpuInterface->lpVtbl->Enter(PcpuInterface);
}

static INLINE VOID PlatformPcpuNmi (INTN PcpuId) {
  IPlatformPcpu *PcpuInterface;
  gpPlatform->lpVtbl->GetPcpuInterface(gpPlatform, &PcpuInterface);
  PcpuInterface->lpVtbl->SendNmi(PcpuInterface, PcpuId);
}

static INLINE VOID PlatformPcpuNmiAll (VOID) {
  IPlatformPcpu *PcpuInterface;
  gpPlatform->lpVtbl->GetPcpuInterface(gpPlatform, &PcpuInterface);
  PcpuInterface->lpVtbl->BroadcastNmi(PcpuInterface);
}

static INLINE VOID PlatformPcpuIpi (INTN PcpuId) {
  IPlatformPcpu *PcpuInterface;
  gpPlatform->lpVtbl->GetPcpuInterface(gpPlatform, &PcpuInterface);
  PcpuInterface->lpVtbl->SendIpi(PcpuInterface, PcpuId);
}

static INLINE VOID PlatformPcpuIpiAll (VOID) {
  IPlatformPcpu *PcpuInterface;
  gpPlatform->lpVtbl->GetPcpuInterface(gpPlatform, &PcpuInterface);
  PcpuInterface->lpVtbl->BroadcastIpi(PcpuInterface);
}

static INLINE UINTN PlatformPcpuId (VOID) {
  IPlatformPcpu *PcpuInterface;
  gpPlatform->lpVtbl->GetPcpuInterface(gpPlatform, &PcpuInterface);
  return PcpuInterface->lpVtbl->GetId(PcpuInterface);
}

static INLINE VOID PlatformPcpuStart (IN UINTN PcpuId, IN PHYSICAL_ADDRESS Start) {
  IPlatformPcpu *PcpuInterface;
  gpPlatform->lpVtbl->GetPcpuInterface(gpPlatform, &PcpuInterface);
  PcpuInterface->lpVtbl->Start(PcpuInterface, PcpuId, Start);
}

static INLINE UINT64 PlatformTmrGetCounter (VOID) {
  IPlatformTimer *TimerInterface;
  gpPlatform->lpVtbl->GetTimerInterface(gpPlatform, &TimerInterface);
  return TimerInterface->lpVtbl->GetCounter(TimerInterface);
}

static INLINE VOID PlatformTmrSetCounter (IN UINT64 Counter) {
  IPlatformTimer *TimerInterface;
  gpPlatform->lpVtbl->GetTimerInterface(gpPlatform, &TimerInterface);
  TimerInterface->lpVtbl->SetCounter(TimerInterface, Counter);
}

static INLINE UINT64 PlatformTmrPeriod (VOID) {
  IPlatformTimer *TimerInterface;
  gpPlatform->lpVtbl->GetTimerInterface(gpPlatform, &TimerInterface);
  return TimerInterface->lpVtbl->GetPeriod(TimerInterface);
}

static INLINE VOID PlatformTmrSetAlarm (IN UINT64 Ticks) {
  IPlatformTimer *TimerInterface;
  gpPlatform->lpVtbl->GetTimerInterface(gpPlatform, &TimerInterface);
  TimerInterface->lpVtbl->SetAlarm(TimerInterface, Ticks);
}

static INLINE VOID PlatformTmrClearAlarm (VOID) {
  IPlatformTimer *TimerInterface;
  gpPlatform->lpVtbl->GetTimerInterface(gpPlatform, &TimerInterface);
  TimerInterface->lpVtbl->ClearAlarm(TimerInterface);
}

static INLINE VOID PlatformEoiTimer (VOID) {
  IPlatformTimer *TimerInterface;
  gpPlatform->lpVtbl->GetTimerInterface(gpPlatform, &TimerInterface);
  TimerInterface->lpVtbl->EndOfInterrupt(TimerInterface);
}

static INLINE VOID PlatformEoiIrq (IN UINTN Irq) {
  IPlatformIrq *IrqInterface;
  gpPlatform->lpVtbl->GetIrqInterface(gpPlatform, &IrqInterface);
  IrqInterface->lpVtbl->EndOfInterrupt(IrqInterface, Irq);
}

static INLINE VOID PlatformEoiIpi (VOID) {
  gpPlatform->lpVtbl->EndOfInterruptIpi(gpPlatform);
}

static INLINE struct hal_frame *PlatformInterrupt (IN UINTN Vect, IN struct hal_frame *Frame) {
  return gpPlatform->lpVtbl->Interrupt(gpPlatform, Vect, Frame);
}

#endif // PLATFORM_H
