#define __SYSCALL0(__sys)				\
  do {							\
  register unsigned long ret __asm__("a0") = (__sys);	\
  asm volatile("ecall"					\
	       : "+r"(ret));				\
  __sys = ret;						\
  } while (0)

#define __SYSCALL1(__sys, a1)				\
  do {							\
  register unsigned long ret __asm__("a0") = (__sys);	\
  register unsigned long ra1 __asm__("a1") = (a1);	\
  asm volatile("ecall"					\
	       : "+r"(ret)				\
	       : "r"(ra1)				\
	       );					\
  __sys = ret;						\
  } while (0)

#define __SYSCALL2(__sys, a1, a2)			\
  do {							\
  register unsigned long ret __asm__("a0") = (__sys);	\
  register unsigned long ra1 __asm__("a1") = (a1);	\
  register unsigned long ra2 __asm__("a2") = (a2);	\
  asm volatile("ecall"					\
	       : "+r"(ret)				\
	       : "r"(ra1),				\
		 "r"(ra2)				\
	       );					\
  __sys = ret;						\
  } while (0)

#define __SYSCALL3(__sys, a1, a2, a3)			\
  do {							\
  register unsigned long ret __asm__("a0") = (__sys);	\
  register unsigned long ra1 __asm__("a1") = (a1);	\
  register unsigned long ra2 __asm__("a2") = (a2);	\
  register unsigned long ra3 __asm__("a3") = (a3);	\
  asm volatile("ecall"					\
	       : "+r"(ret)				\
	       : "r"(ra1),				\
		 "r"(ra2),				\
		 "r"(ra3)				\
	       );					\
  __sys = ret;						\
  } while (0)

#define __SYSCALL4(__sys, a1, a2, a3, a4)		\
  do {							\
  register unsigned long ret __asm__("a0") = (__sys);	\
  register unsigned long ra1 __asm__("a1") = (a1);	\
  register unsigned long ra2 __asm__("a2") = (a2);	\
  register unsigned long ra3 __asm__("a3") = (a3);	\
  register unsigned long ra4 __asm__("a4") = (a4);	\
  asm volatile("ecall"					\
	       : "+r"(ret)				\
	       : "r"(ra1),				\
		 "r"(ra2),				\
		 "r"(ra3),				\
		 "r"(ra4)				\
	       );					\
  __sys = ret;						\
  } while (0)

#define __SYSCALL5(__sys, a1, a2, a3, a4, a5)		\
  do {							\
  register unsigned long ret __asm__("a0") = (__sys);	\
  register unsigned long ra1 __asm__("a1") = (a1);	\
  register unsigned long ra2 __asm__("a2") = (a2);	\
  register unsigned long ra3 __asm__("a3") = (a3);	\
  register unsigned long ra4 __asm__("a4") = (a4);	\
  register unsigned long ra5 __asm__("a5") = (a5);	\
  asm volatile("ecall"					\
	       : "+r"(ret)				\
	       : "r"(ra1),				\
		 "r"(ra2),				\
		 "r"(ra3),				\
		 "r"(ra4),				\
		 "r"(ra5)				\
	       );					\
  __sys = ret;						\
  } while (0)
