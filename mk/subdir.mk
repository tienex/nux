#
# NUX: A kernel Library.
# Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>
#
#  SPDX-License-Identifier:	GPL2.0+
#

.PHONY : subdirs clean_subdirs install_subdirs $(SUBDIRS)
ALL_TARGET+= subdirs
CLEAN_TARGET+= clean_subdirs
INSTALL_TARGET+= install_subdirs

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ all

clean_subdirs:
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean ; done

install_subdirs: subdirs
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir install; done
