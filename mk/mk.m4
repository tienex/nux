AC_DEFUN([AC_MK_DIR],
[
	mk_dir=$1
	AC_SUBST(mk_dir, ${mk_dir})
])

AC_DEFUN([AC_MK_CONFIG_FILE],
[
	AC_CONFIG_FILES([$1:mk/top.mk:$1.in:mk/bottom.mk])
])

AC_DEFUN([AC_MK_CONFIG_FILES],
[
	m4_map_args_w($@, [AC_MK_CONFIG_FILE(], [)])
])

