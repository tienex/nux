# ===========================================================================
#    AX_COMPILER_VENDOR
# ===========================================================================
#
# SYNOPSIS
#
#   AX_COMPILER_VENDOR
#
# DESCRIPTION
#
#   Determine the vendor of the C/C++ compiler, e.g., gnu, intel, ibm, hp,
#   borland, microsoft, sun, tcc, pcc, open64, etc.
#
#   Sets the variable ax_cv_c_compiler_vendor for C compiler
#   Sets the variable ax_cv_cxx_compiler_vendor for C++ compiler
#

AC_DEFUN([AX_COMPILER_VENDOR], [
AC_CACHE_CHECK([for C compiler vendor], ax_cv_c_compiler_vendor,
  [# note: don't check for gcc first since some compilers define __GNUC__
  vendors="intel:     __ICC,__ECC,__INTEL_COMPILER
           ibm:       __xlc__,__xlC__,__IBMC__,__IBMCPP__,__ibmxl__
           pathscale: __PATHCC__,__PATHSCALE__
           clang:     __clang__
           cray:      _CRAYC
           fujitsu:   __FUJITSU
           sdcc:      SDCC,__SDCC
           sx:        _SX
           portland:  __PGI
           gnu:       __GNUC__
           sun:       __SUNPRO_C,__SUNPRO_CC,__SUNPRO_F90,__SUNPRO_F95
           hp:        __HP_cc,__HP_aCC
           dec:       __DECC,__DECCXX,__DECC_VER,__DECCXX_VER
           borland:   __BORLANDC__,__CODEGEARC__,__TURBOC__
           comeau:    __COMO__
           kai:       __KCC
           lcc:       __LCC__
           sgi:       __sgi,sgi
           microsoft: _MSC_VER
           metrowerks: __MWERKS__
           watcom:    __WATCOMC__
           tcc:       __TINYC__
           pcc:       __PCC__
           open64:    __OPEN64__
           unknown:   UNKNOWN"
  for ventest in $vendors; do
    case $ventest in
      *:) vendor=$ventest; continue ;;
      *)  vencpp="defined("`echo $ventest | sed 's/,/) || defined(/g'`")" ;;
    esac
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[
      #if !($vencpp)
        thisisanerror;
      #endif
    ])], [break])
  done
  ax_cv_c_compiler_vendor=`echo $vendor | cut -d: -f1`
  ])

AC_CACHE_CHECK([for C++ compiler vendor], ax_cv_cxx_compiler_vendor,
  [AC_LANG_PUSH([C++])
  # note: don't check for gcc first since some compilers define __GNUC__
  vendors="intel:     __ICC,__ECC,__INTEL_COMPILER
           ibm:       __xlc__,__xlC__,__IBMC__,__IBMCPP__,__ibmxl__
           pathscale: __PATHCC__,__PATHSCALE__
           clang:     __clang__
           cray:      _CRAYC
           fujitsu:   __FUJITSU
           sdcc:      SDCC,__SDCC
           sx:        _SX
           portland:  __PGI
           gnu:       __GNUC__
           sun:       __SUNPRO_C,__SUNPRO_CC,__SUNPRO_F90,__SUNPRO_F95
           hp:        __HP_cc,__HP_aCC
           dec:       __DECC,__DECCXX,__DECC_VER,__DECCXX_VER
           borland:   __BORLANDC__,__CODEGEARC__,__TURBOC__
           comeau:    __COMO__
           kai:       __KCC
           lcc:       __LCC__
           sgi:       __sgi,sgi
           microsoft: _MSC_VER
           metrowerks: __MWERKS__
           watcom:    __WATCOMC__
           tcc:       __TINYC__
           pcc:       __PCC__
           open64:    __OPEN64__
           unknown:   UNKNOWN"
  for ventest in $vendors; do
    case $ventest in
      *:) vendor=$ventest; continue ;;
      *)  vencpp="defined("`echo $ventest | sed 's/,/) || defined(/g'`")" ;;
    esac
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[
      #if !($vencpp)
        thisisanerror;
      #endif
    ])], [break])
  done
  ax_cv_cxx_compiler_vendor=`echo $vendor | cut -d: -f1`
  AC_LANG_POP([C++])
  ])
])# AX_COMPILER_VENDOR
