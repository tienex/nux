/*
  EC - An embedded non standard C library
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef EC_AMD64_LIMITS_H
#define EC_AMD64_LIMITS_H

#define CHAR_BIT 8
#define WORD_BIT 32
#define LONG_BIT 64

#define	UCHAR_MAX	0xff
#define	SCHAR_MAX	0x7f
#define SCHAR_MIN	(-0x7f-1)

#define	USHRT_MAX	0xffff
#define	SHRT_MAX	0x7fff
#define SHRT_MIN        (-0x7fff-1)

#define	UINT_MAX	0xffffffffU
#define	INT_MAX		0x7fffffff
#define	INT_MIN		(-0x7fffffff-1)

#define	ULONG_MAX	0xffffffffffffffffUL
#define	LONG_MAX	0x7fffffffffffffffL
#define	LONG_MIN	(-0x7fffffffffffffffL-1)

#endif /* EC_AMD64_LIMITS_H */
