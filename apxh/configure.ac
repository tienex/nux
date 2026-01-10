AC_INIT([APXH],[0.0])

AC_ARG_WITH([nux],
	[AS_HELP_STRING(
		[--with-nux],
		[specify location of the NUX repository @<:@default=..@:>@])],
	[],[with_nux=..])
nux_dir=${with_nux}

AC_ARG_WITH([libec],
	[AS_HELP_STRING(
		[--with-libec],
		[specify location of libec @<:@default=<nuxdir>/libec@:>@])],
	[],[with_libec=${with_nux}/libec])
libec_dir=${with_libec}

AC_CONFIG_MACRO_DIRS([../m4])

AC_MK_DIR([${nux_dir}/mk])
AC_LIBEC_DIR([${libec_dir}])
AC_CONTRIBLIB_DIR([libfdt],[${nux_dir}/libfdt],[${nux_dir}/contrib/dtc/libfdt])

AC_ARG_ENABLE([targets],
	[AS_HELP_STRING([--enable-targets=LIST],
		[comma-separated list of target architectures: i386,amd64,riscv64 @<:@default: i386@:>@])],
	[enable_targets=$enableval],
	[enable_targets=])

if test x"${enable_targets}" = x ; then
   AC_MSG_ERROR([Please select target architecture(s) with --enable-targets=LIST (e.g., --enable-targets=i386,amd64)])
fi

# Parse target list (POSIX-compatible)
target_count=0
primary_target=""
saved_ifs="$IFS"
IFS=','
for target in $enable_targets; do
	# Validate target
	case "$target" in
		i386|amd64|riscv64) ;;
		*) AC_MSG_ERROR([Unsupported target: $target. Supported targets: i386, amd64, riscv64]) ;;
	esac

	# Set primary target (first in list)
	if test x"$primary_target" = x ; then
		primary_target="$target"
	fi
	target_count=`expr $target_count + 1`
done
IFS="$saved_ifs"

# Toolchain selection
AC_ARG_WITH([toolchain],
	[AS_HELP_STRING([--with-toolchain=TYPE],
		[toolchain type: gnu or llvm @<:@default: gnu@:>@])],
	[with_toolchain=$withval],
	[with_toolchain=gnu])

case "$with_toolchain" in
	gnu|llvm) ;;
	*) AC_MSG_ERROR([Invalid toolchain type: $with_toolchain. Use 'gnu' or 'llvm']) ;;
esac

AC_ARG_WITH([toolchain-prefix],
	[AS_HELP_STRING([--with-toolchain-prefix=PREFIX],
		[toolchain prefix for cross-compilation tools @<:@default: <arch>-unknown-elf-@:>@])],
	[toolchain_prefix=$withval],
	[toolchain_prefix=])

AC_ARG_WITH([toolchain-suffix],
	[AS_HELP_STRING([--with-toolchain-suffix=SUFFIX],
		[toolchain suffix for all tools @<:@default: empty@:>@])],
	[toolchain_suffix=$withval],
	[toolchain_suffix=])

# Image format selection
AC_ARG_WITH([image],
	[AS_HELP_STRING([--with-image=FORMAT],
		[image format: elf, macho, or pecoff @<:@default: elf@:>@])],
	[with_image=$withval],
	[with_image=elf])

case "$with_image" in
	elf|macho|pecoff) ;;
	*) AC_MSG_ERROR([Invalid image format: $with_image. Use 'elf', 'macho', or 'pecoff']) ;;
esac

# Validate universal binary support based on image format
# PE/COFF only supports single target, ELF and Mach-O support multiple targets
if test $target_count -gt 1 ; then
	case "$with_image" in
		pecoff)
			AC_MSG_ERROR([PE/COFF format does not support multiple targets. Only ELF and Mach-O formats support universal binaries. Please specify a single target with --enable-targets=<target>])
			;;
		elf|macho)
			# Universal binary is allowed
			;;
	esac
fi

# Determine tool names based on toolchain type
if test x"$with_toolchain" = x"llvm"; then
	cc_name="clang"
	cxx_name="clang++"
	ld_name="ld.lld"
	ar_name="llvm-ar"
	objcopy_name="llvm-objcopy"
else
	cc_name="gcc"
	cxx_name="g++"
	ld_name="ld"
	ar_name="ar"
	objcopy_name="objcopy"
fi

# Set up tool prefix for primary target
if test x"${toolchain_prefix}" != x ; then
	tool_prefix="${toolchain_prefix}"
else
	case "$primary_target" in
		i386) tool_prefix="i686-unknown-elf-" ;;
		amd64) tool_prefix="x86_64-unknown-elf-" ;;
		riscv64) tool_prefix="riscv64-unknown-elf-" ;;
	esac
fi

# Set up tool prefix for 32-bit (for amd64 multiboot)
if test x"${toolchain_prefix}" != x ; then
	tool_prefix_32="${toolchain_prefix}"
else
	tool_prefix_32="i686-unknown-elf-"
fi

# Check for required tools
AC_CHECK_TOOL(TARGET_CC, ${tool_prefix}${cc_name}${toolchain_suffix}, :)
if test x"${TARGET_CC}" = 'x:' ; then
  AC_MSG_ERROR([${tool_prefix}${cc_name}${toolchain_suffix} not found])
fi
AC_CHECK_TOOL(TARGET_CXX, ${tool_prefix}${cxx_name}${toolchain_suffix}, :)
AC_CHECK_TOOL(TARGET_LD, ${tool_prefix}${ld_name}${toolchain_suffix}, :)
if test x"${TARGET_LD}" = 'x:' ; then
  AC_MSG_ERROR([${tool_prefix}${ld_name}${toolchain_suffix} not found])
fi
AC_CHECK_TOOL(TARGET_AR, ${tool_prefix}${ar_name}${toolchain_suffix}, :)
if test x"${TARGET_AR}" = 'x:' ; then
  AC_MSG_ERROR([${tool_prefix}${ar_name}${toolchain_suffix} not found])
fi
AC_CHECK_TOOL(TARGET_OBJCOPY, ${tool_prefix}${objcopy_name}${toolchain_suffix}, :)
if test x"${TARGET_OBJCOPY}" = 'x:' ; then
  AC_MSG_ERROR([${tool_prefix}${objcopy_name}${toolchain_suffix} not found])
fi

machine=${primary_target}

# Determine build type and subdirectories
# Universal binaries are only supported for ELF and Mach-O with multiple targets
universal_binary=no
if test $target_count -gt 1 ; then
	case "$with_image" in
		elf|macho)
			universal_binary=yes
			;;
		pecoff)
			# Should never reach here due to earlier validation
			universal_binary=no
			;;
	esac
fi

AS_CASE([$primary_target],
	[i386], [apxh_subdirs=multiboot],
	[amd64], [apxh_subdirs="multiboot efi"],
	[riscv64], [apxh_subdirs="sbi efi"])


AC_ARG_ENABLE([opt],
	AS_HELP_STRING([--disable-opt],[compile without compiler optimizations]))
AS_IF([test "x$enable_opt" != "xno"],
	[optflags='-O7 -g'],
	[optflags='-O0 -g'])

AC_ARG_ENABLE([werror],
	AS_HELP_STRING([--disable-werror],[compile ignoring compiler warnings]))
AS_IF([test "x$enable_relax" != "xno"],
	[wflags='-Werror'],
	[wflags=''])

AC_SUBST(MACHINE, ${machine})
AC_SUBST(TOOLCHAIN_32, ${tool_prefix_32})
AC_SUBST(TOOLCHAIN_TYPE, ${with_toolchain})
AC_SUBST(TOOLCHAIN_PREFIX, ${toolchain_prefix})
AC_SUBST(TOOLCHAIN_SUFFIX, ${toolchain_suffix})
AC_SUBST(IMAGE_FORMAT, ${with_image})
AC_SUBST(ENABLE_TARGETS, ${enable_targets})
AC_SUBST(UNIVERSAL_BINARY, ${universal_binary})
AC_SUBST(CONFIGURE_FLAGS, ["${optflags} ${wflags} -I\$(SRCROOT)../include -I\$(BUILDROOT)../include"])
AC_SUBST(APXH_SUBDIRS, [${apxh_subdirs}])
AC_SUBST(INSTALLDIR, ${prefix})

AC_MK_CONFIG_FILES([
	Makefile
	efi/Makefile
	efi/apxhefi/Makefile
	multiboot/Makefile
	sbi/Makefile
])


AC_OUTPUT
