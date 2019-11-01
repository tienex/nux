#include <machine/asm.h>
#include "../internal.h"

	.altmacro
.macro _do_entry vct
	push %r15
	push %r14
	push %r13
	push %r12
	push %r11
	push %r10
	push %r9
	push %r8
	push %rdi
	push %rsi
	push %rbp
	push %rdx
	push %rcx
	push %rbx
	push %rax
	mov %cr3, %rax
	push %rax
	mov %cr2, %rax
	push %rax
	/* XXX: fs, gs handling */
	/* End of frame. */

	mov %rsp, %rsi
	mov $\vct, %rdi

	.if \vct == 2
	call _C_LABEL(do_nmi)
	.elseif \vct < 32
	call _C_LABEL(do_xcpt)
	.else
	call _C_LABEL(do_intr)
	.endif

	mov %rax, %rsp
.endm


.macro _do_iret_exit
	/* Skip CR3, CR2 */
	add $16, %rsp
	popq %rax
	popq %rbx
	popq %rcx
	popq %rdx
	popq %rbp
	popq %rsi
	popq %rdi
	popq %r8
	popq %r9
	popq %r10
	popq %r11
	popq %r12
	popq %r13
	popq %r14
	popq %r15

	/* Skip error */
	add $8, %rsp
	iretq
.endm

.macro handler_e vct
	.globl hdlr_&\vct
hdlr_&\vct&:
	_do_entry \vct
	_do_iret_exit
.endm

.macro handler vct
	.globl hdlr_&\vct
hdlr_&\vct&:
	pushq $0
	_do_entry \vct
	_do_iret_exit
.endm
	

.macro repeat macro from to
	\macro \from
	.if \to-\from
	repeat \macro,%(\from+1),\to
	.endif
.endm

	repeat handler 0, 7
	handler 8
	handler 9
	repeat handler_e 10, 14
	handler 15
	handler 16
	handler_e 17
	repeat handler 18, 31
	repeat handler 32, 63
	repeat handler 64, 127
	repeat handler 128, 191
	repeat handler 192, 255
