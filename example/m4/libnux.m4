# Define the location of LIBNUX sources. This will create 'libnux.mk'
# in the directory where configure is running.
AC_DEFUN([AC_LIBNUX_DIR],
[
	AC_SUBST(NUXROOT, $1)
	AC_SUBST(NUXDIR, $1/libnux)
	AC_SUBST(COMPILE_LIBNUX, ["include "'$(SRCROOT)'"/libnux-compile.mk"])
	AC_SUBST(LINK_LIBNUX, ["include "'$(SRCROOT)'"/libnux.mk"])
	AC_CONFIG_FILES([libnux.mk:$1/libnux/libnux.mk.in])
	AC_CONFIG_FILES([libnux-compile.mk:$1/libnux/libnux-compile.mk.in])

	# libnux_user
	AC_SUBST(NUXUSERDIR, $1/libnux_user)
	AC_SUBST(COMPILE_LIBNUXUSER, ["include "'$(SRCROOT)'"/libnux_user-compile.mk"])
	AC_SUBST(LINK_LIBNUXUSER, ["include "'$(SRCROOT)'"/libnux_user.mk"])
	AC_CONFIG_FILES([libnux_user.mk:$1/libnux_user/libnux_user.mk.in])
	AC_CONFIG_FILES([libnux_user-compile.mk:$1/libnux_user/libnux_user-compile.mk.in])

])
