#
# NUX: A kernel Library.
# Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>
#
#  SPDX-License-Identifier:	GPL2.0+
#

.PHONY: all install clean
CC=@CC@
OBJDIR=.build/@MACHINE@/
SRCROOT=@top_srcdir@
MKDIR=$(SRCROOT)/@mk_dir@
INSTALLDIR=@INSTALLDIR@
CFLAGS+=@CONFIGURE_FLAGS@

all: do_all
