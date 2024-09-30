# Define the location of LIBNUX sources. This will create 'libnux.mk'
# in the directory where configure is running.
AC_DEFUN([AC_LIBNUX_DIR],
[
	AC_SUBST(NUXSRCROOT, '$(SRCROOT)'/$1)
	AC_SUBST(NUXBUILDROOT, '$(BUILDROOT)'/$1)
	AC_SUBST(NUXSRCDIR, '$(SRCROOT)'/$1/libnux)
	AC_SUBST(NUXBUILDDIR, '$(BUILDROOT)'/$1/libnux)
	AC_SUBST(COMPILE_LIBNUX, ["include "'$(BUILDROOT)'"/mkgen/libnux-compile.mk"])
	AC_SUBST(LINK_LIBNUX, ["include "'$(BUILDROOT)'"/mkgen/libnux.mk"])
	AC_CONFIG_FILES([mkgen/libnux.mk:$1/libnux/libnux.mk.in])
	AC_CONFIG_FILES([mkgen/libnux-compile.mk:$1/libnux/libnux-compile.mk.in])

	# libnux_user
	AC_SUBST(NUXUSERSRCDIR, $(SRCDIR)/$1/libnux_user)
	AC_SUBST(NUXUSERBUILDDIR, $(BUILDDIR)/$1/libnux_user)
	AC_SUBST(COMPILE_LIBNUXUSER, ["include "'$(BUILDROOT)'"/mkgen/libnux_user-compile.mk"])
	AC_SUBST(LINK_LIBNUXUSER, ["include "'$(BUILDROOT)'"/mkgen/libnux_user.mk"])
	AC_CONFIG_FILES([mkgen/libnux_user.mk:$1/libnux_user/libnux_user.mk.in])
	AC_CONFIG_FILES([mkgen/libnux_user-compile.mk:$1/libnux_user/libnux_user-compile.mk.in])

])
