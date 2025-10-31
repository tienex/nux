/**
 * x86-64 Kernel Startup
 *
 * Assembly entry point for x86-64 test kernel.
 * Sets up minimal 64-bit environment and calls kernel_main.
 */

.code64
.section .text.start
.global _start

_start:
    # Entry point from bootloader
    # rdi = boot_info_addr (passed by APXH bootloader)
    # rsi = regions_addr (passed by APXH bootloader)

    # Set up stack
    movq $stack_top, %rsp

    # Clear direction flag
    cld

    # Clear BSS section
    movq $__bss_start, %rdi
    movq $__bss_end, %rcx
    subq %rdi, %rcx
    xorq %rax, %rax
    rep stosb

    # Restore boot info arguments
    # (they may have been clobbered during BSS clear)
    # These should already be in rdi and rsi from bootloader

    # Call kernel_main(boot_info_addr, regions_addr)
    call kernel_main

    # Kernel should never return, but if it does, hang
halt:
    hlt
    jmp halt

.section .bss
.align 16
stack_bottom:
    .space 16384  # 16KB stack
stack_top:

.align 8
__bss_start:
    .space 4096  # Reserve 4KB for BSS
__bss_end:
