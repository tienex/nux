#
# top.mk
# Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>
#  SPDX-License-Identifier:	GPL2.0+
#

.PHONY: all install clean
CC=@TARGET_CC@
CCLD=@TARGET_CC@
LD=@TARGET_LD@
AR=@TARGET_AR@
OBJCOPY=@TARGET_OBJCOPY@
OBJDIR=@builddir@/@MACHINE@
SRCDIR=@srcdir@/
SRCROOT=@top_srcdir@/
BUILDROOT=@top_builddir@/
MKDIR=$(SRCROOT)/@mk_dir@
INSTALLDIR=@INSTALLDIR@
CFLAGS+=@CONFIGURE_FLAGS@

all: do_all

