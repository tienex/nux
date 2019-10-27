.PHONY: all install clean
CC=@CC@
OBJDIR=.build
SRCROOT=@top_srcdir@
MKDIR=$(SRCROOT)/@mk_dir@
INSTALLDIR=@INSTALLDIR@
CFLAGS+=@CONFIGURE_FLAGS@

all: do_all
