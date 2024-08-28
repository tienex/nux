#include <stdio.h>

#if 0
int main()
{
  printf("Hello!\n");
  return 0;
}
#endif

void putchar(int ch)
{
  asm ("mv a0, %0;"
       "li a7, 1;"
       "ecall;"
       :: "r"(ch)
       : "a0","a7");
}

void exit(int status)
{
  printf("Exit %d\n", exit);
  /* Should definitely tell SBI about this. */
  while (1);
}
