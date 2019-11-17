#
# fatojb.mk
# Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>
#  SPDX-License-Identifier:	GPL2.0+
#

.PHONY: clean_lib$(FATOBJ)

$(FATOBJ).o: $(OBJDIR) $(OBJS)
ifneq ($(OBJS)z,z)
	$(CCLD) -r $(LDFLAGS) -o $@ $(OBJS) $(LDADD)
endif

clean_$(FATOBJ):
	-rm $(FATOBJ).o

ALL_TARGET+= $(FATOBJ).o
CLEAN_TARGET+= clean_$(FATOBJ)
