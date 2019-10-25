#include <machine/asm.h>
#include "../internal.h"
#include "i386.h"

	.text
	.globl _start
_start:
	movl $stacktop, %esp

	/* Clear flags (cld and cli) */
	pushl $0
	popf

	/* Set segments in a known state */
	call _setup_gdt
	lgdt _gdtr

	ljmp $KCS, $1f
1:	mov $KDS, %eax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov $0, %eax
	mov %ax, %fs
	mov %ax, %gs

	mov $(5 << 3), %ax
	ltr %ax

	/* Setup IDT */
	call _setup_idt
	lidt _idtr

	call _clear_bss

	mov %cr3, %eax
	mov %eax, _bootcr3

	/* init hal */
	call _C_LABEL(x86_init)

	push $zippostring
	call ___start
	jmp zippo


_clear_bss:
	push %edi
	mov $0x0, %eax
	mov $_ebss, %ecx
	sub $_sbss, %ecx
	mov $_sbss, %edi
	rep stosb
	pop %edi
	ret

_setup_gdt:
	movl $_gdt + 8, %eax
	/* 32-bit code */
	movl $0x0000ffff, (%eax)
	movl $0x00cf9a00, 4(%eax)
	add $8, %eax
	/* 32-bit data */
	movl $0x0000ffff, (%eax)
	movl $0x00cf9200, 4(%eax)
	add $8, %eax
	/* User 32-bit code */
	movl $0x0000ffff, (%eax)
	movl $0x00cffa00, 4(%eax)
	add $8, %eax
	/* User 32-bit data */
	movl $0x0000ffff, (%eax)
	movl $0x00cff200, 4(%eax)
	add $8, %eax
	/* TSS */
	movl $0, (%eax)
	movl $0, 4(%eax)
	movl _tss, %ecx
	movw %cx, 2(%eax)
	movw $104, (%eax)
	shr  $16, %ecx
	movb %cl, 4(%eax)
	movb %ch, 7(%eax)
	orl  $0x00408900, 4(%eax)
	ret

	.altmacro
.macro repeat macro from to
	\macro \from
	.if \to-\from
	repeat \macro,%(\from+1),\to
	.endif
.endm

.macro _setidt vct usr
	mov $hdlr_&\vct, %ecx
	mov $KCS, %ax
	shl $16, %eax
	mov %cx, %ax
	mov %eax, _idt + (\vct * 8)
	mov %ecx, %eax
	.if \usr > 0
	mov $0xee00, %ax
	.else
	mov $0x8e00, %ax
	.endif
	mov %eax, _idt + (\vct * 8) + 4
.endm

.macro setidt vct
	_setidt \vct 0
.endm

_setup_idt:
	repeat setidt 0, 31
	repeat setidt 32, 63
	repeat setidt 64, 95
	repeat setidt 96, 127
	repeat setidt 128, 191
	repeat setidt 192, 255
	_setidt 33 1 /* syscall */
	ret
	

	.globl _C_LABEL(_set_tss)
_C_LABEL(_set_tss):
	push  %ebp
	mov   %esp, %ebp
	/* TSS (5 + 4*n) */
	movl  8(%ebp), %eax
	shl   $5, %eax
	add $_gdt + (5 * 8), %eax
	movl $0, (%eax)
	movl $0, 4(%eax)
	movl 0xc(%ebp), %ecx
	movw $104, (%eax)
	movw %cx, 2(%eax)
	shr  $16, %ecx
	movb %cl, 4(%eax)
	movb %ch, 7(%eax)
	orl  $0x00408900, 4(%eax)
	pop  %ebp
	ret

	.globl _C_LABEL(_set_fs)
_C_LABEL(_set_fs):
	push  %ebp
	mov   %esp, %ebp
	/* FS (5 + 4*n + 1)*/
	mov  8(%ebp), %ecx
	mov  %ecx, %eax
	shl $5, %eax
	add $_gdt + ((5 + 1) * 8), %eax
	movl $0, (%eax)
	movl $0, 4(%eax)
	movw $4, (%eax)
	movl 0xc(%ebp), %ecx
	movw %cx, 2(%eax)
	shr  $16, %ecx
	movb %cl, 4(%eax)
	movb %ch, 7(%eax)
	orl  $0x00409200, 4(%eax)
	pop  %ebp
	ret

	.globl _C_LABEL(_set_gs)
_C_LABEL(_set_gs):
	push  %ebp
	mov   %esp, %ebp
	/* GS (5 + 4*n + 2)*/
	mov  8(%ebp), %eax
	shl $5, %eax
	add $_gdt + ((5 + 2) * 8), %eax
	movl $0, (%eax)
	movl $0, 4(%eax)
	movw $0xff, (%eax)
	movl 0xc(%ebp), %ecx
	movw %cx, 2(%eax)
	shr  $16, %ecx
	movb %cl, 4(%eax)
	movb %ch, 7(%eax)
	orl  $0x0040f200, 4(%eax)
	pop  %ebp
	ret

	
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

	lgdtl .Lap_gdtreg - _C_LABEL(_ap_start)
	lidtl .Lap_idtreg - _C_LABEL(_ap_start)

	movw $0xb800, %ax
	movw %ax, %es
	movw $('A'<<8 + 'A'), %es:0

	/* Save AP Boostrap Page Segment. */
	mov %cs, %bx

	mov $1, %ax
	lmsw %ax
	ljmpl $0x8, $(apsetup - KERNBASE)

.Lap_gdtreg:
	.hword (GDTSIZE * 8) - 1
	.long _gdt - KERNBASE
.Lap_idtreg:
	.hword (256 * 8) - 1
	.long _idt - KERNBASE
ENTRY(_ap_end)

	.code32
apsetup: /* bx:	AP Bootstrap Segment */
	/* Setup segments. */
	mov $KDS, %ax
	mov %ax, %ss
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	/* Setup CR registers. */
	mov %cr4, %eax
	or  $0x30, %eax
	mov  %eax, %cr4
	mov _bootcr3, %eax
	mov %eax, %cr3
	mov  %cr0, %eax
	or   $((1<<31)|(1<<16)), %eax
	mov  %eax, %cr0
	ljmp $0x08, $1f
1:
	/* Reload GDT and IDT */
	lgdt _gdtr
	lidt _idtr
	ljmp $KCS, $1f
1:
	/* Setup stack */
	xor %eax, %eax
	mov %bx, %ax
	shl $4, %eax
	add $(KERNBASE + 0x1000), %eax
	mov %eax, %esp
	call _C_LABEL(hal_main_ap)
1:	hlt
	jmp 1b


zippo:
	lea zippostring, %esi
	call print
	jmp loop

loop:
	cli
	hlt
	jmp loop

print:
	movl $0x1000, %ecx
	movb $0x87, %ah
	movl $(_physmap_start + 0xb8000), %edi
1:	lodsb
	stosw
	cmp $0, %al
	loopne 1b
	ret

	.data
	.globl _gdtr, _gdt, _idtr, _idt

_bootcr3:
	.long 0

	.align 64
	.globl _gdtr, _gdt
_gdtr:
	.hword (GDTSIZE * 8) - 1
	.long _gdt

	.align 64
_gdt:
	.zero GDTSIZE * 8

	
	.align 64
	.globl _idtr, _idt
_idtr:
	.word (256 * 8) - 1
	.long _idt
_idt:
	.skip  256 * 8

	.align 64
_tss:
	.skip 102
	.hword 104 // IOMAP over the edge.

zippostring:
	.asciz "Zippo."
	

	.align 4096
stackbtm:
	.skip 4096
stacktop:
