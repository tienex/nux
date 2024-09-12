/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	GPL2.0+
*/

#ifndef HAL_X86_I386_H
#define HAL_X86_I386_h

#define KERNBASE 0xc0000000

#define KCS 0x08
#define KDS 0x10
#define UCS 0x1b
#define UDS 0x23

#define VECT_SYSC 0x21

#define VECT_MAX 255

#define GDTSIZE	(4 * MAXCPUS + 5)

#endif
