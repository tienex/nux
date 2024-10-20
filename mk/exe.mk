#
# exe.mk
# Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>
#  SPDX-License-Identifier:	GPL2.0+
#

.PHONY: program_clean

ifneq (z$(LINKERSCRIPT),z)
LDFLAGS+=-T$(LINKERSCRIPT)
endif

$(PROGRAM): $(OBJS) $(LIBDEPS) $(LINKERSCRIPT)
	$(CC) $(LDADD_START) $(LDFLAGS) \
	-Wl,--start-group $(OBJS) $(LDADD) -Wl,--end-group \
	$(LDADD_END) -o $@

program_clean:
	-rm $(PROGRAM)

program_install:
	install -d $(INSTALLDIR)/$(PROGDIR)
	install -m 0544 $(PROGRAM) $(INSTALLDIR)/$(PROGDIR)/$(PROGRAM)

ALL_TARGET+=$(PROGRAM)
CLEAN_TARGET+=program_clean
ifeq (z$(NOINST),z)
INSTALL_TARGET+=program_install
endif
