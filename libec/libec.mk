#
# EC - An embedded non standard C library
#
# This is the automake include. Can only be included through
# the infamous and terribly specific automake rules:
#
#    include $(srcdir)/file
#    include $(top_srcdir)/file
#
# i.e., only if they are under your own configure script.
#
# To embed libec in your target, include this file,
# and add $(EC_SRCS) to your sources.
#

ifeq (z$(EC_MACHINE),z)
EC_MACHINE=i386
endif

EC_DIR=$(SRCROOT)/libec

EC_ARCH_DIR= $(EC_MACHINE)

ifeq ($(EC_MACHINE),i386)
EC_ARCH_SRCS= i386/_setjmp.S i386/atomic.S i386/ffs.S i386/fls.S	\
		i386/memcmp.S i386/memcpy.S i386/memmove.S		\
		i386/memset.S i386/strlen.S i386/strncmp.S
endif

ifeq ($(EC_MACHINE),amd64)
EC_ARCH_SRCS=  amd64/_setjmp.S amd64/ffs.S amd64/fls.S \
		amd64/memcmp.S amd64/memcpy.S amd64/memmove.S		\
		amd64/memset.S amd64/strlen.S amd64/strncmp.S
endif

include $(EC_DIR)/libec-compile.mk

EC_SRCS = 	crt0-common.c \
		printf.c \
		rb.c \
		strcspn.c \
		strlcpy.c \
		atexit.c \
		$(EC_ARCH_SRCS)

# Used for crt* asm files
%.o: $(EC_DIR)/$(EC_ARCH_DIR)/%.S
	$(CC) -c $(CPPFLAGS) $(ASFLAGS) $< -o $@

LDADD_START+= crti.o crtbegin.o
LDADD_END+= crtn.o crtend.o
LDADD+= -lgcc

SRCS+= $(addprefix $(EC_DIR)/,$(EC_SRCS))
ALL_TARGET+= crti.o crtn.o crtbegin.o crtend.o
CLEAN_FILES+= crti.o crtn.o crtbegin.o crtend.o



#
# Compile an executable with embedded libc.
#
# To use this rule you need to first create libPROGNAME, and then
# define a PROGNAME$(EXEEXT) target.
# You also need to create a target-specific crt0.o
#
%$(EXEEXT): lib%.a crt0.o crti.o crtbegin.o crtend.o crtn.o
	$(CC) crt0.o crti.o crtbegin.o \
	$(AM_LDFLAGS) $(EC_LDFLAGS) $(LDFLAGS) \
	-Wl,--start-group $< $(AM_LDADD) -lgcc -Wl,--end-group \
	crtend.o crtn.o \
	-o $@
