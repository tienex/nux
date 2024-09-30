# Define the location of LIBEC sources. This will create 'libec.mk'
# and 'libec-compile.mk' in the directory where configure is running.
AC_DEFUN([AC_LIBEC_DIR],
[
	ecdir=$1
	AC_SUBST(ECSRCDIR, '$(SRCROOT)'/${ecdir})
	AC_SUBST(ECBUILDDIR, '$(BUILDROOT)'/${ecdir})
	AC_SUBST(COMPILE_LIBEC, ["include "'$(BUILDROOT)'"/mkgen/libec-compile.mk"])
	AC_SUBST(LINK_LIBEC, ["include "'$(BUILDROOT)'"/mkgen/libec.mk"])
	AC_SUBST(EMBED_LIBEC, ["include "'$(BUILDROOT)'"/mkgen/libec-include.mk"])
	AC_CONFIG_FILES([mkgen/libec.mk:$1/libec.mk.in])
	AC_CONFIG_FILES([mkgen/libec-compile.mk:$1/libec-compile.mk.in])
	AC_CONFIG_FILES([mkgen/libec-include.mk:$1/libec-include.mk.in])
])
