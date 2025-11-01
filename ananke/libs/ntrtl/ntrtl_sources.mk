## @file
#  NTRTL - NT Runtime Library
#
#  Copyright (C) 2025 A•NUX Project
#
#  SPDX-License-Identifier: BSD-2-Clause
##

# Architecture-specific optimized implementations
ifeq ($(NTRTL_MACHINE),i386)
NTRTL_ARCH_SRCS = \
	arch/i386/memchr.S \
	arch/i386/memcmp.S \
	arch/i386/memcpy.S \
	arch/i386/memmove.S \
	arch/i386/memset.S \
	arch/i386/strchr.S \
	arch/i386/strlen.S \
	arch/i386/strncmp.S \
	arch/i386/strrchr.S
endif

ifeq ($(NTRTL_MACHINE),amd64)
NTRTL_ARCH_SRCS = \
	arch/amd64/memchr.S \
	arch/amd64/memcmp.S \
	arch/amd64/memcpy.S \
	arch/amd64/memmove.S \
	arch/amd64/memset.S \
	arch/amd64/strchr.S \
	arch/amd64/strlen.S \
	arch/amd64/strncmp.S \
	arch/amd64/strrchr.S
endif

ifeq ($(NTRTL_MACHINE),riscv64)
# RISC-V uses portable C implementations
NTRTL_ARCH_SRCS =
endif

# Portable C implementations
NTRTL_C_SRCS = \
	bitmap.c \
	list.c \
	memory.c \
	string.c \
	tree.c \
	utility.c

NTRTL_SRCS = $(NTRTL_C_SRCS) $(NTRTL_ARCH_SRCS)
