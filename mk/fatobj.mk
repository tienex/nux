#
# fatojb.mk
# Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>
#  SPDX-License-Identifier:	BSD-2-Clause
#

.PHONY: clean_lib$(FATOBJ)

$(OBJDIR)/$(FATOBJ).o: $(OBJS)
ifneq ($(OBJS)z,z)
	$(CCLD) -r $(LDFLAGS) -o $@ $(OBJS) $(LDADD)
endif

clean_$(FATOBJ):
	-rm $(OBJDIR)/$(FATOBJ).o

ALL_TARGET+= $(OBJDIR)/$(FATOBJ).o
CLEAN_TARGET+= clean_$(FATOBJ)
