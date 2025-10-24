/** @file
  NMI Emulation Layer

  Provides NMI (Non-Maskable Interrupt) emulation for architectures that
  do not natively support the NUX interrupt model.

  The NUX kernel model requires:
  - NMI: Non-maskable interrupt to interrupt the kernel
  - IPI: Inter-processor interrupt to interrupt userspace

  Some architectures (e.g., RISC-V) lack native NMI support. This library
  emulates NMI functionality using software flags and IPI mechanisms.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef _NUX_NMIEMUL_H
#define _NUX_NMIEMUL_H

#include <hal/hal.h>

//
// NMI Emulation Entry Points
//

/**
  NMI emulation entry point.

  Called on interrupt entry to check for pending emulated NMI.
  If an NMI is pending, handles it and clears the pending flag.

  @param[in] pFrame  CPU frame at time of interrupt.

  @return Modified frame pointer, or NULL.
**/
struct hal_frame *NmiEmulEntry (
  IN struct hal_frame  *pFrame
  );

/**
  IPI check entry point.

  Called to check and handle pending emulated IPI.
  Should be called periodically or on specific events.

  @param[in] pFrame  CPU frame at time of check.

  @return Modified frame pointer, or NULL.
**/
struct hal_frame *NmiEmulIpiCheck (
  IN struct hal_frame  *pFrame
  );

//
// NMI Emulation Control Functions
//

/**
  Set emulated NMI pending flag for a CPU.

  Marks an emulated NMI as pending on the specified CPU.

  @param[in] CpuId  Target CPU identifier.
**/
VOID NmiEmulNmiSet (
  IN UINTN  CpuId
  );

/**
  Set emulated NMI pending flag for all CPUs.

  Broadcasts an emulated NMI to all CPUs in the system.
**/
VOID NmiEmulNmiSetAll (
  VOID
  );

//
// IPI Emulation Control Functions
//

/**
  Check if IPI is pending on current CPU.

  @retval TRUE   IPI is pending.
  @retval FALSE  No IPI pending.
**/
BOOLEAN NmiEmulIpiPending (
  VOID
  );

/**
  Clear pending IPI flag on current CPU.
**/
VOID NmiEmulIpiClear (
  VOID
  );

/**
  Set emulated IPI pending flag for a CPU.

  Marks an emulated IPI as pending on the specified CPU.

  @param[in] CpuId  Target CPU identifier.
**/
VOID NmiEmulIpiSet (
  IN UINTN  CpuId
  );

/**
  Set emulated IPI pending flag for all CPUs.

  Broadcasts an emulated IPI to all CPUs in the system.
**/
VOID NmiEmulIpiSetAll (
  VOID
  );

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use NmiEmulEntry instead **/
static inline struct hal_frame *nmiemul_entry (struct hal_frame *f) {
  return NmiEmulEntry (f);
}

/** @deprecated Use NmiEmulIpiCheck instead **/
static inline struct hal_frame *nmiemul_ipicheck (struct hal_frame *f) {
  return NmiEmulIpiCheck (f);
}

/** @deprecated Use NmiEmulNmiSet instead **/
static inline void nmiemul_nmi_set (unsigned cpu) {
  NmiEmulNmiSet (cpu);
}

/** @deprecated Use NmiEmulNmiSetAll instead **/
static inline void nmiemul_nmi_setall (void) {
  NmiEmulNmiSetAll ();
}

/** @deprecated Use NmiEmulIpiPending instead **/
static inline bool nmiemul_ipi_pending (void) {
  return NmiEmulIpiPending ();
}

/** @deprecated Use NmiEmulIpiClear instead **/
static inline void nmiemul_ipi_clear (void) {
  NmiEmulIpiClear ();
}

/** @deprecated Use NmiEmulIpiSet instead **/
static inline void nmiemul_ipi_set (unsigned cpu) {
  NmiEmulIpiSet (cpu);
}

/** @deprecated Use NmiEmulIpiSetAll instead **/
static inline void nmiemul_ipi_setall (void) {
  NmiEmulIpiSetAll ();
}

#endif // _NUX_NMIEMUL_H
