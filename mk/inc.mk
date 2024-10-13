
#
# inc.mk
# Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>
#  SPDX-License-Identifier:	GPL2.0+
#

.PHONY: all includes incdir incs buildincs

ifneq ($(INCDIR)z,z)
INSTALL_TARGET+= incdir
incdir:
	install -d ${INSTALLDIR}/${INCDIR}
else
incdir:

endif

ifneq ($(BUILDINCS)z,z)
INSTALL_TARGET+= buildincs
buildincs:
	install -m 0644 $(addprefix $(BUILDDIR), $(BUILDINCS)) ${INSTALLDIR}/${INCDIR}/
else
buildincs:

endif

ifneq ($(INCS)z,z)
INSTALL_TARGET+= incs
incs:
	echo srcdir is $(SRCDIR)
	echo $(addprefix $(SRCDIR),$(INCS))
	install -m 0644 $(addprefix $(SRCDIR),$(INCS)) ${INSTALLDIR}/${INCDIR}/ 
else
incs:

endif

ifneq ($(INCSUBDIRS)z,z)
INSTALL_TARGET+= includes
includes:
	for dir in $(INCSUBDIRS); do $(MAKE) -C $$dir includes incdir incs; done
else
includes:

endif
