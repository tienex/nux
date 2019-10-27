AC_INIT([NUX],[0.0])
AC_CONFIG_MACRO_DIR([mk])
AC_MK_DIR([mk])

#Allow AC_PROG_CC test to succeed on cross compilers.
CFLAGS=-nostdlib
AC_PROG_CC([gcc])

AS_CASE([$host],
	[i?86-*], [machine=i386],
	[amd64-*], [machine=amd64],
	[x86_64-*], [machine=amd64])

AS_CASE([$machine],
	[i386], [hal=hal_x86 plt=plt_acpi],
	[amd64], [hal=hal_amd64 plt=plt_acpi])

ecdir='$(SRCROOT)'/libec
optflags='-O7 -g'
#optflags='-O0 -g'

AC_SUBST(MACHINE, ${machine})
AC_SUBST(HALDIR, lib${hal})
AC_SUBST(PLTDIR, lib${plt})
AC_SUBST(INSTALLDIR, "$(pwd)"/dist)

AC_SUBST(ECDIR, ${ecdir})
AC_SUBST(COMPILE_LIBEC, ["include ${ecdir}/libec-compile.mk"])
AC_SUBST(LINK_LIBEC, ["include ${ecdir}/libec.mk"])

AC_SUBST(CONFIGURE_FLAGS, ["${optflags}"])

AC_CONFIG_FILES([libec/libec.mk libec/libec-compile.mk])
AC_MK_CONFIG_FILES([
	Makefile
	apxh/Makefile
	apxh/multiboot/Makefile
	lib${hal}/Makefile
	lib${plt}/Makefile
	libnux/Makefile
	example/Makefile
])

AC_OUTPUT
