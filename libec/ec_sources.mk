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

EC_SRCS = 	crt0-common.c \
		printf.c \
		rb.c \
		strcspn.c \
		strlcpy.c \
		atexit.c \
		$(EC_ARCH_SRCS)

