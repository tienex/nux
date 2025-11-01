/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * i386 MMX-optimized memory operations
 *
 * Requires: MMX (Pentium MMX+, K6+)
 * Uses 64-bit MM registers for SIMD operations
 * Note: MMX is mostly deprecated but still useful for legacy systems
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_MMX - MMX-optimized memory copy
 * 4(%esp): destination
 * 8(%esp): source
 * 12(%esp): length
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_MMX)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%edi		/* Destination (after push) */
	movl	16(%esp),%esi		/* Source (after push) */
	movl	20(%esp),%ecx		/* Length (after push) */

	movl	%edi,%eax		/* Save destination for return */

	/* Check for small copies */
	cmpl	$32,%ecx
	jb	.Lsmall_copy_mmx

	/* Check alignment */
	movl	%edi,%edx
	andl	$7,%edx			/* Check if destination is 8-byte aligned */
	jz	.Laligned_copy_mmx

	/* Align destination to 8 bytes */
	movl	$8,%ebx
	subl	%edx,%ebx		/* Bytes to align */
	subl	%ebx,%ecx		/* Adjust remaining count */

	/* Copy alignment bytes */
.Lalign_loop_mmx:
	movb	(%esi),%dl
	movb	%dl,(%edi)
	incl	%esi
	incl	%edi
	decl	%ebx
	jnz	.Lalign_loop_mmx

.Laligned_copy_mmx:
	/* Main MMX copy loop - 32 bytes per iteration (4x MMX) */
	movl	%ecx,%edx
	shrl	$5,%edx			/* Number of 32-byte blocks */
	jz	.Lremainder_mmx

.Lmmx_loop:
	movq	(%esi),%mm0
	movq	8(%esi),%mm1
	movq	16(%esi),%mm2
	movq	24(%esi),%mm3

	movq	%mm0,(%edi)
	movq	%mm1,8(%edi)
	movq	%mm2,16(%edi)
	movq	%mm3,24(%edi)

	addl	$32,%esi
	addl	$32,%edi
	decl	%edx
	jnz	.Lmmx_loop

.Lremainder_mmx:
	andl	$31,%ecx		/* Remaining bytes */
	jz	.Ldone_mmx

	/* Copy remaining 8-byte chunks */
	movl	%ecx,%edx
	shrl	$3,%edx
	jz	.Ltail_mmx

.Lrem8_loop_mmx:
	movq	(%esi),%mm0
	movq	%mm0,(%edi)
	addl	$8,%esi
	addl	$8,%edi
	decl	%edx
	jnz	.Lrem8_loop_mmx

	andl	$7,%ecx

.Ltail_mmx:
	/* Copy remaining bytes */
	testl	%ecx,%ecx
	jz	.Ldone_mmx
.Ltail_loop_mmx:
	movb	(%esi),%dl
	movb	%dl,(%edi)
	incl	%esi
	incl	%edi
	decl	%ecx
	jnz	.Ltail_loop_mmx
	jmp	.Ldone_mmx

.Lsmall_copy_mmx:
	/* Small copy without MMX */
	rep	movsb

.Ldone_mmx:
	emms				/* Clear MMX state */
	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory_MMX)

/*
 * RtlFillMemory_MMX - MMX-optimized memory fill
 * 4(%esp): buffer
 * 8(%esp): length
 * 12(%esp): byte value
 * Returns: buffer pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_MMX)
	pushl	%edi

	movl	8(%esp),%edi		/* Buffer (after push) */
	movl	12(%esp),%ecx		/* Length (after push) */
	movzbl	16(%esp),%eax		/* Byte value (after push) */

	movl	%edi,%edx		/* Save buffer for return */

	/* Replicate byte to all bytes in MMX register */
	movl	%eax,%ebx
	shll	$8,%eax
	orl	%ebx,%eax		/* 16-bit pattern */
	movl	%eax,%ebx
	shll	$16,%eax
	orl	%ebx,%eax		/* 32-bit pattern */

	movd	%eax,%mm0
	punpckldq %mm0,%mm0		/* Replicate to 64-bit */

	/* Check for small fills */
	cmpl	$32,%ecx
	jb	.Lsmall_fill_mmx

	/* Align destination to 8 bytes */
	movl	%edi,%ebx
	andl	$7,%ebx
	jz	.Laligned_fill_mmx

	movl	$8,%ebx
	andl	$7,%ebx
	subl	%ebx,%ecx

.Lalign_fill_loop_mmx:
	stosb
	decl	%ebx
	jnz	.Lalign_fill_loop_mmx

.Laligned_fill_mmx:
	/* Main MMX fill loop - 32 bytes per iteration */
	pushl	%ecx
	shrl	$5,%ecx
	jz	.Lfill_remainder_mmx

.Lmmx_fill_loop:
	movq	%mm0,(%edi)
	movq	%mm0,8(%edi)
	movq	%mm0,16(%edi)
	movq	%mm0,24(%edi)

	addl	$32,%edi
	decl	%ecx
	jnz	.Lmmx_fill_loop

.Lfill_remainder_mmx:
	popl	%ecx
	andl	$31,%ecx
	jz	.Lfill_done_mmx

	/* Fill remaining 8-byte chunks */
	pushl	%ecx
	shrl	$3,%ecx
	jz	.Lfill_tail_mmx

.Lrem8_fill_loop_mmx:
	movq	%mm0,(%edi)
	addl	$8,%edi
	decl	%ecx
	jnz	.Lrem8_fill_loop_mmx

	popl	%ecx
	andl	$7,%ecx
	jmp	.Lfill_tail_bytes_mmx

.Lfill_tail_mmx:
	popl	%ecx

.Lfill_tail_bytes_mmx:
	testl	%ecx,%ecx
	jz	.Lfill_done_mmx
	rep	stosb
	jmp	.Lfill_done_mmx

.Lsmall_fill_mmx:
	rep	stosb

.Lfill_done_mmx:
	emms				/* Clear MMX state */
	movl	%edx,%eax		/* Return buffer */
	popl	%edi
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_MMX)
