/**
 * RISC-V64 Kernel Startup
 *
 * Assembly entry point for RISC-V64 test kernel.
 * Sets up minimal environment and calls kernel_main.
 */

.section .text.start
.global _start
.align 2

_start:
    # Entry point from bootloader
    # a0 = boot_info_addr (passed by APXH bootloader)
    # a1 = regions_addr (passed by APXH bootloader)

    # Set up stack (use top of RAM, assuming 128MB)
    li   sp, 0x88000000

    # Clear BSS section
    la   t0, __bss_start
    la   t1, __bss_end
1:
    beq  t0, t1, 2f
    sd   zero, 0(t0)
    addi t0, t0, 8
    j    1b
2:

    # Call kernel_main(boot_info_addr, regions_addr)
    # Arguments already in a0, a1
    call kernel_main

    # Kernel should never return, but if it does, hang
halt:
    wfi
    j halt

.section .bss
.align 3
__bss_start:
    .space 4096  # Reserve 4KB for BSS
__bss_end:
