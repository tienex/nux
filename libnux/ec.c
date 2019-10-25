#include <stdio.h>
#include <stdlib.h>
#include <nux/hal.h>

int
putchar (int c)
{
  return hal_putchar (c);
}

void __dead
exit (int status)
{
  hal_cpu_halt ();
}
