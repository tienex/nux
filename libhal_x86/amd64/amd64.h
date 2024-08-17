/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	GPL2.0+
*/

#ifndef _AMD64_INTERNAL
#define _AMD64_INTERNAL

#define TSS_GDTIDX(_i) (6 + (_i) * 2)

#define KCS 0x08
#define KDS 0x10
#define UCS32 0x1b		/* Unused. */
#define UDS 0x23
#define UCS 0x2b


#define VECT_IPI0 0xF0
#define VECT_IRQ0 0x22
#define VECT_SYSC 0x21
#define MAXIRQS   (VECT_IPI0 - VECT_IRQ0)

#endif
