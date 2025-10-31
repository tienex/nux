/*++
    Module Name:

        platform.h

    Abstract:

        Architecture, endianness, and object format detection.

    Environment:

        Cross-platform.
--*/

#pragma once

/* Architecture family detection */
#ifndef ANX_ARCH_FAMILY
#   if defined(__x86_64__) || defined(_M_X64)
#       define ANX_ARCH_FAMILY x86_64
#       define ANX_ARCH_X86_64 1
#   elif defined(__i386__) || defined(_M_IX86)
#       define ANX_ARCH_FAMILY x86
#       define ANX_ARCH_X86 1
#   elif defined(__aarch64__) || defined(_M_ARM64)
#       define ANX_ARCH_FAMILY arm64
#       define ANX_ARCH_ARM64 1
#   elif defined(__arm__) || defined(_M_ARM)
#       define ANX_ARCH_FAMILY arm
#       define ANX_ARCH_ARM 1
#   elif defined(__riscv)
#       define ANX_ARCH_FAMILY riscv
#       define ANX_ARCH_RISCV 1
#   else
#       define ANX_ARCH_FAMILY unknown
#   endif
#endif

/* Endianness detection */
#ifndef ANX_ENDIAN_LE
#   if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#       define ANX_ENDIAN_BE 1
#   else
#       define ANX_ENDIAN_LE 1
#   endif
#endif

/* Object file format detection */
#ifndef ANX_OBJFMT
#   if defined(__APPLE__) && defined(__MACH__)
#       define ANX_OBJFMT_MACHO 1
#       define ANX_OBJFMT "Mach-O"
#   elif defined(_WIN32) || defined(_WIN64)
#       define ANX_OBJFMT_PECOFF 1
#       define ANX_OBJFMT "PE/COFF"
#   elif defined(__ELF__)
#       define ANX_OBJFMT_ELF 1
#       define ANX_OBJFMT "ELF"
#   else
#       define ANX_OBJFMT_UNKNOWN 1
#       define ANX_OBJFMT "UNKNOWN"
#   endif
#endif
