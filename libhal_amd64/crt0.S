#include <machine/asm.h>
#include "../internal.h"

#define KCS 0x08

	.text
	.globl _start
_start:
	movabs $_stacktop, %rsp

	movabs $_gdtr, %rax
	lgdt (%rax)


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
	.quad 0x00a09a0000000000 /* Kernel Code */
	.quad 0                  /* Kernel Data */
	.quad 0x00a0fa0000000000 /* User Code */
	.quad 0                  /* User Data */
	.quad 0                  /* TSS1 */
	.quad 0                  /* TSS2 */
_end_gdt:

	.align 4096
_stackbtm:
	.zero 4096
_stacktop:	
