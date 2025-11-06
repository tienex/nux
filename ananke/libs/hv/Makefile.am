# Makefile.am for Ananke Hypervisor Framework

lib_LTLIBRARIES = libhv.la

# Public headers
hvincludedir = $(includedir)/hv
hvinclude_HEADERS = \
	include/hv/hypervisor.h

# Source files
libhv_la_SOURCES = \
	src/hypervisor.c \
	src/hypervisor_impl.h \
	src/vm.c \
	src/vcpu.c \
	src/vmem.c \
	src/tc_cache.c \
	src/arch/x86_backend.c \
	src/arch/backends_stub.c \
	src/arch/vtx_support.c \
	src/devices/virtual_device.c \
	src/devices/virtual_disk.c \
	src/devices/virtual_network.c \
	src/devices/virtual_serial.c \
	src/paravirt/mol_drivers.c

# Compiler flags
libhv_la_CFLAGS = \
	-I$(top_srcdir)/ananke/include \
	-I$(top_srcdir)/ananke/libs/hv/include \
	-I$(top_srcdir)/ananke/libs/ntrtl/include \
	-std=c11 \
	-Wall \
	-Wextra \
	-Werror=implicit-function-declaration \
	-fno-strict-aliasing

# Linker flags
libhv_la_LDFLAGS = \
	-version-info 1:0:0 \
	-no-undefined

# Dependencies
libhv_la_LIBADD = \
	$(top_builddir)/ananke/libs/ntrtl/libntrtl.la

# Example program (optional)
if BUILD_EXAMPLES
noinst_PROGRAMS = hv_test

hv_test_SOURCES = example/hv_test.c
hv_test_CFLAGS = $(libhv_la_CFLAGS)
hv_test_LDADD = libhv.la
endif

# Documentation
EXTRA_DIST = \
	README.md \
	INTEGRATION.md \
	example/hv_test.c

# Clean generated files
CLEANFILES = *.lo *.la

# Maintainer mode
MAINTAINERCLEANFILES = \
	Makefile.in
