AC_DEFUN([AC_LIBEC_DIR],
[
	ecdir=$1
	AC_SUBST(ECDIR, '$(SRCROOT)'${ecdir})
	AC_SUBST(COMPILE_LIBEC, ["include "'$(SRCROOT)'"/libec-compile.mk"])
	AC_SUBST(LINK_LIBEC, ["include "'$(SRCROOT)'"/libec.mk"])
	AC_CONFIG_FILES([libec.mk:$1/libec.mk.in])
	AC_CONFIG_FILES([libec-compile.mk:$1/libec-compile.mk.in])
])
