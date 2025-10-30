/** @file
  NUX Kernel Library Initialization

  Provides system initialization, memory subsystem setup, platform
  initialization, CPU discovery and startup, and system status management.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <stdio.h>
#include <platform/platform.h>
#include <nux/nux.h>

#include <nux/internal.h>

/** Set and cleared by HAL. If this is on, we're still in initialisation mode. **/
volatile UINT32 gNuxApBooting = 0;

volatile UINT8 gNuxStFlags = 0;

/**
  Get current NUX system status flags.

  @return Current status flags (NUXST_*).
**/
UINT8
NuxStatus (
  VOID
  )
{
  UINT8 St;

  __atomic_load (&gNuxStFlags, &St, __ATOMIC_ACQUIRE);
  return St;
}

/**
  Set NUX system status flags atomically.

  @param[in] Flags  Status flags to set (NUXST_*).

  @return Previous status flags value.
**/
UINT8
NuxStatusSetFlags (
  IN UINT8  Flags
  )
{
  return __atomic_fetch_or (&gNuxStFlags, Flags, __ATOMIC_ACQ_REL);
}

/**
  Check if CPU operations are safe to use.

  @retval TRUE   CPU operations are ready.
  @retval FALSE  Still in initialization, CPU operations not ready.
**/
BOOLEAN
NuxStatusOkCpu (
  VOID
  )
{
  if (__predict_false (!(NuxStatus () & NUXST_OKCPU))
      || __predict_false (gNuxApBooting))
    return FALSE;
  else
    return TRUE;
}

/**
  Initialize memory management subsystems.

  Initializes page frame allocator, KMEM, KVA allocator, and PFN cache.
**/
static VOID
InitializeMemory (
  VOID
  )
{
  /*
     Initialise Page Allocator.
   */
  PfnCacheBootstrap ();
  BatreePfnInitialize ();

  /*
     Initialise KMEM.
   */
  KmemInitialize ();

  /*
     Initialise KVA Allocator.
   */
  KvaInitialize ();

  /*
     Initialise PFN Cache.
   */
  PfnCacheInitialize ();

#if 0
  /*
     Step 1: Initialise PFN Database.
   */
  fmap_init ();

  pginit ();

  /*
     Step 3: Enable heap.
   */
  heap_init ();

  /*
     Step 4: Initialise Slab Allocator.
   */
  slab_init ();
#endif
}

#define PACKAGE        "NUX library"
#define PACKAGE_NAME   "nux"
#define VERSION        "0.0"
#define COPYRIGHT_YEAR 2019

/**
  Print system banner with version and copyright information.
**/
static VOID
PrintBanner (
  VOID
  )
{
  printf ("%s (%s) %s\n", PACKAGE, PACKAGE_NAME, VERSION);
  printf ("Copyright (C) %d Gianluca Guida\n\n", COPYRIGHT_YEAR);
}

VOID klog_start (VOID);

/**
  System initialization entry point.

  Called as a constructor function during early system startup.
  Initializes memory, platform support, CPUs, and waits for all
  Application Processors to boot before completing initialization.
**/
VOID __attribute__((constructor (0))) _nux_sysinit (VOID)
{
  PrintBanner ();

  /* Initialise memory management */
  InitializeMemory ();

  /* Start the platform. This will discover CPUs and set up interrupt
     controllers. */
  PlatformInit ();

  NuxStatusSetFlags (NUXST_OKPLT);

  /* Init CPUs operations */
  CpuInitialize ();

  /* Now safe to use CPU operations. */
  NuxStatusSetFlags (NUXST_OKCPU);

  /* Start all CPUs. */
  CpuStartAll ();

  printf ("Waiting for APs to boot..");
  while (gNuxApBooting)
    hal_cpu_relax ();
  printf ("done.\n");

  /* Signal HAL that we're done initialising. */
  hal_init_done ();

  NuxStatusSetFlags (NUXST_RUNNING);
}

/**
  Application Processor main entry point.

  Called by HAL when an Application Processor has completed low-level
  initialization and is ready to enter the system.
**/
VOID
HalMainAp (
  VOID
  )
{
  CpuEnter ();
  __atomic_sub_fetch (&gNuxApBooting, 1, __ATOMIC_ACQ_REL);
  exit (main_ap ());
}

#include <nux/nuxperf.h>
#undef NUXPERF
#undef NUXPERF_DECLARE
#define NUXPERF_DEFINE
#include <nux/perf.h>
