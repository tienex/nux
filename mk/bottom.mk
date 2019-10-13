
ifneq (z$(SUBDIRS),z)
include $(MKDIR)/subdir.mk
endif

ifneq (z$(SRCS),z)
include $(MKDIR)/obj.mk
else
ifneq (z$(CUSTOBJS),z)
include $(MKDIR)/obj.mk
endif
endif

ifneq (z$(LIBRARY),z)
include $(MKDIR)/lib.mk
endif

ifneq (z$(PROGRAM),z)
include $(MKDIR)/exe.mk
endif

include $(MKDIR)/inc.mk

ifneq (z$(ALL_TARGET),z)
do_all: $(ALL_PREDEP)
	$(MAKE) $(ALL_TARGET)
else
do_all: $(ALL_PREDEP)
endif

install: $(INSTALL_TARGET)

clean: $(CLEAN_TARGET)
ifneq (z$(CLEAN_FILES),z)
	-rm $(CLEAN_FILES)
endif

