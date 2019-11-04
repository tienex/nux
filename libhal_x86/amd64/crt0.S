#include <machine/asm.h>
#include "amd64.h"
#include "../internal.h"

	.altmacro
.macro repeat macro from to
	\macro \from
	.if \to-\from
	repeat \macro,%(\from+1),\to
	.endif
.endm

.macro _setidt vct usr
	movabs $hdlr_&\vct, %rcx
	movabs $_idt, %rbx
	movl $(KCS << 16), %eax
	movw  %cx, %ax
	movl %eax, (\vct * 16)(%rbx)
	movq %rcx, %rax
	.if \usr > 0
	movw $0xee00, %ax
	.else
	movw $0x8e00, %ax
	.endif
	movq %rax, ((\vct * 16) + 4)(%rbx)
.endm

.macro setidt vct
	_setidt \vct 0
.endm

	.text
	.globl _start
_start:
	movabs $_stacktop, %rsp

	movabs $_gdtr, %rax
	lgdt (%rax)

	mov $KDS, %ax
	mov %ax, %ss

	repeat setidt 0, 31
	repeat setidt 32, 63
	repeat setidt 64, 95
	repeat setidt 96, 127
	repeat setidt 128, 191
	repeat setidt 192, 255
	movabs $_idtr, %rax
	lidt (%rax)

	/* init hal */
	call _C_LABEL(x86_init)

	// Save CR3 for future AP boots.
	mov %cr3, %rax
	movabs $_ap_cr3, %rbx
	mov %rax, (%rbx)

	movabs $zippostring, %rdi
	call ___start
	jmp zippo


/*
 * AP BOOTSTRAP.
 *
 * This code is copied into per-cpu pages.
 * These same page will become the AP kernel
 * stack.
 */
	.code16
	.globl _C_LABEL(_ap_start)
ENTRY(_ap_start)
	cli
	movw %cs, %ax
	movw %ax, %ds
	movw %ax, %es

	lgdtl _ap_gdtreg - _C_LABEL(_ap_start)
	/* No IDT. If anything goes wrong here, machine triple-faults. */

	movw $0xb800, %ax
	movw %ax, %es
	movw $('A'<<8 + 'A'), %es:0

	/* Save AP Boostrap Page Segment. */
	mov %cs, %bx

	/* Enter protected mode. */
	mov $1, %ax
	lmsw %ax

	/* Jump into protected mode */
	ljmpl _ap_ljmp1 - _ap_start

	.align 16
.Lap_gdttmp:
	.long 0
	.long 0
	/* 64-bit CS */
	.long 0x0000ffff
	.long 0x00af9a00
	/* DS */
	.long 0x0000ffff
	.long 0x00cf9200
	/* 32-bit CS */
#define APTMP_CS32 0x18	
	.long 0x0000ffff
	.long 0x00cf9a00

	/*
	  AP Bootstrap configuration.

	  The following fields are set up by prepare.
	*/
	.globl _ap_gdtreg, _ap_ljmp1, _ap_ljmp2, _ap_stackpage, _ap_cr3
_ap_gdtreg:
	.hword 127
	.long .Lap_gdttmp - _C_LABEL(_ap_start)
_ap_ljmp1:
	.long .Lap_setup - _ap_start
	.word APTMP_CS32
_ap_ljmp2:
	.long .Lap_tramp64 - _ap_start
	.word KCS
_ap_stackpage:
	.quad 0
	.globl _ap_cr3
_ap_cr3:
	.quad 0x12345
	.code32
.Lap_setup:
	/* Setup segments. */
	mov $KDS, %ax
	mov %ax, %ds

	/* Get AP configuration. */
	xor %ecx, %ebp
	mov %bx, %bp
	shl $4, %ebp
	mov (_ap_cr3 - _ap_start)(%ebp), %eax
	mov %eax, %cr3
	mov (_ap_stackpage - _ap_start)(%ebp), %edi
	mov (_ap_stackpage - _ap_start + 4)(%ebp), %esi

	/*
	   Set NX mode.

                NB: This is hairy. If there's any misunderstanding
	        between BSP and APs on the NX setting (I am
	        looking at you IA32_MISC_ENABLE's DISABLE_NX)
	        then the AP will likely triple fault on loading
	        the pagetable. This will reset the whole machine.

	        It shouldn't happen,  but if you ever see a machine
	        reset while enabling secondary processor, start
	        looking at this.
	*/
	mov $0x80000001, %eax
	mov $0, %ecx
	cpuid
	and $(1 << 20), %edx
	jz .Lno_nx

	mov $MSR_IA32_EFER, %ecx
	rdmsr
	or $(1 << 11), %eax
	wrmsr
	/* NX should be enabled now. */

.Lno_nx:
	/* Set EFER.LME */
	mov $MSR_IA32_EFER, %ecx
	rdmsr
	or $(1 << 8), %eax
	wrmsr
	/* Setup CR registers. */
	mov %cr4, %eax
	or  $0x30, %eax /* PAE on */
	mov  %eax, %cr4
	mov  %cr0, %eax
	or   $((1<<31)|(1<<16)), %eax
	mov  %eax, %cr0


	/* trampoline2 */
	ljmpl (_ap_ljmp2 - _ap_start)(%ebp)
	.code64
.Lap_tramp64:
	movabs $apsetup, %rax
	jmp *%rax
	.globl _ap_end
_ap_end:	

apsetup: /* edi: ESP low 32, esi: ESP hi 32 */
1:
	/* Reload GDT and IDT */
	movabs $_gdtr, %rax
	lgdt (%rax)
	movabs $_idtr, %rax
	lidt (%rax)

	mov $KDS, %ax
	mov %ax, %ss

	/* Setup stack */
	mov %edi, %esp
	shl $32, %rsi
	or %rsi, %rsp
	call _C_LABEL(hal_main_ap)
1:	hlt
	jmp 1b	

zippo:
	movabs $zippostring, %rsi
	movl $0x1000, %ecx
	movb $0x87, %ah
	movl $0xb8000, %edi
1:	lodsb
	stosw
	cmp $0, %al
	loopne 1b
1:	jmp 1b


	.data
zippostring:
	.asciz "Zippo."
	
	.align 64
_gdtr:
	.hword (_end_gdt - _gdt) - 1
	.quad _gdt

	.align 64
	.globl _gdt
_gdt:
	.quad 0
	.quad 0x00af9a000000ffff /* Kernel Code */
	.quad 0x00cf93000000ffff /* Kernel Data */
	.quad 0x00a0fa0000000000 /* User Code */
	.quad 0                  /* User Data */
	.skip (8 * (3*MAXCPUS))  /* TSS + GS per CPU. */
	.globl _end_gdt
_end_gdt:

	.align 64
	.globl _idtr
_idtr:
	.hword (_end_idt - _idt) - 1
	.quad _idt

	.align 64
	.globl _idtr
_idt:	
	.skip 256 * 16
_end_idt:

	.align 4096
_stackbtm:
	.zero 4096
_stacktop:	
