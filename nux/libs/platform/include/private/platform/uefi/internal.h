/** @file
  UEFI Platform Internal Definitions

  Private header for UEFI platform implementation. Contains internal
  function declarations used between main.c and md.c.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
**/

#ifndef __platform_uefi_internal_h__
#define __platform_uefi_internal_h__

void efi_exit (int st);
void efi_exitbs (void);
unsigned long efi_allocate_maxaddr (unsigned long maxaddr);

#endif
