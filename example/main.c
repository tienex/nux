#include <stdio.h>
#include <nux/nux.h>

int
putchar (int c)
{
  return hal_putchar (c);
}

void hal_main_ap (void)
{

}

int main ()
{
  printf ("Hello");

  extern int _linear_start;
  uintptr_t linaddr = (uintptr_t)&_linear_start;

  linaddr = linaddr + (0xc0100000 >> 9);
  uint64_t *pte = (uint64_t *)linaddr;
  
  printf("pte linaddr= %lx\n", linaddr);

  int i;
  for (i = 0; i < 512; i++) {
    printf("%llx", pte[i]);
  }
}

int exit()
{
  printf("Exit");
  while (1);
}
