#include <stdio.h>
#include <nux/nux.h>
#include <nux/hal.h>

struct hal_frame *hal_entry_pf (struct hal_frame *f, unsigned long va,
				hal_pfinfo_t info)
{
  printf ("Pagefault at %lx\n", va);
  hal_frame_print (f);
  hal_cpu_halt ();
}

struct hal_frame *hal_entry_xcpt (struct hal_frame *f, unsigned xcpt)
{
  printf ("Exception %lx\n", xcpt);
  hal_frame_print (f);
  hal_cpu_halt ();
}

struct hal_frame *hal_entry_nmi (struct hal_frame *f)
{
  printf ("Hm. NMI received!\n");
  hal_cpu_halt ();
}

struct hal_frame *hal_entry_vect (struct hal_frame *f, unsigned irq)
{
  printf ("Hm. IRQ %d received\n");
  hal_cpu_halt ();
}

struct hal_frame *hal_entry_syscall (struct hal_frame *f,
				     unsigned long a0, unsigned long a1,
				     unsigned long a2, unsigned long a3,
				     unsigned long a4, unsigned long a5)
{
  printf ("Oh! Syscalls!\n");
  hal_cpu_halt ();
}
