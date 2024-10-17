AC_DEFUN([AC_LIBEC_DIR],
[
	AC_LIB_DIR(libec, $1)
	AC_SUBST(EMBED_LIBEC, ["include "'$(BUILDROOT)'"/mk/libec-include.mk"])
	AC_CONFIG_FILES([mk/libec-embed.mk:$1/libec-embed.mk.in])
])
