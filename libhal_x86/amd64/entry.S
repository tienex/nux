#include <machine/asm.h>
#include "../internal.h"

	.altmacro
.macro _do_entry vct
.endm


.macro _do_iret_exit
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
