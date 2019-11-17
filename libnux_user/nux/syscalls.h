#ifndef __NUX_SYSCALLS_H
#define __NUX_SYSCALLS_H

int syscall0(unsigned sys);
int syscall1(unsigned sys, unsigned long arg1);
int syscall2(unsigned sys, unsigned long arg1, unsigned long arg2);
int syscall3(unsigned sys, unsigned long arg1, unsigned long arg2,
	     unsigned long arg3);
int syscall4(unsigned sys, unsigned long arg1, unsigned long arg2,
	     unsigned long arg3, unsigned long arg4);
int syscall5(unsigned sys, unsigned long arg1, unsigned long arg2,
	     unsigned long arg3, unsigned long arg4, unsigned long arg5);

#endif
