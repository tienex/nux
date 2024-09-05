/*
  EC - An embedded non standard C library

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef EC_CDEFS_ELF_H
#define EC_CDEFS_ELF_H

#include <machine/asm.h>

#define __weak_alias(alias,sym)					\
  __asm(".weak " _C_LABEL_STRING(alias) "\n"			\
	_C_LABEL_STRING(alias) " = " _C_LABEL_STRING(sym))

#define __alias(alias,sym)					\
  __asm(".globl " _C_LABEL_STRING(alias) "\n"			\
	_C_LABEL_STRING(alias) " = " _C_LABEL_STRING(sym))


#define	__weak_reference(sym)	__attribute__((__weakref__(#sym)))

#define	__weakref_visible	static

#endif /* EC_CDEFS_ELF_H */
