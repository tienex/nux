#include <arch_syscalls.h>

long
syscall0 (unsigned long sys)
{
  __SYSCALL0 (sys);
  return sys;
}

long
syscall1 (unsigned long sys, unsigned long arg1)
{

  __SYSCALL1 (sys, arg1);
  return sys;
}

long
syscall2 (unsigned long sys, unsigned long arg1, unsigned long arg2)
{

  __SYSCALL2 (sys, arg1, arg2);
  return sys;
}

long
syscall3 (unsigned long sys, unsigned long arg1, unsigned long arg2,
	  unsigned long arg3)
{

  __SYSCALL3 (sys, arg1, arg2, arg3);
  return sys;
}

long
syscall4 (unsigned long sys, unsigned long arg1, unsigned long arg2,
	  unsigned long arg3, unsigned long arg4)
{

  __SYSCALL4 (sys, arg1, arg2, arg3, arg4);
  return sys;
}

long
syscall5 (unsigned long sys, unsigned long arg1, unsigned long arg2,
	  unsigned long arg3, unsigned long arg4, unsigned long arg5)
{

  __SYSCALL5 (sys, arg1, arg2, arg3, arg4, arg5);
  return sys;
}

long
syscall6 (unsigned long sys, unsigned long arg1, unsigned long arg2,
	  unsigned long arg3, unsigned long arg4, unsigned long arg5,
	  unsigned long arg6)
{
  __SYSCALL6 (sys, arg1, arg2, arg3, arg4, arg5, arg6);
  return sys;
}
