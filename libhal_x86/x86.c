
#include <cdefs.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <nux/hal.h>

#include "x86.h"
#include "stree.h"


extern int _info_start;

extern int _physmap_start;
extern int _physmap_end;

extern int _kva_start;
extern int _kva_end;

extern int _stree_start;
extern int _stree_end;

#define APXH_BOOTINFO_MAGIC 0xAF10B007
struct apxh_bootinfo
{
  uint64_t magic;
  uint64_t maxpfn;
} __attribute__ ((__packed__));

#define APXH_STREE_MAGIC 0xAF1057EE
#define APXH_STREE_VERSION 0
struct apxh_stree
{
  uint64_t magic;
  uint8_t version;
  uint8_t order;
  uint16_t offset;
  uint32_t size;
} __attribute__ ((__packed__));

const struct apxh_bootinfo *bootinfo = (struct apxh_bootinfo *)&_info_start;

void *hal_stree_ptr;
unsigned hal_stree_order;

static inline __dead void
__halt (void)
{
  while (1)
    asm volatile ("cli; hlt");
}

int
inb (int port)
{
  int ret;

  asm volatile ("xor %%eax, %%eax; inb %%dx, %%al":"=a" (ret):"d" (port));
  return ret;
}

int
inw (unsigned port)
{
  int ret;

  asm volatile ("xor %%eax, %%eax; inw %%dx, %%ax":"=a" (ret):"d" (port));
  return ret;
}

int
inl (unsigned port)
{
  int ret;

  asm volatile ("inl %%dx, %%eax":"=a" (ret):"d" (port));
  return ret;
}

void
outb (int port, int val)
{
  asm volatile ("outb %%al, %%dx"::"d" (port), "a" (val));
}

void
outw (unsigned port, int val)
{
  asm volatile ("outw %%ax, %%dx"::"d" (port), "a" (val));
}

void
outl (unsigned port, int val)
{
  asm volatile ("outl %%eax, %%dx"::"d" (port), "a" (val));
}

void
tlbflush_global (void)
{
  asm volatile ("mov %%cr4, %%eax\n"
		"and $0xffffff7f, %%eax\n"
		"mov %%eax, %%cr4\n"
		"or  $0x00000080, %%eax\n" "mov %%eax, %%cr4\n":::"eax");
}

void
tlbflush_local (void)
{
  asm volatile ("mov %%cr3, %%eax; mov %%eax, %%cr3\n":::"eax");
}


int
hal_putchar (int c)
{
  vga_putchar (c);
  return c;
}

unsigned long
hal_cpu_in (uint8_t size, uint32_t port)
{
  unsigned long val;

  switch (size)
    {
    case 1:
      val = inb (port);
      break;
    case 2:
      val = inw (port);
      break;
    case 4:
      val = inl (port);
      break;
    default:
      //      halwarn ("Invalid I/O port size %d", size);
      val = (unsigned long) -1;
    }

  return val;
}

void
hal_cpu_out (uint8_t size, uint32_t port, unsigned long val)
{
  switch (size)
    {
    case 1:
      outb (port, val);
      break;
    case 2:
      outw (port, val);
      break;
    case 4:
      outl (port, val);
      break;
    default:
      //      halwarn ("Invalid I/O port size %d", size);
      break;
    }
}

void
hal_cpu_relax (void)
{
  asm volatile ("pause");
}

void
hal_cpu_trap (void)
{
  asm volatile ("ud2");
}

void __dead
hal_cpu_idle (void)
{
  while (1)
    {
      asm volatile ("sti; hlt");
    }
}

__dead void
hal_cpu_halt (void)
{
  __halt();
}

void
hal_tlbop (hal_tlbop_t tlbop)
{
  if (tlbop == HAL_TLBOP_NONE)
    return;

  if (tlbop == HAL_TLBOP_FLUSHALL)
    tlbflush_global ();
  else
    tlbflush_local ();
}

uint64_t
hal_physmem_dmapbase (void)
{
  return (const uint64_t) 0;
}

const size_t
hal_physmem_dmapsize (void)
{
  return (size_t)((void *)&_physmap_end - (void *)&_physmap_start);
}

unsigned long
hal_physmem_maxpfn (void)
{
  return (unsigned long)bootinfo->maxpfn;
}

void *
hal_physmem_stree (unsigned *order)
{
  if (order)
    *order = hal_stree_order;
  return hal_stree_ptr;
}

void *
hal_virtmem_kvabase (void)
{
  return (void *)&_kva_start;
}

const size_t
hal_virtmem_kvasize (void)
{
  return (size_t)((void *)&_kva_end - (void *)&_kva_start);
}

static void
early_print (const char *str)
{
  size_t i;
  size_t len = strlen (str);
  for (i = 0; i < len; i++)
    hal_putchar (str[i]);
}

int
x86_init (void)
{
  size_t stree_memsize;
  struct apxh_stree *stree_hdr;

  if (bootinfo->magic != APXH_BOOTINFO_MAGIC)
    {
      early_print("ERROR: Unrecognised bootinfo magic!");
      hal_cpu_halt ();
    }

  /* Check  APXH stree. */
  stree_hdr = (struct apxh_stree *)&_stree_start;
  if (stree_hdr->magic != APXH_STREE_MAGIC)
    {
      early_print("ERROR: Unrecognised stree magic!");
      hal_cpu_halt();
    }
  if (stree_hdr->size != 8 * STREE_SIZE(stree_hdr->order))
    {
      early_print("ERROR: stree size doesn't match!");
      hal_cpu_halt();
    }
  stree_memsize = (size_t)((void *)&_stree_end - (void *)&_stree_start);
  if (stree_hdr->size + stree_hdr->offset > stree_memsize)
    {
      early_print("ERROR: stree doesn't fit in allocated memory!");
      hal_cpu_halt();
    }
  hal_stree_order = stree_hdr->order;
  hal_stree_ptr = (uint8_t *)stree_hdr + stree_hdr->offset;

  /* Reserve page 0. It's special in X86. */
  stree_clrbit(hal_stree_ptr, hal_stree_order, 0);

#ifdef __i386__
  early_print("i386 HAL booting from APXH.\n");
#endif
#ifdef __amd64__
  early_print("AMD64 HAL booting from APXH.\n");
#endif

  return 0;
}
