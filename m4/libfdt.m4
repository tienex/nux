AC_DEFUN([AC_LIBFDT_DIR],
[
	AC_SUBST(FDTROOT, $1)
	AC_SUBST(FDTDIR, $1)
	AC_SUBST(DTCDIR, $2)
	AC_SUBST(COMPILE_LIBFDT, ["include "'$(SRCROOT)/$(NUXROOT)'"/libfdt-compile.mk"])
	AC_SUBST(LINK_LIBFDT, ["include "'$(SRCROOT)/$(NUXROOT)'"/libfdt.mk"])
	AC_CONFIG_FILES([libfdt.mk:$1/libfdt.mk.in])
	AC_CONFIG_FILES([libfdt-compile.mk:$1/libfdt-compile.mk.in])

])
