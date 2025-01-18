#
# EC - An embedded non standard C library
#

CFLAGS += -D_STANDALONE

EXTRA_DIST= amd64/crti.S amd64/crtn.S
EXTRA_DIST+= i386/crti.S i386/crtn.S i386/crtbegin.S i386/crtend.S

noinst_HEADERS = assert.h cdefs.h cdefs_elf.h errno.h inttypes.h	\
		queue.h rbtree.h setjmp.h stdarg.h stdbool.h stddef.h	\
		stdio.h stdint.h stdlib.h string.h

noinst_HEADERS +=i386/ansi.h i386/asm.h i386/inttypes.h	\
		i386/setjmp.h i386/stdint.h

noinst_HEADERS +=amd64/ansi.h amd64/asm.h amd64/inttypes.h	\
		amd64/setjmp.h amd64/stdint.h

