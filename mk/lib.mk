.PHONY: clean_lib$(LIBRARY) install_lib$(LIBRARY)

lib$(LIBRARY).a: $(OBJS)
ifneq ($(OBJS)z,z)
	$(AR) r $@ $(OBJS)
endif

install_lib$(LIBRARY): lib$(LIBRARY).a $(EXTOBJS)
ifeq ($(NOINST)z, z)
ifneq ($(OBJS)z,z)
	install -d $(INSTALLDIR)/$(LIBDIR)
	install -m 0644 lib$(LIBRARY).a  $(INSTALLDIR)/$(LIBDIR)/lib$(LIBRARY).a
endif
ifneq ($(EXTOBJS)z,z)
	install -m 0644 $(EXTOBJS) $(INSTALLDIR)/$(LIBDIR)/
endif
endif

clean_lib$(LIBRARY):
	-rm $(EXTOBJS) lib$(LIBRARY).a

ALL_TARGET+= lib$(LIBRARY).a
CLEAN_TARGET+= clean_lib$(LIBRARY)
INSTALL_TARGET+= install_lib$(LIBRARY)
