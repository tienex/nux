/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * i386 SSE-optimized memory operations
 *
 * Requires: SSE (Pentium III+, Athlon XP+)
 * Uses 128-bit XMM registers with floating-point moves
 * Note: SSE lacks integer SIMD, so uses movups/movaps
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_SSE - SSE-optimized memory copy
 * 4(%esp): destination
 * 8(%esp): source
 * 12(%esp): length
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_SSE)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%edi		/* Destination (after push) */
	movl	16(%esp),%esi		/* Source (after push) */
	movl	20(%esp),%ecx		/* Length (after push) */

	movl	%edi,%eax		/* Save destination for return */

	/* Check for small copies */
	cmpl	$64,%ecx
	jb	.Lsmall_copy_sse

	/* Check alignment */
	movl	%edi,%edx
	andl	$15,%edx		/* Check if destination is 16-byte aligned */
	jz	.Laligned_copy_sse

	/* Align destination to 16 bytes */
	movl	$16,%ebx
	subl	%edx,%ebx		/* Bytes to align */
	subl	%ebx,%ecx		/* Adjust remaining count */

	/* Copy alignment bytes */
.Lalign_loop_sse:
	movb	(%esi),%dl
	movb	%dl,(%edi)
	incl	%esi
	incl	%edi
	decl	%ebx
	jnz	.Lalign_loop_sse

.Laligned_copy_sse:
	/* Main SSE copy loop - 64 bytes per iteration (4x XMM using FP moves) */
	movl	%ecx,%edx
	shrl	$6,%edx			/* Number of 64-byte blocks */
	jz	.Lremainder_sse

.Lsse_loop:
	movups	(%esi),%xmm0
	movups	16(%esi),%xmm1
	movups	32(%esi),%xmm2
	movups	48(%esi),%xmm3

	movaps	%xmm0,(%edi)
	movaps	%xmm1,16(%edi)
	movaps	%xmm2,32(%edi)
	movaps	%xmm3,48(%edi)

	addl	$64,%esi
	addl	$64,%edi
	decl	%edx
	jnz	.Lsse_loop

.Lremainder_sse:
	andl	$63,%ecx		/* Remaining bytes */
	jz	.Ldone_sse

	/* Copy remaining 16-byte chunks */
	movl	%ecx,%edx
	shrl	$4,%edx
	jz	.Ltail_sse

.Lrem16_loop_sse:
	movups	(%esi),%xmm0
	movaps	%xmm0,(%edi)
	addl	$16,%esi
	addl	$16,%edi
	decl	%edx
	jnz	.Lrem16_loop_sse

	andl	$15,%ecx

.Ltail_sse:
	/* Copy remaining bytes */
	testl	%ecx,%ecx
	jz	.Ldone_sse
.Ltail_loop_sse:
	movb	(%esi),%dl
	movb	%dl,(%edi)
	incl	%esi
	incl	%edi
	decl	%ecx
	jnz	.Ltail_loop_sse
	jmp	.Ldone_sse

.Lsmall_copy_sse:
	/* Small copy without SSE */
	rep	movsb

.Ldone_sse:
	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory_SSE)

/*
 * RtlFillMemory_SSE - SSE-optimized memory fill
 * 4(%esp): buffer
 * 8(%esp): length
 * 12(%esp): byte value
 * Returns: buffer pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_SSE)
	pushl	%edi

	movl	8(%esp),%edi		/* Buffer (after push) */
	movl	12(%esp),%ecx		/* Length (after push) */
	movzbl	16(%esp),%eax		/* Byte value (after push) */

	movl	%edi,%edx		/* Save buffer for return */

	/* Replicate byte to all bytes in XMM register using FP moves */
	movl	%eax,%ebx
	shll	$8,%eax
	orl	%ebx,%eax		/* 16-bit pattern */
	movl	%eax,%ebx
	shll	$16,%eax
	orl	%ebx,%eax		/* 32-bit pattern */

	/* Move to stack then load into XMM */
	pushl	%eax
	pushl	%eax
	pushl	%eax
	pushl	%eax
	movups	(%esp),%xmm0		/* Load pattern into XMM */
	addl	$16,%esp		/* Clean stack */

	/* Check for small fills */
	cmpl	$64,%ecx
	jb	.Lsmall_fill_sse

	/* Align destination to 16 bytes */
	movl	%edi,%ebx
	andl	$15,%ebx
	jz	.Laligned_fill_sse

	movl	$16,%ebx
	andl	$15,%ebx
	subl	%ebx,%ecx

	movzbl	16(%esp),%eax		/* Reload byte value for stosb */
.Lalign_fill_loop_sse:
	stosb
	decl	%ebx
	jnz	.Lalign_fill_loop_sse

.Laligned_fill_sse:
	/* Main SSE fill loop - 64 bytes per iteration */
	pushl	%ecx
	shrl	$6,%ecx
	jz	.Lfill_remainder_sse

.Lsse_fill_loop:
	movaps	%xmm0,(%edi)
	movaps	%xmm0,16(%edi)
	movaps	%xmm0,32(%edi)
	movaps	%xmm0,48(%edi)

	addl	$64,%edi
	decl	%ecx
	jnz	.Lsse_fill_loop

.Lfill_remainder_sse:
	popl	%ecx
	andl	$63,%ecx
	jz	.Lfill_done_sse

	/* Fill remaining 16-byte chunks */
	pushl	%ecx
	shrl	$4,%ecx
	jz	.Lfill_tail_sse

.Lrem16_fill_loop_sse:
	movaps	%xmm0,(%edi)
	addl	$16,%edi
	decl	%ecx
	jnz	.Lrem16_fill_loop_sse

	popl	%ecx
	andl	$15,%ecx
	jmp	.Lfill_tail_bytes_sse

.Lfill_tail_sse:
	popl	%ecx

.Lfill_tail_bytes_sse:
	testl	%ecx,%ecx
	jz	.Lfill_done_sse
	movzbl	16(%esp),%eax		/* Reload byte value */
	rep	stosb
	jmp	.Lfill_done_sse

.Lsmall_fill_sse:
	movzbl	16(%esp),%eax
	rep	stosb

.Lfill_done_sse:
	movl	%edx,%eax		/* Return buffer */
	popl	%edi
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_SSE)
