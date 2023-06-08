AC_INIT([NUX],[0.0])

AC_CONFIG_MACRO_DIRS([m4])

AC_MK_DIR([mk])
AC_LIBEC_DIR([libec])
AC_LIBNUX_DIR([.])

AC_PREFIX_DEFAULT([${PWD}/build/])

AC_ARG_VAR([ARCH], [The architecture to compile for: i386 or amd64])
if test x"${ARCH}" == x ; then
   AC_MSG_ERROR([Please select a valid architecture with `./configure ARCH=<arch>`])
fi

AC_ARG_VAR([TOOLCHAIN_VENDOR],[toolchain vendor prefix @<:@default: unknown-elf@:>@])

AS_CASE([${ARCH}],
	[i386], [tool_prefix=i686-"${TOOLCHAIN_VENDOR}"],
	[amd64], [tool_prefix=x86_64-"${TOOLCHAIN_VENDOR}"],
	[*], [AC_MSG_ERROR([Supported Architectures: i386, amd64])])

AC_CHECK_TOOL(CC, ${tool_prefix}-gcc, :)
if test x"${CC}" = 'x:' ; then
  AC_MSG_ERROR([${tool_prefix}-gcc not found])
fi

AC_CHECK_TOOL(LD, ${tool_prefix}-ld, :)
if test x"${LD}" = 'x:' ; then
  AC_MSG_ERROR([${tool_prefix}-ld not found])
fi

AC_CHECK_TOOL(AR, ${tool_prefix}-ar, :)
if test x"${AR}" = 'x:' ; then
  AC_MSG_ERROR([${tool_prefix}-ar not found])
fi

AC_CHECK_TOOL(OBJCOPY, ${tool_prefix}-objcopy, :)
if test x"${OBJCOPY}" = 'x:' ; then
  AC_MSG_ERROR([${tool_prefix}-objcopy not found])
fi

machine=${ARCH}

AS_CASE([$machine],
	[i386], [hal=hal_x86 plt=plt_acpi],
	[amd64], [hal=hal_x86 plt=plt_acpi])


AC_ARG_ENABLE([opt],
	AS_HELP_STRING([--disable-opt],[compile without compiler optimizations]))
AS_IF([test "x$enable_opt" != "xno"],
	[optflags='-O7 -g'],
	[optflags='-O0 -g'])

AC_ARG_ENABLE([werror],
	AS_HELP_STRING([--disable-werror],[compile ignoring compiler warnings]))
AS_IF([test "x$enable_relax" != "xno"],
	[wflags='-Werror'],
	[wflags=''])

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

AC_SUBST(CONFIGURE_FLAGS, ["${optflags} ${wflags} -DNUX_MACHINE=${machine}"])

AS_CASE([$hal],
	[hal_x86], [extraconf=libhal_x86/hal.mk])

AC_CONFIG_FILES([include/config.h libnux/cfg.mk ${extraconf}])
AC_MK_CONFIG_FILES([
	Makefile
	lib${hal}/Makefile
	lib${plt}/Makefile
	libnux/Makefile
	libnux_user/Makefile
	example/Makefile
	example/kern/Makefile
	example/user/Makefile
	tools/Makefile
	tools/libbfd/Makefile
	tools/nuxar/Makefile
])

AC_CONFIG_SUBDIRS([apxh example])

AC_OUTPUT
