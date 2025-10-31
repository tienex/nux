# ===========================================================================
#    NUX_COMPILER_FLAGS
# ===========================================================================
#
# SYNOPSIS
#
#   NUX_COMPILER_FLAGS
#
# DESCRIPTION
#
#   Set up compiler-specific flags for various compilers

AC_DEFUN([NUX_COMPILER_FLAGS], [
AC_REQUIRE([AX_COMPILER_VENDOR])

# Initialize flag variables
COMPILER_VENDOR="${ax_cv_c_compiler_vendor}"
CFLAGS_OPT=""
CFLAGS_DEBUG=""
CFLAGS_WARN=""
CFLAGS_NOSTDINC=""
CFLAGS_FREESTANDING=""
CFLAGS_PIC=""
CFLAGS_SECTION_ATTR=""
LDFLAGS_NOSTDLIB=""
LDFLAGS_STATIC=""

case "$COMPILER_VENDOR" in
  gnu|clang)
    CFLAGS_OPT="-O2"
    CFLAGS_DEBUG="-g"
    CFLAGS_WARN="-Wall -Wextra"
    CFLAGS_NOSTDINC="-nostdinc"
    CFLAGS_FREESTANDING="-ffreestanding"
    CFLAGS_PIC="-fPIC"
    CFLAGS_SECTION_ATTR="-fdata-sections -ffunction-sections"
    LDFLAGS_NOSTDLIB="-nostdlib"
    LDFLAGS_STATIC="-static"
    ;;

  intel)
    CFLAGS_OPT="-O2"
    CFLAGS_DEBUG="-g"
    CFLAGS_WARN="-Wall"
    CFLAGS_NOSTDINC="-nostdinc"
    CFLAGS_FREESTANDING="-freestanding"
    CFLAGS_PIC="-fPIC"
    CFLAGS_SECTION_ATTR="-ffunction-sections -fdata-sections"
    LDFLAGS_NOSTDLIB="-nostdlib"
    LDFLAGS_STATIC="-static"
    ;;

  microsoft)
    CFLAGS_OPT="/O2"
    CFLAGS_DEBUG="/Zi"
    CFLAGS_WARN="/W3"
    CFLAGS_NOSTDINC="/X"
    CFLAGS_FREESTANDING="/kernel"
    CFLAGS_PIC=""  # Position independent code not needed on Windows
    CFLAGS_SECTION_ATTR="/Gy"  # Function-level linking
    LDFLAGS_NOSTDLIB="/NODEFAULTLIB"
    LDFLAGS_STATIC="/MT"
    ;;

  sun)
    CFLAGS_OPT="-xO2"
    CFLAGS_DEBUG="-g"
    CFLAGS_WARN="-v"
    CFLAGS_NOSTDINC="-Xc"
    CFLAGS_FREESTANDING=""
    CFLAGS_PIC="-KPIC"
    CFLAGS_SECTION_ATTR=""
    LDFLAGS_NOSTDLIB="-nolib"
    LDFLAGS_STATIC="-Bstatic"
    ;;

  hp)
    CFLAGS_OPT="+O2"
    CFLAGS_DEBUG="-g"
    CFLAGS_WARN="+w"
    CFLAGS_NOSTDINC=""
    CFLAGS_FREESTANDING=""
    CFLAGS_PIC="+Z"
    CFLAGS_SECTION_ATTR=""
    LDFLAGS_NOSTDLIB="-nodefaultlibs"
    LDFLAGS_STATIC="-Wl,-a,archive"
    ;;

  ibm)
    CFLAGS_OPT="-O2"
    CFLAGS_DEBUG="-g"
    CFLAGS_WARN="-qinfo=all"
    CFLAGS_NOSTDINC="-qnoinclude"
    CFLAGS_FREESTANDING=""
    CFLAGS_PIC="-qpic"
    CFLAGS_SECTION_ATTR=""
    LDFLAGS_NOSTDLIB="-nostdlib"
    LDFLAGS_STATIC="-bstatic"
    ;;

  watcom)
    CFLAGS_OPT="-ox"
    CFLAGS_DEBUG="-d2"
    CFLAGS_WARN="-wx"
    CFLAGS_NOSTDINC="-zl"
    CFLAGS_FREESTANDING=""
    CFLAGS_PIC=""
    CFLAGS_SECTION_ATTR=""
    LDFLAGS_NOSTDLIB="system causeway"  # Watcom-specific
    LDFLAGS_STATIC=""
    ;;

  tcc)
    CFLAGS_OPT="-O2"
    CFLAGS_DEBUG="-g"
    CFLAGS_WARN="-Wall"
    CFLAGS_NOSTDINC="-nostdinc"
    CFLAGS_FREESTANDING=""
    CFLAGS_PIC="-fPIC"
    CFLAGS_SECTION_ATTR=""
    LDFLAGS_NOSTDLIB="-nostdlib"
    LDFLAGS_STATIC="-static"
    ;;

  pcc)
    CFLAGS_OPT="-O"
    CFLAGS_DEBUG="-g"
    CFLAGS_WARN="-Wall"
    CFLAGS_NOSTDINC="-nostdinc"
    CFLAGS_FREESTANDING=""
    CFLAGS_PIC="-fPIC"
    CFLAGS_SECTION_ATTR=""
    LDFLAGS_NOSTDLIB="-nostdlib"
    LDFLAGS_STATIC="-static"
    ;;

  lcc)
    CFLAGS_OPT="-O"
    CFLAGS_DEBUG="-g"
    CFLAGS_WARN="-A"
    CFLAGS_NOSTDINC=""
    CFLAGS_FREESTANDING=""
    CFLAGS_PIC=""
    CFLAGS_SECTION_ATTR=""
    LDFLAGS_NOSTDLIB=""
    LDFLAGS_STATIC=""
    ;;

  open64)
    CFLAGS_OPT="-O2"
    CFLAGS_DEBUG="-g"
    CFLAGS_WARN="-Wall"
    CFLAGS_NOSTDINC="-nostdinc"
    CFLAGS_FREESTANDING="-ffreestanding"
    CFLAGS_PIC="-fPIC"
    CFLAGS_SECTION_ATTR="-ffunction-sections -fdata-sections"
    LDFLAGS_NOSTDLIB="-nostdlib"
    LDFLAGS_STATIC="-static"
    ;;

  *)
    AC_MSG_WARN([Unknown compiler vendor: $COMPILER_VENDOR, using generic flags])
    CFLAGS_OPT="-O"
    CFLAGS_DEBUG="-g"
    CFLAGS_WARN=""
    CFLAGS_NOSTDINC=""
    CFLAGS_FREESTANDING=""
    CFLAGS_PIC=""
    CFLAGS_SECTION_ATTR=""
    LDFLAGS_NOSTDLIB=""
    LDFLAGS_STATIC=""
    ;;
esac

AC_SUBST(COMPILER_VENDOR)
AC_SUBST(CFLAGS_OPT)
AC_SUBST(CFLAGS_DEBUG)
AC_SUBST(CFLAGS_WARN)
AC_SUBST(CFLAGS_NOSTDINC)
AC_SUBST(CFLAGS_FREESTANDING)
AC_SUBST(CFLAGS_PIC)
AC_SUBST(CFLAGS_SECTION_ATTR)
AC_SUBST(LDFLAGS_NOSTDLIB)
AC_SUBST(LDFLAGS_STATIC)

])# NUX_COMPILER_FLAGS
