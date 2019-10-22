.PHONY: all install clean
CC=@CC@
OBJDIR=.build
SRCROOT=@top_srcdir@
MKDIR=$(SRCROOT)/@mk_dir@
INSTALLDIR=@INSTALLDIR@
CFLAGS+=@OPTIMIZE_FLAGS@ -fno-strict-aliasing

all: do_all
