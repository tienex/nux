/*
  EC - An embedded non standard C library

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef EC_I386_LIMITS_H
#define EC_I386_LIMITS_H

#define CHAR_BIT 8
#define WORD_BIT 32
#define LONG_BIT 32

#define	UCHAR_MAX	0xff
#define	SCHAR_MAX	0x7f
#define SCHAR_MIN	(-0x7f-1)

#define	USHRT_MAX	0xffff
#define	SHRT_MAX	0x7fff
#define SHRT_MIN        (-0x7fff-1)

#define	UINT_MAX	0xffffffffU
#define	INT_MAX		0x7fffffff
#define	INT_MIN		(-0x7fffffff-1)

#define	ULONG_MAX	0xffffffffUL
#define	LONG_MAX	0x7fffffffL
#define	LONG_MIN	(-0x7fffffffL-1)

#define	ULLONG_MAX	0xffffffffffffffffULL
#define	LLONG_MAX	0x7fffffffffffffffLL
#define	LLONG_MIN	(-0x7fffffffffffffffLL-1)

#define INT_MAX 0x7fff
#define ULONG_MAX 0xffffffffL

#endif /* EC_I386_LIMITS_H */
