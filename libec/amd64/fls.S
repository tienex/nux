#include <machine/asm.h>

ENTRY (fls)
	bsrl	%edi,%eax
	jz	1f
	incl	%eax
	ret

1:	xorl	%eax, %eax
	ret
END (fls)
