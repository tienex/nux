#include <machine/asm.h>
#include "../internal.h"

#define KCS 0x08

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

	mov $0x10, %ax
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
	
	movabs $zippostring, %rdi
	call ___start
	jmp zippo
	

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
	.quad 0                  /* TSS 1/2 */
	.quad 0                  /* TSS 2/2 */
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
