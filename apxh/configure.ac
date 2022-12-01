AC_INIT([APXH],[0.0])
AC_CONFIG_MACRO_DIRS([../mk])
AC_MK_DIR([../mk])

AC_CONFIG_MACRO_DIRS([../libec])
AC_LIBEC_DIR([../libec])

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
	[i386], [apxh_subdirs=multiboot],
	[amd64], [apxh_subdirs="multiboot efi"])


AC_ARG_ENABLE([opt],
	AS_HELP_STRING([--disable-opt],[compile without compiler optimizations]))

AS_IF([test "x$enable_opt" != "xno"],
	[optflags='-O7 -g'],
	[optflags='-O0 -g'])


AC_SUBST(MACHINE, ${machine})
AC_SUBST(CONFIGURE_FLAGS, ["${optflags} -I\$(SRCROOT)../include"])
AC_SUBST(APXH_SUBDIRS, [${apxh_subdirs}])

AC_MK_CONFIG_FILES([
	Makefile
	efi/Makefile
	efi/apxhefi/Makefile
	multiboot/Makefile
])


AC_OUTPUT
