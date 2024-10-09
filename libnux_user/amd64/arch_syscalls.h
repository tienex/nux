#define __SYSCALL0(__sys)			\
  asm volatile(					\
	       "syscall;" :			\
	       "+a" (__sys) :			\
  );

#define __SYSCALL1(__sys, a1)			\
  asm volatile(					\
	       "syscall;" :			\
	       "+a" (__sys) :			\
	       "D" (a1)				\
  );

#define __SYSCALL2(__sys, a1, a2)		\
  asm volatile(					\
	       "syscall;" :			\
	       "+a" (__sys) :			\
	       "D" (a1),			\
	       "S" (a2) 			\
  );

#define __SYSCALL3(__sys, a1, a2, a3)		\
  asm volatile(					\
	       "syscall;" :			\
	       "+a" (__sys) :			\
	       "D" (a1),			\
	       "S" (a2), 			\
	       "d" (a3)				\
  );

#define __SYSCALL4(__sys, a1, a2, a3, a4)	\
  asm volatile(					\
	       "syscall;" :			\
	       "+a" (__sys) :			\
	       "D" (a1),			\
	       "S" (a2), 			\
	       "d" (a3),			\
	       "b" (a4)				\
  );

#define __SYSCALL5(__sys, a1, a2, a3, a4, a5)		\
  asm volatile(						\
	       "mov %5, %%r8;"				\
	       "syscall;" :   				\
	       "+a" (__sys) :				\
	       "D" (a1),				\
	       "S" (a2), 				\
	       "d" (a3),				\
	       "b" (a4),				\
	       "r" (a5)      				\
  );

#define __SYSCALL6(__sys, a1, a2, a3, a4, a5, a6)      	\
  asm volatile(						\
	       "mov %5, %%r8;"				\
	       "mov %6, %%r9;"				\
	       "syscall;" :   				\
	       "+a" (__sys) :				\
	       "D" (a1),				\
	       "S" (a2), 				\
	       "d" (a3),				\
	       "b" (a4),				\
	       "r" (a5),				\
	       "r" (a6) : "r8", "r9"   			\
  );
