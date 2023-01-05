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
])
