#include <machine/asm.h>

ENTRY (fls)
	bsr	%rdi,%rax
	jz	1f
	inc	%rax
	ret

1:	xor	%rax, %rax
	ret
END (fls)
