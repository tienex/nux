#ifndef __NUX_SYSCALLS_H
#define __NUX_SYSCALLS_H

long syscall0 (unsigned sys);
long syscall1 (unsigned sys, unsigned long arg1);
long syscall2 (unsigned sys, unsigned long arg1, unsigned long arg2);
long syscall3 (unsigned sys, unsigned long arg1, unsigned long arg2,
	       unsigned long arg3);
long syscall4 (unsigned sys, unsigned long arg1, unsigned long arg2,
	       unsigned long arg3, unsigned long arg4);
long syscall5 (unsigned sys, unsigned long arg1, unsigned long arg2,
	       unsigned long arg3, unsigned long arg4, unsigned long arg5);
long syscall6 (unsigned sys, unsigned long arg1, unsigned long arg2,
	       unsigned long arg3, unsigned long arg4, unsigned long arg5,
	       unsigned long arg6);

#endif
