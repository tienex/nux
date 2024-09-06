#include <machine/asm.h>
#include "riscv.h"

	.globl _start
	.align 8
_start:
	csrw satp, a0

	/* Set TP to zero, to catch when it is used uninitialised. */
	mv tp, x0

	/* Set sstatus to zero. Importantly, disable interrupts and
	set VS, FS, and XS to off. */
	csrw sstatus, x0

	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop
	la sp, stacktop

	la t0, _sbss
	la t1, _ebss
1:
	bge t0, t1, 1f
	sw zero, 0(t0)
	addi t0, t0, 4
	j 1b
1:

	la t0, _early_intrentry
	csrw stvec, t0

	call riscv_init
	
	mv a0, zero		;
	call ___start

1:	j 1b


#define _EXPORT_FAR_PTR(_x) \
	.globl _riscv64##_x	; \
_riscv64##_x : .dword _x 	;
	
	
	.data

	_EXPORT_FAR_PTR(_kmem_start)
	_EXPORT_FAR_PTR(_kmem_end)

	_EXPORT_FAR_PTR(_kva_start)
	_EXPORT_FAR_PTR(_kva_end)

	_EXPORT_FAR_PTR(_physmap_start)
	_EXPORT_FAR_PTR(_physmap_end)

	_EXPORT_FAR_PTR(_pfncache_start)
	_EXPORT_FAR_PTR(_pfncache_end)

	.align 12
stackbtm:
	.skip 4096
stacktop:
