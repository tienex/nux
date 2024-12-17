#
# NUX: A kernel Library.
# Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>
#
#  SPDX-License-Identifier:	BSD-2-Clause
#
#

PROGRAM=$(NUX_KERNEL)_nosym


__nux_syms.c: $(PROGRAM)
	$(NUXSRCROOT)/tools/mksyms/mksyms.sh $(PROGRAM) $@

__nux_syms.o: __nux_syms.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

$(NUX_KERNEL): __nux_syms.o $(PROGRAM)
	$(CC) $(LDADD_START) $(LDFLAGS) \
	-Wl,--start-group $(OBJS) $(LDADD) -Wl,--end-group \
	$(LDADD_END) __nux_syms.o -o $@


.PHONY: program_clean
nuxkernel_clean:
	-rm __nux_syms.o __nux_syms.c $(NUX_KERNEL)

ALL_TARGET+= $(NUX_KERNEL)
CLEAN_TARGET+= nuxkernel_clean
