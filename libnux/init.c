#include <stdio.h>
//#include <nux/hal.h>

#include "internal.h"

static void
init_physmem (void)
{

  /*
     Initialise Page Allocator.
   */
  pginit();

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

static void
init_virtmem (void)
{
#if 0
  mmap_init ();
  vmap_init ();
  vmap_free ((vaddr_t) hal_virtmem_vmapbase (), hal_virtmem_vmapsize ());
#endif
}

#define PACKAGE "NUX library"
#define PACKAGE_NAME "nux"
#define VERSION "0.0"
#define COPYRIGHT_YEAR 2019

static void
banner (void)
{
  printf ("%s (%s) %s\n", PACKAGE, PACKAGE_NAME, VERSION);

  printf ("\
Copyright (C) %d Gianluca Guida\n\
\n\
This program is free software; you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation; either version 2 of the License, or (at\n\
your option) any later version.\n\
\n\
This program is distributed in the hope that it will be useful, but\n\
WITHOUT ANY WARRANTY; see the GNU General Public License for more\n\
details.\n\n", COPYRIGHT_YEAR);
}

void klog_start (void);

void __attribute__((constructor(0)))
_nux_sysinit (void)
{
  banner ();
  init_physmem ();
#if 0
  klog_start ();


  init_virtmem ();

  /* Init CPUs operations */
  cpu_init ();

  /* Start the platform. This will discover CPUs and set up interrupt
     controllers. */
  plt_platform_start ();

  /* Start CPUs operations. */
  cpu_start ();

  /* Initialise components log. */
  klog_init (&uctxtlog, "UCTXT");
#endif
}

void
_nux_apinit (void)
{
#if 0
  mmap_enter ();
  cpu_enter ();
  exit (main_ap ());
#endif
}
