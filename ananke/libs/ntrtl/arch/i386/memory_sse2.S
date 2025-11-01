/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * i386 SSE2-optimized memory operations
 *
 * Requires: SSE2 (Pentium 4+, AthlonXP+)
 * Uses 128-bit XMM registers for fast bulk operations
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_SSE2 - SSE2-optimized memory copy
 * 4(%esp): destination
 * 8(%esp): source
 * 12(%esp): length
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_SSE2)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%edi		/* Destination (after push) */
	movl	16(%esp),%esi		/* Source (after push) */
	movl	20(%esp),%ecx		/* Length (after push) */

	movl	%edi,%eax		/* Save destination for return */

	/* Check for small copies */
	cmpl	$64,%ecx
	jb	.Lsmall_copy_i386

	/* Check alignment */
	movl	%edi,%edx
	andl	$15,%edx		/* Check if destination is 16-byte aligned */
	jz	.Laligned_copy_i386

	/* Align destination to 16 bytes */
	movl	$16,%ebx
	subl	%edx,%ebx		/* Bytes to align */
	subl	%ebx,%ecx		/* Adjust remaining count */

	/* Copy alignment bytes */
.Lalign_loop_i386:
	movb	(%esi),%dl
	movb	%dl,(%edi)
	incl	%esi
	incl	%edi
	decl	%ebx
	jnz	.Lalign_loop_i386

.Laligned_copy_i386:
	/* Main SSE2 copy loop - 64 bytes per iteration */
	movl	%ecx,%edx
	shrl	$6,%edx			/* Number of 64-byte blocks */
	jz	.Lremainder_i386

.Lsse2_loop_i386:
	movdqu	(%esi),%xmm0
	movdqu	16(%esi),%xmm1
	movdqu	32(%esi),%xmm2
	movdqu	48(%esi),%xmm3

	movdqa	%xmm0,(%edi)
	movdqa	%xmm1,16(%edi)
	movdqa	%xmm2,32(%edi)
	movdqa	%xmm3,48(%edi)

	addl	$64,%esi
	addl	$64,%edi
	decl	%edx
	jnz	.Lsse2_loop_i386

.Lremainder_i386:
	andl	$63,%ecx		/* Remaining bytes */
	jz	.Ldone_i386

	/* Copy remaining 16-byte chunks */
	movl	%ecx,%edx
	shrl	$4,%edx
	jz	.Ltail_i386

.Lrem16_loop_i386:
	movdqu	(%esi),%xmm0
	movdqu	%xmm0,(%edi)
	addl	$16,%esi
	addl	$16,%edi
	decl	%edx
	jnz	.Lrem16_loop_i386

	andl	$15,%ecx

.Ltail_i386:
	/* Copy remaining bytes */
	testl	%ecx,%ecx
	jz	.Ldone_i386
.Ltail_loop_i386:
	movb	(%esi),%dl
	movb	%dl,(%edi)
	incl	%esi
	incl	%edi
	decl	%ecx
	jnz	.Ltail_loop_i386
	jmp	.Ldone_i386

.Lsmall_copy_i386:
	/* Small copy without SSE2 */
	rep	movsb

.Ldone_i386:
	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory_SSE2)

/*
 * RtlFillMemory_SSE2 - SSE2-optimized memory fill
 * 4(%esp): buffer
 * 8(%esp): length
 * 12(%esp): byte value
 * Returns: buffer pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_SSE2)
	pushl	%edi

	movl	8(%esp),%edi		/* Buffer (after push) */
	movl	12(%esp),%ecx		/* Length (after push) */
	movzbl	16(%esp),%eax		/* Byte value (after push) */

	movl	%edi,%edx		/* Save buffer for return */

	/* Replicate byte to all bytes in XMM register */
	movl	%eax,%ebx
	shll	$8,%eax
	orl	%ebx,%eax		/* 16-bit pattern */
	movl	%eax,%ebx
	shll	$16,%eax
	orl	%ebx,%eax		/* 32-bit pattern */

	movd	%eax,%xmm0
	pshufd	$0,%xmm0,%xmm0		/* Broadcast to entire XMM */

	/* Check for small fills */
	cmpl	$64,%ecx
	jb	.Lsmall_fill_i386

	/* Align destination to 16 bytes */
	movl	%edi,%ebx
	andl	$15,%ebx
	jz	.Laligned_fill_i386

	movl	$16,%ebx
	subl	%ebx,%ebx
	subl	%ebx,%ecx

.Lalign_fill_loop_i386:
	stosb
	decl	%ebx
	jnz	.Lalign_fill_loop_i386

.Laligned_fill_i386:
	/* Main SSE2 fill loop - 64 bytes per iteration */
	pushl	%ecx
	shrl	$6,%ecx
	jz	.Lfill_remainder_i386

.Lsse2_fill_loop_i386:
	movdqa	%xmm0,(%edi)
	movdqa	%xmm0,16(%edi)
	movdqa	%xmm0,32(%edi)
	movdqa	%xmm0,48(%edi)

	addl	$64,%edi
	decl	%ecx
	jnz	.Lsse2_fill_loop_i386

.Lfill_remainder_i386:
	popl	%ecx
	andl	$63,%ecx
	jz	.Lfill_done_i386

	/* Fill remaining 16-byte chunks */
	pushl	%ecx
	shrl	$4,%ecx
	jz	.Lfill_tail_i386

.Lrem16_fill_loop_i386:
	movdqa	%xmm0,(%edi)
	addl	$16,%edi
	decl	%ecx
	jnz	.Lrem16_fill_loop_i386

	popl	%ecx
	andl	$15,%ecx
	jmp	.Lfill_tail_bytes_i386

.Lfill_tail_i386:
	popl	%ecx

.Lfill_tail_bytes_i386:
	testl	%ecx,%ecx
	jz	.Lfill_done_i386
	rep	stosb
	jmp	.Lfill_done_i386

.Lsmall_fill_i386:
	rep	stosb

.Lfill_done_i386:
	movl	%edx,%eax		/* Return buffer */
	popl	%edi
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_SSE2)
