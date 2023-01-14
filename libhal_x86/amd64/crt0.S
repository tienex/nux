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

.macro _setidt vct usr ist
	movabs $hdlr_&\vct, %rcx
	movabs $_idt, %rbx
	movl $(KCS << 16), %eax
	movw  %cx, %ax
	movl %eax, (\vct * 16)(%rbx)
	movq %rcx, %rax
	.if \usr > 0
	movw $0xee00 + \ist, %ax
	.else
	movw $0x8e00 + \ist, %ax
	.endif
	movq %rax, ((\vct * 16) + 4)(%rbx)
.endm

.macro setidt vct
	_setidt \vct 0 0
.endm

	.text
	.globl _start
_start:
	mov %rsi, %cr3
	movabs $_bsp_stacktop, %rsp

	movabs $_gdtr, %rax
	lgdt (%rax)

	movabs $_setcs_ljmp, %rax
	push $KCS
	movabs $1f, %rax
	push %rax
	lretq
_setcs_ljmp:
	.quad 1f
	.word KCS
1:
	mov $KDS, %ax
	mov %ax, %ss

	setidt 0
	setidt 1
	_setidt 2 0 1 /* IST #1: NMI */
	repeat setidt 3, 7
	_setidt 8 0 2 /* IST #2: #DF */
	repeat setidt 9, 17
	_setidt 18 0 3 /* IST #3: #MC */
	repeat setidt 19, 31
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
	movabs $_bsp_cr3, %rbx
	mov %eax, (%rbx)

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
	.globl _ap_ljmp1, _ap_ljmp2, _ap_gdtreg, _ap_cr3, _ap_end

	.globl _C_LABEL(_ap_start)
ENTRY(_ap_start)
	cli
	movw %cs, %ax
	movw %ax, %ds
	movw %ax, %es

	lgdtl _ap_gdtreg - _C_LABEL(_ap_start)
	/* No IDT. If anything goes wrong here, machine
	/* triple-faults. */

	/* Save AP Boostrap Page Segment. */
	mov %cs, %bx

	/* Enter protected mode. */
	mov $1, %ax
	lmsw %ax

	/* Jump into protected mode */
	ljmpl _ap_ljmp1 - _ap_start

_ap_ljmp1:
	.long .Lap_setup - _ap_start
#define APTMP_CS32 0x18	
	.word APTMP_CS32

	.code32
.Lap_setup:
	/* Setup segments. */
	mov $KDS, %ax
	mov %ax, %ds
	mov %ax, %ss

	/* Get AP configuration. */
	xor %ecx, %ebp
	mov %bx, %bp
	shl $4, %ebp
	mov (_ap_cr3 - _ap_start)(%ebp), %eax
	mov %eax, %cr3

	/* Check NX Support. */
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

_ap_ljmp2:
	.long .Lap_tramp64 - _ap_start
	.word KCS

	.code64
.Lap_tramp64:
	movabs $apsetup, %rax
	jmp *%rax

	/*
	 * Trampoline GDTREG + GDT
	 */
_ap_gdtreg:
	.hword 127
	.long .Lap_gdttmp - _C_LABEL(_ap_start)
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
	/* 32-bit CS (APTMP_CS32) */
	.long 0x0000ffff
	.long 0x00cf9a00
	
	/*
	  Temporary 1:1 CR3
	*/
_ap_cr3:
	.quad 0x12345

	/*
	 * End of trampoline code
	 */
_ap_end:	

apsetup:
1:
	/* Reload GDT and IDT */
	movabs $_gdtr, %rax
	lgdt (%rax)
	movabs $_idtr, %rax
	lidt (%rax)

	mov $KDS, %ax
	mov %ax, %ss

	/* Allocate and setup stack. */
	movabs $pcpu_kstack, %rsi
	movabs $pcpu_kstackcnt, %rdi
	mov (%rdi), %rax
1:
	mov %rax, %rbx
	inc %rbx
	lock cmpxchg %rbx, (%rdi)
	jz 1f
	pause
	jmp 1b
1:
	lea (%rsi, %rax, 8), %rax
	mov (%rax), %rsp

	mov %rsp, %rdi
	call _C_LABEL(amd64_init_ap)
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

	.globl _bsp_cr3
_bsp_cr3:
	.quad 0
	
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
	.quad 0			 /* Left empty. Base of MSR_IA32_STAR[63:48] */
	.quad 0x00c0f3000000ffff /* User Data */
	.quad 0x00a0fa000000ffff /* User Code */
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
	.globl _bsp_stacktop
_bsp_stacktop:

	.align 4096
_ist1btm:
	.zero 4096
	.globl _ist1_stacktop
_ist1_stacktop:

	.align 4096
_ist2btm:
	.zero 4096
	.globl _ist2_stacktop
_ist2_stacktop:

	.align 4096
_ist3btm:
	.zero 4096
	.globl _ist3_stacktop
_ist3_stacktop:
