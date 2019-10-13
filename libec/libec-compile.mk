CFLAGS+= -nostdinc -fno-builtin
LDFLAGS+= -nostdlib
CPPFLAGS+= -D_EC_SOURCE -I$(SRCROOT)/libec
ASFLAGS+= -D_ASSEMBLER
