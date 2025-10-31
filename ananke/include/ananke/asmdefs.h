/*++
    Module Name:

        asmdefs.h

    Abstract:

        Portable assembly definitions for different assemblers (GAS, YASM, NASM)
        and object formats (ELF, Mach-O, PE/COFF). Provides consistent macros
        for function/data labels, symbol visibility, section directives, and
        syntax selection.

        Supported assemblers:
            - GAS (GNU Assembler) - AT&T and Intel syntax
            - YASM - Intel syntax
            - NASM - Intel syntax

        Supported object formats:
            - ELF (Linux, BSD, Solaris)
            - Mach-O (macOS, iOS)
            - PE/COFF (Windows)

    Environment:

        Assembly language. Include this in .S (GAS) or .asm (YASM/NASM) files.
--*/

#pragma once

/* --------------------------------------------------------------- */
/*  Assembler detection.                                           */
/* --------------------------------------------------------------- */

#if defined(__YASM_MAJOR__) || defined(YASM)
#   define ANX_ASM_YASM 1
#   define ANX_ASM_INTEL_SYNTAX 1
#elif defined(__NASM_MAJOR__) || defined(NASM)
#   define ANX_ASM_NASM 1
#   define ANX_ASM_INTEL_SYNTAX 1
#else
    /* Assume GAS (GNU Assembler) */
#   define ANX_ASM_GAS 1
    /* GAS can use either AT&T or Intel syntax - default to AT&T unless specified */
#   if defined(ANX_USE_INTEL_SYNTAX) || defined(__INTEL_SYNTAX__)
#       define ANX_ASM_INTEL_SYNTAX 1
#   else
#       define ANX_ASM_ATT_SYNTAX 1
#   endif
#endif

/* --------------------------------------------------------------- */
/*  Object format detection (platform-specific).                   */
/* --------------------------------------------------------------- */

/*
 * Detect object format based on compiler predefined macros:
 * - ELF:    Linux, *BSD, Solaris, most Unix systems
 * - Mach-O: macOS, iOS, Darwin
 * - COFF:   Windows
 */

#if defined(__APPLE__) && defined(__MACH__)
#   define ANX_ASM_MACHO 1
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#   define ANX_ASM_COFF 1
#else
    /* Default to ELF for Linux, BSD, and other Unix-like systems */
#   define ANX_ASM_ELF 1
#endif

/* Architecture detection for stack alignment and other arch-specific needs */
#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
#   define ANX_ARCH_X86_64 1
#   define ANX_ARCH_64 1
#elif defined(__i386__) || defined(_M_IX86) || defined(__i386)
#   define ANX_ARCH_X86_32 1
#   define ANX_ARCH_32 1
#elif defined(__riscv) && (__riscv_xlen == 64)
#   define ANX_ARCH_RISCV64 1
#   define ANX_ARCH_64 1
#elif defined(__riscv) && (__riscv_xlen == 32)
#   define ANX_ARCH_RISCV32 1
#   define ANX_ARCH_32 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#   define ANX_ARCH_ARM64 1
#   define ANX_ARCH_64 1
#elif defined(__arm__) || defined(_M_ARM)
#   define ANX_ARCH_ARM32 1
#   define ANX_ARCH_32 1
#endif

/* --------------------------------------------------------------- */
/*  Symbol name decoration (underscores, etc).                     */
/* --------------------------------------------------------------- */

/*
 * Mach-O and 32-bit Windows typically prefix symbols with underscore.
 * ELF and 64-bit Windows do not.
 */
#if defined(ANX_ASM_MACHO)
#   define ANX_ASM_SYMBOL_PREFIX _
#   define ANX_ASM_NEEDS_UNDERSCORE 1
#elif defined(ANX_ASM_COFF) && defined(ANX_ARCH_32)
#   define ANX_ASM_SYMBOL_PREFIX _
#   define ANX_ASM_NEEDS_UNDERSCORE 1
#else
    /* ELF and 64-bit Windows - no prefix */
#   define ANX_ASM_SYMBOL_PREFIX
#   define ANX_ASM_NEEDS_UNDERSCORE 0
#endif

/* Helper to concatenate symbol prefix */
#define ANX_ASM_MANGLE_IMPL(prefix, name)  prefix##name
#define ANX_ASM_MANGLE(name)               ANX_ASM_MANGLE_IMPL(ANX_ASM_SYMBOL_PREFIX, name)

/* --------------------------------------------------------------- */
/*  GAS-specific syntax control.                                   */
/* --------------------------------------------------------------- */

#if defined(ANX_ASM_GAS)
#   if defined(ANX_ASM_INTEL_SYNTAX)
        /* Switch GAS to Intel syntax */
#       define ANX_ASM_SYNTAX_DIRECTIVE  .intel_syntax noprefix
#   else
        /* Use default AT&T syntax */
#       define ANX_ASM_SYNTAX_DIRECTIVE  /* empty */
#   endif
#else
    /* YASM/NASM always use Intel syntax */
#   define ANX_ASM_SYNTAX_DIRECTIVE  /* empty */
#endif

/* --------------------------------------------------------------- */
/*  Function/label declaration macros.                             */
/* --------------------------------------------------------------- */

#if defined(ANX_ASM_ELF)
    /*
     * ELF format (Linux, BSD, Solaris).
     * Uses .type, .size, .globl directives.
     */
#   if defined(ANX_ASM_GAS)
#       define ANX_ASM_FUNC_BEGIN(name) \
            .globl ANX_ASM_MANGLE(name) ; \
            .type ANX_ASM_MANGLE(name), @function ; \
            ANX_ASM_MANGLE(name):
#       define ANX_ASM_FUNC_END(name) \
            .size ANX_ASM_MANGLE(name), . - ANX_ASM_MANGLE(name)
#   else /* YASM/NASM */
#       define ANX_ASM_FUNC_BEGIN(name) \
            global ANX_ASM_MANGLE(name) ; \
            ANX_ASM_MANGLE(name):
#       define ANX_ASM_FUNC_END(name)  /* YASM/NASM don't need .size */
#   endif

#   define ANX_ASM_DATA_BEGIN(name) \
        .globl ANX_ASM_MANGLE(name) ; \
        .type ANX_ASM_MANGLE(name), @object ; \
        ANX_ASM_MANGLE(name):
#   define ANX_ASM_DATA_END(name) \
        .size ANX_ASM_MANGLE(name), . - ANX_ASM_MANGLE(name)

#elif defined(ANX_ASM_MACHO)
    /*
     * Mach-O format (macOS, iOS).
     * Uses .globl directive, symbols prefixed with underscore.
     */
#   if defined(ANX_ASM_GAS)
#       define ANX_ASM_FUNC_BEGIN(name) \
            .globl ANX_ASM_MANGLE(name) ; \
            ANX_ASM_MANGLE(name):
#       define ANX_ASM_FUNC_END(name)  /* Mach-O doesn't need .size */
#   else /* YASM/NASM */
#       define ANX_ASM_FUNC_BEGIN(name) \
            global ANX_ASM_MANGLE(name) ; \
            ANX_ASM_MANGLE(name):
#       define ANX_ASM_FUNC_END(name)  /* no size directive */
#   endif

#   define ANX_ASM_DATA_BEGIN(name) \
        .globl ANX_ASM_MANGLE(name) ; \
        ANX_ASM_MANGLE(name):
#   define ANX_ASM_DATA_END(name)  /* no size directive */

#elif defined(ANX_ASM_COFF)
    /*
     * PE/COFF format (Windows).
     * Uses .globl directive (or GLOBAL in YASM/NASM).
     */
#   if defined(ANX_ASM_GAS)
#       define ANX_ASM_FUNC_BEGIN(name) \
            .globl ANX_ASM_MANGLE(name) ; \
            .def ANX_ASM_MANGLE(name); .scl 2; .type 32; .endef ; \
            ANX_ASM_MANGLE(name):
#       define ANX_ASM_FUNC_END(name)  /* COFF doesn't need .size */
#   else /* YASM/NASM */
#       define ANX_ASM_FUNC_BEGIN(name) \
            global ANX_ASM_MANGLE(name) ; \
            ANX_ASM_MANGLE(name):
#       define ANX_ASM_FUNC_END(name)  /* no size directive */
#   endif

#   define ANX_ASM_DATA_BEGIN(name) \
        .globl ANX_ASM_MANGLE(name) ; \
        ANX_ASM_MANGLE(name):
#   define ANX_ASM_DATA_END(name)  /* no size directive */

#else
    /* Fallback for unknown object format */
#   define ANX_ASM_FUNC_BEGIN(name)  ANX_ASM_MANGLE(name):
#   define ANX_ASM_FUNC_END(name)    /* empty */
#   define ANX_ASM_DATA_BEGIN(name)  ANX_ASM_MANGLE(name):
#   define ANX_ASM_DATA_END(name)    /* empty */
#endif

/* Local (non-exported) label */
#if defined(ANX_ASM_GAS)
#   define ANX_ASM_LOCAL_LABEL(name)  .L##name:
#elif defined(ANX_ASM_YASM) || defined(ANX_ASM_NASM)
#   define ANX_ASM_LOCAL_LABEL(name)  .##name:
#else
#   define ANX_ASM_LOCAL_LABEL(name)  name:
#endif

/* --------------------------------------------------------------- */
/*  Section directives.                                            */
/* --------------------------------------------------------------- */

#if defined(ANX_ASM_ELF)
#   define ANX_ASM_SECTION_TEXT      .section .text
#   define ANX_ASM_SECTION_DATA      .section .data
#   define ANX_ASM_SECTION_RODATA    .section .rodata
#   define ANX_ASM_SECTION_BSS       .section .bss
#elif defined(ANX_ASM_MACHO)
#   define ANX_ASM_SECTION_TEXT      .section __TEXT,__text
#   define ANX_ASM_SECTION_DATA      .section __DATA,__data
#   define ANX_ASM_SECTION_RODATA    .section __TEXT,__const
#   define ANX_ASM_SECTION_BSS       .section __DATA,__bss
#elif defined(ANX_ASM_COFF)
#   define ANX_ASM_SECTION_TEXT      .section .text
#   define ANX_ASM_SECTION_DATA      .section .data
#   define ANX_ASM_SECTION_RODATA    .section .rdata
#   define ANX_ASM_SECTION_BSS       .section .bss
#else
#   define ANX_ASM_SECTION_TEXT      .text
#   define ANX_ASM_SECTION_DATA      .data
#   define ANX_ASM_SECTION_RODATA    .rodata
#   define ANX_ASM_SECTION_BSS       .bss
#endif

/* --------------------------------------------------------------- */
/*  Alignment directives.                                          */
/* --------------------------------------------------------------- */

#if defined(ANX_ASM_GAS)
    /* GAS uses .align with different semantics per architecture:
     * - On x86: power-of-2 bytes (e.g., .align 4 = 16 bytes)
     * - On RISC-V/ARM: direct bytes (e.g., .align 4 = 4 bytes)
     * Use .balign for consistent byte alignment. */
#   define ANX_ASM_ALIGN(n)  .balign n
#elif defined(ANX_ASM_YASM) || defined(ANX_ASM_NASM)
    /* YASM/NASM align takes byte count directly */
#   define ANX_ASM_ALIGN(n)  align n
#else
#   define ANX_ASM_ALIGN(n)  .align n
#endif

/* --------------------------------------------------------------- */
/*  Common architecture-specific register names (for documentation).*/
/* --------------------------------------------------------------- */

/*
 * AT&T syntax (GAS):  %rax, %rbx, etc.
 * Intel syntax:       rax, rbx, etc.
 *
 * This header doesn't redefine register names - it's the assembler's job.
 * But for reference:
 *
 * x86-64 (64-bit): rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp, r8-r15
 * x86 (32-bit):    eax, ebx, ecx, edx, esi, edi, ebp, esp
 * RISC-V:          x0-x31 (a0-a7, t0-t6, s0-s11, ra, sp, gp, tp)
 * ARM64:           x0-x30, sp, pc
 */

/* --------------------------------------------------------------- */
/*  Stack frame helpers (architecture-specific).                   */
/* --------------------------------------------------------------- */

#if defined(ANX_ARCH_X86_64)
    /* x86-64 stack is 16-byte aligned at function entry (after call) */
#   define ANX_ASM_STACK_ALIGNMENT  16
#elif defined(ANX_ARCH_RISCV64) || defined(ANX_ARCH_ARM64)
    /* RISC-V and ARM64 require 16-byte stack alignment */
#   define ANX_ASM_STACK_ALIGNMENT  16
#elif defined(ANX_ARCH_X86_32)
    /* x86 32-bit typically 4-byte aligned, but modern code uses 16 */
#   define ANX_ASM_STACK_ALIGNMENT  16
#else
#   define ANX_ASM_STACK_ALIGNMENT  8
#endif

/* --------------------------------------------------------------- */
/*  CFI (Call Frame Information) directives for ELF debugging.     */
/* --------------------------------------------------------------- */

#if defined(ANX_ASM_ELF) && defined(ANX_ASM_GAS)
#   define ANX_ASM_CFI_STARTPROC     .cfi_startproc
#   define ANX_ASM_CFI_ENDPROC       .cfi_endproc
#   define ANX_ASM_CFI_DEF_CFA(reg, offset)       .cfi_def_cfa reg, offset
#   define ANX_ASM_CFI_DEF_CFA_OFFSET(offset)     .cfi_def_cfa_offset offset
#   define ANX_ASM_CFI_OFFSET(reg, offset)        .cfi_offset reg, offset
#   define ANX_ASM_CFI_ADJUST_CFA_OFFSET(offset)  .cfi_adjust_cfa_offset offset
#else
    /* No CFI support for non-ELF or non-GAS */
#   define ANX_ASM_CFI_STARTPROC     /* empty */
#   define ANX_ASM_CFI_ENDPROC       /* empty */
#   define ANX_ASM_CFI_DEF_CFA(reg, offset)       /* empty */
#   define ANX_ASM_CFI_DEF_CFA_OFFSET(offset)     /* empty */
#   define ANX_ASM_CFI_OFFSET(reg, offset)        /* empty */
#   define ANX_ASM_CFI_ADJUST_CFA_OFFSET(offset)  /* empty */
#endif

/* --------------------------------------------------------------- */
/*  Common immediate/addressing syntax helpers.                    */
/* --------------------------------------------------------------- */

#if defined(ANX_ASM_ATT_SYNTAX)
    /* AT&T syntax: registers prefixed with %, immediates with $ */
#   define ANX_ASM_IMM(val)   $##val
#   define ANX_ASM_REG(name)  %##name
#elif defined(ANX_ASM_INTEL_SYNTAX)
    /* Intel syntax: no prefixes */
#   define ANX_ASM_IMM(val)   val
#   define ANX_ASM_REG(name)  name
#else
#   define ANX_ASM_IMM(val)   val
#   define ANX_ASM_REG(name)  name
#endif

/* --------------------------------------------------------------- */
/*  Convenience macros for common patterns.                        */
/* --------------------------------------------------------------- */

/*
 * Complete function wrapper - use this for simple assembly functions:
 *
 * ANX_ASM_FUNCTION_BEGIN(MyFunction)
 *     ... assembly code ...
 * ANX_ASM_FUNCTION_END(MyFunction)
 */
#define ANX_ASM_FUNCTION_BEGIN(name) \
    ANX_ASM_SECTION_TEXT ; \
    ANX_ASM_ALIGN(ANX_ASM_STACK_ALIGNMENT) ; \
    ANX_ASM_FUNC_BEGIN(name) ; \
    ANX_ASM_CFI_STARTPROC

#define ANX_ASM_FUNCTION_END(name) \
    ANX_ASM_CFI_ENDPROC ; \
    ANX_ASM_FUNC_END(name)

/*
 * Data object wrapper:
 *
 * ANX_ASM_DATA_OBJECT_BEGIN(MyData)
 *     .quad 0x1234
 * ANX_ASM_DATA_OBJECT_END(MyData)
 */
#define ANX_ASM_DATA_OBJECT_BEGIN(name, alignment) \
    ANX_ASM_SECTION_DATA ; \
    ANX_ASM_ALIGN(alignment) ; \
    ANX_ASM_DATA_BEGIN(name)

#define ANX_ASM_DATA_OBJECT_END(name) \
    ANX_ASM_DATA_END(name)

/*
 * Read-only data wrapper:
 */
#define ANX_ASM_RODATA_OBJECT_BEGIN(name, alignment) \
    ANX_ASM_SECTION_RODATA ; \
    ANX_ASM_ALIGN(alignment) ; \
    ANX_ASM_DATA_BEGIN(name)

#define ANX_ASM_RODATA_OBJECT_END(name) \
    ANX_ASM_DATA_END(name)
