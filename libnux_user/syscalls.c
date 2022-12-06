
#if NUX_MACHINE==i386
#include <i386/syscalls.h>
#else
#error "Unknown NUX architecture"
#endif

int
syscall0 (unsigned sys)
{
  int ret;

  __syscall0 (sys, ret);
  return ret;
}

int
syscall1 (unsigned sys, unsigned long arg1)
{
  int ret;

  __syscall1 (sys, arg1, ret);
  return ret;
}

int
syscall2 (unsigned sys, unsigned long arg1, unsigned long arg2)
{
  int ret;

  __syscall2 (sys, arg1, arg2, ret);
  return ret;
}

int
syscall3 (unsigned sys, unsigned long arg1, unsigned long arg2,
	  unsigned long arg3)
{
  int ret;

  __syscall3 (sys, arg1, arg2, arg3, ret);
  return ret;
}

int
syscall4 (unsigned sys, unsigned long arg1, unsigned long arg2,
	  unsigned long arg3, unsigned long arg4)
{
  int ret;

  __syscall4 (sys, arg1, arg2, arg3, arg4, ret);
  return ret;
}

int
syscall5 (unsigned sys, unsigned long arg1, unsigned long arg2,
	  unsigned long arg3, unsigned long arg4, unsigned long arg5)
{
  int ret;

  __syscall5 (sys, arg1, arg2, arg3, arg4, arg5, ret);
  return ret;
}
