#include <machine/asm.h>

	.globl _start
_start:
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop
	la sp, stacktop

#	la t0, boothid
#	sw a0, 0(t0)

	la t0, dtbptr
	sw a1, 0(t0)

	la t0, __bss_start
	la t1, __BSS_END__
1:
	bge t0, t1, 1f
	sw zero, 0(t0)
	addi t0, t0, 4
	j 1b
1:
	mv a0, zero		;
	call ___start
1:	j 1b


	.data
	.globl boothid, dtbptr
boothid:
	.quad 0
dtbptr:
	.quad 0

	.align 12
stackbtm:
	.skip 4096
stacktop:
