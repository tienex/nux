#
# NUX: A kernel Library
#
#  SPDX-License-Identifier:	GPL-2.0+
#
#

MACHINE= amd64

ifeq ($(MACHINE),amd64)
CFLAGS+= -mcmodel=large -mno-red-zone
endif

CFLAGS+= -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-avx -mno-avx2

LDFILE=$(MACHINE)/exe.ld

