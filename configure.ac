AC_INIT([NUX],[0.0])
AC_CONFIG_MACRO_DIRS([mk])
AC_MK_DIR([mk])

AC_CONFIG_MACRO_DIRS([libec])
AC_LIBEC_DIR([libec])

#Allow AC_PROG_CC test to succeed on cross compilers.
CFLAGS=-nostdlib
AC_PROG_CC([gcc])
AC_CHECK_TOOL(LD, ld, :)
AC_CHECK_TOOL(AR, ar, :)

AS_CASE([$host],
	[i?86-*], [machine=i386],
	[amd64-*], [machine=amd64],
	[x86_64-*], [machine=amd64])

AS_CASE([$machine],
	[i386], [hal=hal_x86 plt=plt_acpi],
	[amd64], [hal=hal_x86 plt=plt_acpi])


optflags='-O7 -g'
#optflags='-O0 -g'

AC_SUBST(MACHINE, ${machine})
AC_SUBST(HALDIR, lib${hal})
AC_SUBST(PLTDIR, lib${plt})
AC_SUBST(INSTALLDIR, "$(pwd)"/dist)


nuxdir='$(SRCROOT)'/libnux
AC_SUBST(COMPILE_LIBNUX, ["include ${nuxdir}/libnux-compile.mk"])
AC_SUBST(LINK_LIBNUX, ["include ${nuxdir}/libnux.mk"])

AC_SUBST(CONFIGURE_FLAGS, ["${optflags} -DNUX_MACHINE=${machine}"])

AS_CASE([$hal],
	[hal_x86], [extraconf=libhal_x86/hal.mk])

AC_CONFIG_FILES([libec/libec.mk libec/libec-compile.mk libnux/libnux.mk libnux/libnux-compile.mk ${extraconf}])
AC_MK_CONFIG_FILES([
	Makefile
	lib${hal}/Makefile
	lib${plt}/Makefile
	libnux/Makefile
	libnux_user/Makefile
	example/Makefile
	example/kern/Makefile
	example/user/Makefile
])

AC_CONFIG_SUBDIRS([apxh])

AC_OUTPUT
