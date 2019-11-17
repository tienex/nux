#define VECT_SYSC 0x21

#define _str(_x) #_x
#define ___systrap(_vect)	\
	"movl %0, %%eax;"	\
	"int $" _str(_vect)

#define __systrap ___systrap(VECT_SYSC)


#define __syscall0(__sys, __ret)	\
	asm volatile(__systrap		\
		: "=a" (__ret)		\
		: "a" (__sys));

#define __syscall1(__sys, a1, __ret)	\
	asm volatile(__systrap		\
		: "=a" (__ret)		\
		: "a" (__sys),		\
		  "D" (a1));

#define __syscall2(__sys, a1, a2, __ret)\
	asm volatile(__systrap		\
		: "=a" (__ret)		\
		: "a" (__sys),		\
		  "D" (a1),		\
		  "S" (a2));


#define __syscall3(__sys, a1, a2, a3, __ret)	\
	asm volatile(__systrap			\
		: "=a" (__ret)			\
		: "a" (__sys),			\
		  "D" (a1),			\
		  "S" (a2), 			\
		  "c" (a3));


#define __syscall4(__sys, a1, a2, a3, a4, __ret)\
	asm volatile(__systrap			\
		: "=a" (__ret)			\
		: "a" (__sys),			\
		  "D" (a1),			\
		  "S" (a2), 			\
		  "c" (a3),			\
		  "d" (a4));

#define __syscall5(__sys, a1, a2, a3, a4, a5, __ret)	\
	asm volatile(__systrap				\
		: "=a" (__ret)				\
		: "a" (__sys),				\
		  "D" (a1),				\
		  "S" (a2), 				\
		  "c" (a3),				\
		  "d" (a4),				\
		  "b" (a5));
