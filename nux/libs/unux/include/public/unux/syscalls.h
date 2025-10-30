#ifndef __unux_syscalls_h__
#define __unux_syscalls_h__

long syscall0 (unsigned INTN sys);
long syscall1 (unsigned INTN sys, unsigned INTN arg1);
long syscall2 (unsigned INTN sys, unsigned INTN arg1, unsigned INTN arg2);
long syscall3 (unsigned INTN sys, unsigned INTN arg1, unsigned INTN arg2,
	       unsigned INTN arg3);
long syscall4 (unsigned INTN sys, unsigned INTN arg1, unsigned INTN arg2,
	       unsigned INTN arg3, unsigned INTN arg4);
long syscall5 (unsigned INTN sys, unsigned INTN arg1, unsigned INTN arg2,
	       unsigned INTN arg3, unsigned INTN arg4, unsigned INTN arg5);
long syscall6 (unsigned INTN sys, unsigned INTN arg1, unsigned INTN arg2,
	       unsigned INTN arg3, unsigned INTN arg4, unsigned INTN arg5,
	       unsigned INTN arg6);

#endif
