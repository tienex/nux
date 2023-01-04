/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	GPL2.0+
*/

#ifndef _INTERNAL_H
#define _INTERNAL_H

void efi_exit (int st);
void efi_exitbs (void);
unsigned long efi_allocate_maxaddr (unsigned long maxaddr);

#endif
