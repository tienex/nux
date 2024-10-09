#define VECT_SYSC 0x21
#define str(_x) #_x

#define __SYSCALL0(_a0)					\
  asm volatile						\
  (							\
   "int $" "0x21" ";"					\
   : "+a" (_a0)						\
  );

#define __SYSCALL1(_a0, _a1)				\
  asm volatile (					\
		"int $" "0x21" ";"			\
		: "+a" (_a0)				\
		:"D" (_a1)				\
		);

#define __SYSCALL2(_a0, _a1, _a2)			\
  asm volatile (					\
		"int $" "0x21" ";"			\
		: "+a" (_a0)				\
		:"D" (_a1)				\
		 ,"S" (_a2)				\
		);

#define __SYSCALL3(_a0, _a1, _a2, _a3)			\
  asm volatile (					\
		"int $" "0x21" ";"			\
		: "+a" (_a0)				\
		:"D" (_a1)				\
		 ,"S" (_a2)				\
		 ,"c" (_a3)				\
		);

#define __SYSCALL4(_a0, _a1, _a2, _a3, _a4)		\
  asm volatile (					\
		"int $" "0x21" ";"			\
		: "+a" (_a0)				\
		:"D" (_a1)				\
		 ,"S" (_a2)				\
		 ,"c" (_a3)				\
		 ,"d" (_a4)				\
		);

#define __SYSCALL5(_a0, _a1, _a2, _a3, _a4, _a5)	\
  asm volatile (					\
		"int $" "0x21" ";"			\
		: "+a" (_a0)				\
		:"D" (_a1)				\
		 ,"S" (_a2)				\
		 ,"c" (_a3)				\
		 ,"d" (_a4)				\
		 ,"b" (_a5)				\
		);

#define __SYSCALL6(_a0, _a1, _a2, _a3, _a4, _a5, _a6)	\
  asm volatile (					\
       		"pushl %%ebp;" 				\
		"pushl %6;"				\
		"popl %%ebp;"				\
		"int $" "0x21" ";"			\
		"popl %%ebp;"				\
		: "+a" (_a0)				\
		:"D" (_a1)				\
		 ,"S" (_a2)				\
		 ,"c" (_a3)				\
		 ,"d" (_a4)				\
		 ,"b" (_a5)				\
		 ,"m" (_a6)				\
		);
