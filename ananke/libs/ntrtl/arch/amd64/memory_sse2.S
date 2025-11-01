/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 SSE2-optimized memory operations
 *
 * Requires: SSE2 (baseline for x86-64, always available)
 * Uses 128-bit XMM registers for fast bulk operations
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_SSE2 - SSE2-optimized memory copy
 * %rdi: destination
 * %rsi: source
 * %rdx: length
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_SSE2)
	movq	%rdi,%rax		/* Save destination for return */
	movq	%rdx,%rcx		/* Copy length */

	/* Check for small copies */
	cmpq	$64,%rcx
	jb	.Lsmall_copy

	/* Check alignment */
	movq	%rdi,%r8
	andq	$15,%r8			/* Check if destination is 16-byte aligned */
	jz	.Laligned_copy

	/* Align destination to 16 bytes */
	movq	$16,%r9
	subq	%r8,%r9			/* Bytes to align */
	subq	%r9,%rcx		/* Adjust remaining count */

	/* Copy alignment bytes */
.Lalign_loop:
	movb	(%rsi),%r8b
	movb	%r8b,(%rdi)
	incq	%rsi
	incq	%rdi
	decq	%r9
	jnz	.Lalign_loop

.Laligned_copy:
	/* Main SSE2 copy loop - 64 bytes per iteration */
	movq	%rcx,%r9
	shrq	$6,%r9			/* Number of 64-byte blocks */
	jz	.Lremainder

.Lsse2_loop:
	movdqu	(%rsi),%xmm0
	movdqu	16(%rsi),%xmm1
	movdqu	32(%rsi),%xmm2
	movdqu	48(%rsi),%xmm3

	movdqa	%xmm0,(%rdi)
	movdqa	%xmm1,16(%rdi)
	movdqa	%xmm2,32(%rdi)
	movdqa	%xmm3,48(%rdi)

	addq	$64,%rsi
	addq	$64,%rdi
	decq	%r9
	jnz	.Lsse2_loop

.Lremainder:
	andq	$63,%rcx		/* Remaining bytes */
	jz	.Ldone

	/* Copy remaining 16-byte chunks */
	movq	%rcx,%r9
	shrq	$4,%r9
	jz	.Ltail

.Lrem16_loop:
	movdqu	(%rsi),%xmm0
	movdqu	%xmm0,(%rdi)
	addq	$16,%rsi
	addq	$16,%rdi
	decq	%r9
	jnz	.Lrem16_loop

	andq	$15,%rcx

.Ltail:
	/* Copy remaining bytes */
	testq	%rcx,%rcx
	jz	.Ldone
.Ltail_loop:
	movb	(%rsi),%r8b
	movb	%r8b,(%rdi)
	incq	%rsi
	incq	%rdi
	decq	%rcx
	jnz	.Ltail_loop
	jmp	.Ldone

.Lsmall_copy:
	/* Small copy without SSE2 */
	rep	movsb

.Ldone:
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory_SSE2)

/*
 * RtlFillMemory_SSE2 - SSE2-optimized memory fill
 * %rdi: buffer
 * %rsi: byte value
 * %rdx: length
 * Returns: buffer pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_SSE2)
	movq	%rdi,%rax		/* Save buffer for return */
	movzbq	%sil,%rsi		/* Zero-extend byte value */

	/* Replicate byte to all bytes in XMM register */
	movq	%rsi,%r8
	shlq	$8,%rsi
	orq	%r8,%rsi		/* 16-bit pattern */
	movq	%rsi,%r8
	shlq	$16,%rsi
	orq	%r8,%rsi		/* 32-bit pattern */
	movq	%rsi,%r8
	shlq	$32,%rsi
	orq	%r8,%rsi		/* 64-bit pattern */

	movq	%rsi,%xmm0
	punpcklqdq %xmm0,%xmm0		/* Fill entire XMM with pattern */

	movq	%rdx,%rcx		/* Copy length */

	/* Check for small fills */
	cmpq	$64,%rcx
	jb	.Lsmall_fill

	/* Align destination to 16 bytes */
	movq	%rdi,%r8
	andq	$15,%r8
	jz	.Laligned_fill

	movq	$16,%r9
	subq	%r8,%r9
	subq	%r9,%rcx

.Lalign_fill_loop:
	movb	%sil,(%rdi)
	incq	%rdi
	decq	%r9
	jnz	.Lalign_fill_loop

.Laligned_fill:
	/* Main SSE2 fill loop - 64 bytes per iteration */
	movq	%rcx,%r9
	shrq	$6,%r9
	jz	.Lfill_remainder

.Lsse2_fill_loop:
	movdqa	%xmm0,(%rdi)
	movdqa	%xmm0,16(%rdi)
	movdqa	%xmm0,32(%rdi)
	movdqa	%xmm0,48(%rdi)

	addq	$64,%rdi
	decq	%r9
	jnz	.Lsse2_fill_loop

.Lfill_remainder:
	andq	$63,%rcx
	jz	.Lfill_done

	/* Fill remaining 16-byte chunks */
	movq	%rcx,%r9
	shrq	$4,%r9
	jz	.Lfill_tail

.Lrem16_fill_loop:
	movdqa	%xmm0,(%rdi)
	addq	$16,%rdi
	decq	%r9
	jnz	.Lrem16_fill_loop

	andq	$15,%rcx

.Lfill_tail:
	testq	%rcx,%rcx
	jz	.Lfill_done
.Lfill_tail_loop:
	movb	%sil,(%rdi)
	incq	%rdi
	decq	%rcx
	jnz	.Lfill_tail_loop
	jmp	.Lfill_done

.Lsmall_fill:
	rep	stosb

.Lfill_done:
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_SSE2)
