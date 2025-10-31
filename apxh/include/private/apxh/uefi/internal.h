/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/
#pragma once

VOID EfiExit (INT32 st);
VOID EfiExitBs (VOID);
UINTN EfiAllocateMaxAddr (UINTN maxaddr);

