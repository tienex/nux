/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * x32 optimized memory operations
 *
 * x32 is the ILP32 ABI for x86-64 (32-bit pointers, 64-bit instructions)
 *
 * Adapted for x32 ABI:
 * - Pointers are 32-bit (%edi, %esi instead of %rdi, %rsi)
 * - Sizes can be 32-bit or 64-bit depending on type
 * - Can still use full 64-bit operations for performance
 * - Return addresses and pointers use 32-bit registers
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory - Copy memory (non-overlapping buffers)
 * %edi: destination (32-bit pointer)
 * %esi: source (32-bit pointer)
 * %edx: length (32-bit size)
 * Returns: destination pointer (32-bit)
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory)
	mov	%edx,%ecx
	mov	%edi,%eax	/* return destination */
	mov	%edi,%r11d

	shr	$3,%ecx
	jz	8f

	lea	-8(%rdi,%rdx),%r9d
	mov	-8(%rsi,%rdx),%r8
	lea	-8(%rsi,%rdx),%r10d

	/* Check for partial overlap (dest < src) */
	cmp	%edi,%r9d
	jb	10f

	/* Copy forwards */
	rep
	movsq
	movq	%r8,(%r9)
	ret

8:	/* Less than 8 bytes */
	testb	$4,%dl
	jz	1f
	movsl
1:	testb	$2,%dl
	jz	1f
	movsw
1:	testb	$1,%dl
	jz	1f
	movsb
1:	ret

10:	/* Potential overlap - use memmove logic */
	cmp	%esi,%r10d
	jae	12f

	/* Copy backwards */
	lea	-8(%rdi,%rdx),%edi
	lea	-8(%rsi,%rdx),%esi
	std
	rep
	movsq
	cld
	movq	%r8,(%r9)
	ret

12:	/* No overlap, copy forwards */
	rep
	movsq
	movq	%r8,(%r9)
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory)

/*
 * RtlMoveMemory - Move memory (may overlap)
 * %edi: destination
 * %esi: source
 * %edx: length
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlMoveMemory)
	mov	%edx,%ecx
	mov	%edi,%eax	/* return destination */

	mov	%edi,%r8d
	sub	%esi,%r8d

	shr	$3,%ecx
	jz	8f

	/* Check direction of overlap */
	cmp	%edx,%r8d
	jb	10f

	/* No overlap or forward copy is safe */
	rep
	movsq

	/* Handle remaining bytes */
	mov	%edx,%ecx
	and	$7,%ecx
	rep
	movsb
	ret

8:	/* Less than 8 bytes */
	mov	%edx,%ecx
	rep
	movsb
	ret

10:	/* Overlapping, copy backwards */
	lea	-1(%rdi,%rdx),%edi
	lea	-1(%rsi,%rdx),%esi
	mov	%edx,%ecx
	std
	rep
	movsb
	cld
	ret
ANX_ASM_FUNCTION_END(RtlMoveMemory)

/*
 * RtlCompareMemory - Compare memory regions
 * %edi: buffer1
 * %esi: buffer2
 * %edx: length
 * Returns: number of matching bytes
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareMemory)
	xor	%eax,%eax
	mov	%edx,%ecx
	repe	cmpsb
	jne	1f
	mov	%edx,%eax	/* all bytes matched */
1:	ret
ANX_ASM_FUNCTION_END(RtlCompareMemory)

/*
 * RtlFillMemory - Fill memory with byte value
 * %edi: destination
 * %esi: fill value (byte in %sil)
 * %edx: length
 * Returns: void
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory)
	mov	%sil,%al
	mov	%edx,%ecx
	rep
	stosb
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory)

/*
 * RtlZeroMemory - Zero memory
 * %edi: destination
 * %esi: length
 * Returns: void
 */
ANX_ASM_FUNCTION_BEGIN(RtlZeroMemory)
	xor	%eax,%eax
	mov	%esi,%ecx
	rep
	stosb
	ret
ANX_ASM_FUNCTION_END(RtlZeroMemory)

/*
 * RtlFindByteInMemory - Find byte in memory region
 * %edi: buffer
 * %esi: length
 * %edx: byte to find (in %dl)
 * Returns: pointer to first occurrence, or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindByteInMemory)
	mov	%dl,%al
	mov	%esi,%ecx
	repne	scasb
	jne	1f
	lea	-1(%rdi),%eax	/* found, return pointer */
	ret
1:	xor	%eax,%eax	/* not found */
	ret
ANX_ASM_FUNCTION_END(RtlFindByteInMemory)
