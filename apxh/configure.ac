AC_INIT([APXH],[0.0])
AC_CONFIG_MACRO_DIRS([../mk])
AC_MK_DIR([../mk])

AC_CONFIG_MACRO_DIRS([../libec])
AC_LIBEC_DIR([../libec])

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
	[i386], [apxh_subdirs=multiboot],
	[amd64], [apxh_subdirs=multiboot])

AC_SUBST(MACHINE, ${machine})
AC_SUBST(CONFIGURE_FLAGS, ["${optflags} -I\$(SRCROOT)../include"])
AC_SUBST(APXH_SUBDIRS, [${apxh_subdirs}])

optflags='-O7 -g'

AC_MK_CONFIG_FILES([
	Makefile
	efi/Makefile
	efi/apxhefi/Makefile
	multiboot/Makefile
])


AC_OUTPUT
