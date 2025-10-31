/** @file
  NUX NMI Emulation

  Provides NMI and IPI emulation for platforms without hardware NMI
  support. Uses atomic flags per CPU to track pending NMI/IPI events.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <nux/internal.h>
#include <hal/config.h>	/* For HAL_NMIEMUL */
#include <nux/nmiemul.h>
#include <nux/nux.h>
#include <assert.h>

#ifdef HAL_NMIEMUL

#define PENDING_NMI 1
#define PENDING_IPI 2
static UINT8 gPending[HAL_MAXCPUS];

/**
  Check if NMI is pending for current CPU.

  @retval TRUE   NMI is pending.
  @retval FALSE  No NMI pending.
**/
static BOOLEAN
NmiEmulNmiPending (
  VOID
  )
{
  UINT32 CpuId = CpuGetId ();
  UINT8 Pending;

  assert (CpuId < HAL_MAXCPUS);

  ANX_ATOMIC_LOAD (gPending + CpuId, &Pending, __ATOMIC_ACQUIRE);
  if (Pending & PENDING_NMI)
    return TRUE;
  return FALSE;
}

/**
  Clear pending NMI for current CPU.
**/
static VOID
NmiEmulNmiClear (
  VOID
  )
{
  UINT32 CpuId = CpuGetId ();

  assert (CpuId < HAL_MAXCPUS);

  ANX_ATOMIC_FETCH_AND (gPending + CpuId, ~(PENDING_NMI), __ATOMIC_ACQUIRE);
}

/**
  Check if IPI is pending for current CPU.

  @retval TRUE   IPI is pending.
  @retval FALSE  No IPI pending.
**/
BOOLEAN
NmiEmulIpiPending (
  VOID
  )
{
  UINT32 CpuId = CpuGetId ();
  UINT8 Pending;

  assert (CpuId < HAL_MAXCPUS);

  ANX_ATOMIC_LOAD (gPending + CpuId, &Pending, __ATOMIC_ACQUIRE);
  if (Pending & PENDING_IPI)
    return TRUE;
  return FALSE;
}

/**
  Clear pending IPI for current CPU.
**/
VOID
NmiEmulIpiClear (
  VOID
  )
{
  UINT32 CpuId = CpuGetId ();

  assert (CpuId < HAL_MAXCPUS);

  ANX_ATOMIC_FETCH_AND (gPending + CpuId, ~(PENDING_IPI), __ATOMIC_ACQUIRE);
}

/**
  Set pending NMI for specified CPU.

  @param[in] CpuId  CPU to send NMI to.
**/
VOID
NmiEmulNmiSet (
  IN UINT32  CpuId
  )
{
  assert (CpuId < HAL_MAXCPUS);
  ANX_ATOMIC_FETCH_OR (gPending + CpuId, PENDING_NMI, __ATOMIC_RELEASE);

}

/**
  Set pending NMI for all CPUs.
**/
VOID
NmiEmulNmiSetAll (
  VOID
  )
{
  for (UINT32 i = 0; i < HAL_MAXCPUS; i++)
    ANX_ATOMIC_FETCH_OR (gPending + i, PENDING_NMI, __ATOMIC_RELEASE);

}

/**
  Set pending IPI for specified CPU.

  @param[in] CpuId  CPU to send IPI to.
**/
VOID
NmiEmulIpiSet (
  IN UINT32  CpuId
  )
{
  assert (CpuId < HAL_MAXCPUS);
  ANX_ATOMIC_FETCH_OR (gPending + CpuId, PENDING_IPI, __ATOMIC_RELEASE);
}

/**
  Set pending IPI for all CPUs.
**/
VOID
NmiEmulIpiSetAll (
  VOID
  )
{
  for (UINT32 i = 0; i < HAL_MAXCPUS; i++)
    ANX_ATOMIC_FETCH_OR (gPending + i, PENDING_IPI, __ATOMIC_RELEASE);

}

/**
  NMI emulation entry point.

  Checks and processes pending emulated NMI if present.

  @param[in] Frame  HAL frame at entry.

  @return Frame to return to.
**/
struct hal_frame *
NmiEmulEntry (
  IN struct hal_frame  *Frame
  )
{
  if (NmiEmulNmiPending ())
    {
      NmiEmulNmiClear ();
      /* VOID */ hal_entry_nmi (Frame);
    }
  return Frame;
}

#endif /* HAVE_NMIEMUL */
