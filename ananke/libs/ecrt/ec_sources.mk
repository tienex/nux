## @file
#  eCRT - An embedded C runtime library
#
#  Copyright (C) 2025 A•NUX Project
#
#  SPDX-License-Identifier: BSD-2-Clause
##

ifeq ($(eCRT_MACHINE),i386)
eCRT_ARCH_SRCS= i386/_setjmp.S i386/atomic.S i386/ffs.S i386/fls.S	\
		i386/memcmp.S i386/memcpy.S i386/memmove.S		\
		i386/memset.S i386/strlen.S i386/strncmp.S		\
		i386/memchr.S i386/strchr.S i386/strrchr.S
endif

ifeq ($(eCRT_MACHINE),amd64)
eCRT_ARCH_SRCS=  amd64/_setjmp.S amd64/ffs.S amd64/fls.S \
		amd64/memcmp.S amd64/memcpy.S amd64/memmove.S		\
		amd64/memset.S amd64/strlen.S amd64/strncmp.S		\
		amd64/memchr.S amd64/strchr.S amd64/strrchr.S
endif

ifeq ($(eCRT_MACHINE),riscv64)
eCRT_ARCH_SRCS=	riscv64/_setjmp.S \
		memcmp.c memcpy.c memmove.c \
		memset.c strlen.c strncmp.c \
		memchr.c strchr.c strrchr.c
endif

eCRT_INTRINSICS_SRCS+= \
	intrinsics/popcountdi2.c \
	intrinsics/popcountsi2.c \
	intrinsics/udivmoddi4.c

eCRT_SRCS = 	crt0-common.c \
		printf.c \
		snprintf.c \
		subr_prf.c \
		rb.c \
		strtoul.c \
		strcspn.c \
		strncpy.c \
		strlcpy.c \
		strnlen.c \
		atexit.c \
		ctype.c \
		$(eCRT_ARCH_SRCS) \
		$(eCRT_INTRINSICS_SRCS)
