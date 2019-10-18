int
putchar (int c)
{
  return hal_putchar (c);
}

int hal_main_ap (void)
{
  return 0;
}

int main ()
{
  printf ("Hello");
}

int exit()
{
  long t;
  for (;;) {
    long l = pgalloc_low();
    if (l < 0)
      break;
    printf("%lx ", l);
  }

  pgfree(0x170);

  for (;;) {
    long l = pgalloc_low();
    if (l < 0)
      break;
    printf("%lx ", l);
  }

  
  printf("Exit");
  while (1);
}
