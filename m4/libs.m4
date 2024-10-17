AC_DEFUN([AC_LIB_DIR],
	[
	AC_SUBST(m4_toupper($1)_SRCDIR, '$(SRCROOT)'/$2)
	AC_SUBST(m4_toupper($1)_BUILDDIR, '$(BUILDROOT)'/$2)
	AC_SUBST(COMPILE_[]m4_toupper($1), ["include "'$(BUILDROOT)'"/mk/$1-compile.mk"])
	AC_SUBST(LINK_[]m4_toupper($1), ["include "'$(BUILDROOT)'"/mk/$1-link.mk"])
	AC_CONFIG_FILES([mk/$1-compile.mk:$2/$1-compile.mk.in])
	AC_CONFIG_FILES([mk/$1-link.mk:$2/$1-link.mk.in])
])

AC_DEFUN([AC_CONTRIBLIB_DIR],
[
	AC_LIB_DIR($1, $2)
	AC_SUBST(m4_toupper($1)[]_CONTRIBDIR, $3)
])
