/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef __apxh_uefi_internal_h__
#define __apxh_uefi_internal_h__

VOID EfiExit (INT32 st);
VOID EfiExitBs (VOID);
UINTN EfiAllocateMaxAddr (UINTN maxaddr);

#endif
