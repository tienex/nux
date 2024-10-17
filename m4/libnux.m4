# Define the location of LIBNUX sources. This will create 'libnux.mk'
# in the directory where configure is running.
AC_DEFUN([AC_NUXROOT_DIR],
[
	AC_SUBST(NUXSRCROOT, '$(SRCROOT)'/$1)
	AC_SUBST(NUXBUILDROOT, '$(BUILDROOT)'/$1)
	AC_LIB_DIR(libnux, $1/libnux)

	# libnux_user
	AC_LIB_DIR(libnux_user, $1/libnux_user)
])
