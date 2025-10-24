#ifndef __unux_syscalls_h__
#define __unux_syscalls_h__

long syscall0 (unsigned long sys);
long syscall1 (unsigned long sys, unsigned long arg1);
long syscall2 (unsigned long sys, unsigned long arg1, unsigned long arg2);
long syscall3 (unsigned long sys, unsigned long arg1, unsigned long arg2,
	       unsigned long arg3);
long syscall4 (unsigned long sys, unsigned long arg1, unsigned long arg2,
	       unsigned long arg3, unsigned long arg4);
long syscall5 (unsigned long sys, unsigned long arg1, unsigned long arg2,
	       unsigned long arg3, unsigned long arg4, unsigned long arg5);
long syscall6 (unsigned long sys, unsigned long arg1, unsigned long arg2,
	       unsigned long arg3, unsigned long arg4, unsigned long arg5,
	       unsigned long arg6);

#endif
