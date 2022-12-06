
#include <nux/syscalls.h>

int
putchar (const char c)
{
  (void) syscall1 (4096, c);
}

void
exit (int status)
{
  return syscall0 (0);
}

int
puts (const char *s)
{
  char c;

  while ((c = *s++) != '\0')
    putchar (c);

  return 0;
}

int
main (void)
{
  puts ("Hello from userspace, NUX!\n");

  return 0;
}
