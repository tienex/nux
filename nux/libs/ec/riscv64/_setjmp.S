/* Common setjmp/longjmp implementation save float status when the
   softfloat abi is on. Let's ignore all that. */

#include <machine/asm.h>
#include <machine/setjmp.h>

ENTRY(_setjmp)
	sd	ra, _JB_RA(a0)		/* save return address */
	sd	sp, _JB_SP(a0)		/* save stack pointer */
	sd	s0, _JB_S0(a0)		/* save callee saved register */
	sd	s1, _JB_S1(a0)		/* save callee saved register */
	sd	s2, _JB_S2(a0)		/* save callee saved register */
	sd	s3, _JB_S3(a0)		/* save callee saved register */
	sd	s4, _JB_S4(a0)		/* save callee saved register */
	sd	s5, _JB_S5(a0)		/* save callee saved register */
	sd	s6, _JB_S6(a0)		/* save callee saved register */
	sd	s7, _JB_S7(a0)		/* save callee saved register */
	sd	s8, _JB_S8(a0)		/* save callee saved register */
	sd	s9, _JB_S9(a0)		/* save callee saved register */
	sd	s10, _JB_S10(a0)		/* save callee saved register */
	sd	s11, _JB_S11(a0)		/* save callee saved register */
	li	a0, 0			/* indicate success */
	ret				/* return */
END(_setjmp)

ENTRY(_longjmp)
	ld	ra, _JB_RA(a0)		/* save return address */
	ld	sp, _JB_SP(a0)		/* save stack pointer */
	ld	s0, _JB_S0(a0)		/* save callee saved register */
	ld	s1, _JB_S1(a0)		/* save callee saved register */
	ld	s2, _JB_S2(a0)		/* save callee saved register */
	ld	s3, _JB_S3(a0)		/* save callee saved register */
	ld	s4, _JB_S4(a0)		/* save callee saved register */
	ld	s5, _JB_S5(a0)		/* save callee saved register */
	ld	s6, _JB_S6(a0)		/* save callee saved register */
	ld	s7, _JB_S7(a0)		/* save callee saved register */
	ld	s8, _JB_S8(a0)		/* save callee saved register */
	ld	s9, _JB_S9(a0)		/* save callee saved register */
	ld	s10, _JB_S10(a0)		/* save callee saved register */
	ld	s11, _JB_S11(a0)		/* save callee saved register */
	li	a0, 1			/* default return value */
	beqz	a1, 1f			/* test return value */
	mv	a0, a1			/* use it if not 0 */
1:	ret
END(_longjmp)

ENTRY(_setupjmp)
	sd a1, _JB_RA(a0)
	sd sp, _JB_SP(a0)
	ret
END(_setupjmp)
