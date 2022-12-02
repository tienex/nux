AC_INIT([NUX],[0.0])

AC_CONFIG_MACRO_DIRS([mk])
AC_MK_DIR([mk])

AC_CONFIG_MACRO_DIRS([libec])
AC_LIBEC_DIR([libec])

AC_PREFIX_DEFAULT([${PWD}/build/])

AC_ARG_VAR([ARCH], [The architecture to compile for: i386 or amd64])

if test x"${ARCH}" == x ; then
   AC_MSG_ERROR([Please select a valid architecture with `./configure ARCH=<arch>`])
fi

AS_CASE([${ARCH}],
	[i386], [tool_prefix=i686-elf],
	[amd64], [tool_prefix=x86_64-elf],
	[*], [AC_MSG_ERROR([Supported Architectures: i386, amd64])])


AC_CHECK_TOOL(CC, ${tool_prefix}-gcc, :)
AC_CHECK_TOOL(LD, ${tool_prefix}-ld, :)
AC_CHECK_TOOL(AR, ${tool_prefix}-ar, :)


machine=${ARCH}

AS_CASE([$machine],
	[i386], [hal=hal_x86 plt=plt_acpi],
	[amd64], [hal=hal_x86 plt=plt_acpi])


AC_ARG_ENABLE([opt],
	AS_HELP_STRING([--disable-opt],[compile without compiler optimizations]))
AS_IF([test "x$enable_opt" != "xno"],
	[optflags='-O7 -g'],
	[optflags='-O0 -g'])

debug=1
AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--disable-debug],[compile without debug code and messages]))
AS_IF([test "x$enable_debug" != "xno"],
	[debug=1],
	[debug=0])
AC_SUBST(DEBUG, ${debug})

plt_verbose = 0
AC_ARG_ENABLE([plt_verbose],
	AS_HELP_STRING([--enable-plt-verbose],[print extra information from the plt module]))
AS_IF([test "x$enable_plt_verbose" != "xyes"],
	[plt_verbose=0],
	[plt_verbose=1])
AC_SUBST(PLT_VERBOSE, ${plt_verbose})

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

AC_CONFIG_FILES([include/config.h libec/libec.mk libec/libec-compile.mk libnux/libnux.mk libnux/libnux-compile.mk ${extraconf}])
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
