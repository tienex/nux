#ifndef EC_CDEFS_ELF_H
#define EC_CDEFS_ELF_H

#include <machine/asm.h>

#define __decl_weak_alias(alias,sym)				\
  __asm(".weak " _C_LABEL_STRING(alias) "\n"			\
	_C_LABEL_STRING(alias) " = " _C_LABEL_STRING(sym))

#define __decl_alias(alias,sym)					\
  __asm(".globl " _C_LABEL_STRING(alias) "\n"			\
	_C_LABEL_STRING(alias) " = " _C_LABEL_STRING(sym))

#define __weak_alias(x) __attribute__((weak, alias (#x)))
#define __alias(x) __attribute__((alias (#x)))

#endif /* EC_CDEFS_ELF_H */
