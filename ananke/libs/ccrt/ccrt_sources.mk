## @file
#  cCRT - Compiler C Runtime Library
#
#  Copyright (C) 2025 A•NUX Project
#
#  SPDX-License-Identifier: BSD-2-Clause
##

CCRT_INTRINSICS_SRCS = \
	intrinsics/popcountdi2.c \
	intrinsics/popcountsi2.c \
	intrinsics/udivmoddi4.c

CCRT_SRCS = $(CCRT_INTRINSICS_SRCS)
