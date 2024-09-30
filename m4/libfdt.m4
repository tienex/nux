AC_DEFUN([AC_LIBFDT_DIR],
[
	AC_SUBST(FDTROOT, $1)
	AC_SUBST(FDTDIR, $1)
	AC_SUBST(DTCDIR, $2)
	AC_SUBST(COMPILE_LIBFDT, ["include "'$(BUILDROOT)'"/mkgen/libfdt-compile.mk"])
	AC_SUBST(LINK_LIBFDT, ["include "'$(BUILDROOT)'"/mkgen/libfdt.mk"])
	AC_CONFIG_FILES([mkgen/libfdt.mk:$1/libfdt.mk.in])
	AC_CONFIG_FILES([mkgen/libfdt-compile.mk:$1/libfdt-compile.mk.in])
])
