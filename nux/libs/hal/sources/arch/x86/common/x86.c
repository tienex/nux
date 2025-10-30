/** @file
  x86 Hardware Abstraction Layer Implementation

  Core x86/AMD64 HAL initialization, CPU operations, I/O port access,
  memory management, and hardware interface functions.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <cdefs.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <framebuffer.h>

#include <hal/hal.h>
#include <apxh/apxh.h>
#include <nux/symbol.h>

#include <hal/internal.h>
#include <nux/batree.h>


extern INT32 _info_start;

extern INT32 _physmap_start;
extern INT32 _physmap_end;

extern INT32 _pfncache_start;
extern INT32 _pfncache_end;

extern INT32 _kva_start;
extern INT32 _kva_end;

extern INT32 _kmem_start;
extern INT32 _kmem_end;

extern INT32 _stree_start[];
extern INT32 _stree_end[];

extern INT32 _fbuf_start;
extern INT32 _fbuf_end;

extern INT32 _memregs_start;
extern INT32 _memregs_end;

//
// Pin areas of memory to a fixed memory region type.
//
struct apxh_region _memregs_pinned[] = {
  // Remove me after getting ACPI pointer from kernel.
  {.type = APXH_REGION_MMIO,.pfn = 0,.len = 1,},
  // Mark the whole 0xA0000-0x100000 area as MMIO.
  {.type = APXH_REGION_MMIO,.pfn = 0xa0,.len = 96,},
};

#define PINNED_MEMREGS (sizeof(_memregs_pinned)/sizeof(struct apxh_region))

CONST struct apxh_bootinfo *bootinfo = (struct apxh_bootinfo *) &_info_start;

FRAMEBUFFER_DESC fbdesc;
struct apxh_platformdesc pltdesc;

void *gHalStreePtr;
UINT32 gHalStreeOrder;

int gUseFb;
INT32 gNuxInitialized = 0;

/**
  Halt the CPU indefinitely.

  Disables interrupts and halts the processor in an infinite loop.
**/
static INLINE __dead VOID
Halt (
  VOID
  )
{
  while (1)
    asm volatile ("cli; hlt");
}

/**
  Read a Model-Specific Register.

  @param[in] Ecx  MSR index to read.

  @return 64-bit MSR value.
**/
UINT64
ReadMsr (
  IN UINT32  Ecx
  )
{
  UINT32 Edx, Eax;

  asm volatile ("rdmsr\n":"=d" (Edx), "=a" (Eax):"c" (Ecx));

  return ((UINT64) Edx << 32) | Eax;
}

/**
  Write a Model-Specific Register.

  @param[in] Ecx  MSR index to write.
  @param[in] Val  64-bit value to write.
**/
VOID
WriteMsr (
  IN UINT32  Ecx,
  IN UINT64  Val
  )
{
  UINT32 Edx, Eax;

  Eax = (UINT32) Val;
  Edx = (UINT32) (Val >> 32);

  asm volatile ("wrmsr\n"::"a" (Eax), "d" (Edx), "c" (Ecx));
}

/**
  Read CR4 control register.

  @return CR4 value.
**/
UINTN
ReadCr4 (
  VOID
  )
{
  UINTN r;

  asm volatile ("mov %%cr4, %0\n":"=r" (r));

  return r;
}

/**
  Write CR4 control register.

  @param[in] r  Value to write to CR4.
**/
VOID
WriteCr4 (
  IN UINTN  r
  )
{
  asm volatile ("mov %0, %%cr4\n"::"r" (r));
}

/**
  Read CR3 control register.

  @return CR3 value (page table base).
**/
UINTN
ReadCr3 (
  VOID
  )
{
  UINTN r;

  asm volatile ("mov %%cr3, %0\n":"=r" (r));

  return r;
}

/**
  Write CR3 control register.

  @param[in] r  Value to write to CR3 (page table base).
**/
VOID
WriteCr3 (
  IN UINTN  r
  )
{
  asm volatile ("mov %0, %%cr3\n"::"r" (r));
}

/**
  Read a byte from an I/O port.

  @param[in] Port  I/O port address.

  @return Byte value read from the port.
**/
INT32
InB (
  IN INT32  Port
  )
{
  INT32 ret;

  asm volatile ("xor %%eax, %%eax; inb %%dx, %%al":"=a" (ret):"d" (Port));
  return ret;
}

/**
  Read a word from an I/O port.

  @param[in] Port  I/O port address.

  @return Word value read from the port.
**/
INT32
InW (
  IN UINT32  Port
  )
{
  INT32 ret;

  asm volatile ("xor %%eax, %%eax; inw %%dx, %%ax":"=a" (ret):"d" (Port));
  return ret;
}

/**
  Read a dword from an I/O port.

  @param[in] Port  I/O port address.

  @return Dword value read from the port.
**/
INT32
InL (
  IN UINT32  Port
  )
{
  INT32 ret;

  asm volatile ("inl %%dx, %%eax":"=a" (ret):"d" (Port));
  return ret;
}

/**
  Write a byte to an I/O port.

  @param[in] Port  I/O port address.
  @param[in] Val   Byte value to write.
**/
VOID
OutB (
  IN INT32  Port,
  IN INT32  Val
  )
{
  asm volatile ("outb %%al, %%dx"::"d" (Port), "a" (Val));
}

/**
  Write a word to an I/O port.

  @param[in] Port  I/O port address.
  @param[in] Val   Word value to write.
**/
VOID
OutW (
  IN UINT32  Port,
  IN INT32   Val
  )
{
  asm volatile ("outw %%ax, %%dx"::"d" (Port), "a" (Val));
}

/**
  Write a dword to an I/O port.

  @param[in] Port  I/O port address.
  @param[in] Val   Dword value to write.
**/
VOID
OutL (
  IN UINT32  Port,
  IN INT32   Val
  )
{
  asm volatile ("outl %%eax, %%dx"::"d" (Port), "a" (Val));
}

/**
  Flush the global TLB.

  Uses the CR4.PGE toggle method to flush all TLB entries.
**/
VOID
TlbFlushGlobal (
  VOID
  )
{
  UINTN r;

  r = ReadCr4 ();
  WriteCr4 (r ^ (1 << 7));
  WriteCr4 (r);
}

/**
  Flush the local TLB.

  Reloads CR3 to flush non-global TLB entries.
**/
VOID
TlbFlushLocal (
  VOID
  )
{
  UINTN r;

  r = ReadCr3 ();
  WriteCr3 (r);

  asm volatile ("":::"memory");
}


INT32
hal_putchar (
  IN INT32  c
  )
{

  if (gUseFb)
    framebuffer_putc (c, 0xe0e0e0);
  else
    vga_putchar (c);

  /* Always use serial. */
  SerialPutChar (c);

  return c;
}

UINTN
hal_cpu_in (
  IN UINT8   size,
  IN UINT32  port
  )
{
  UINTN val;

  switch (size)
    {
    case 1:
      val = InB (port);
      break;
    case 2:
      val = InW (port);
      break;
    case 4:
      val = InL (port);
      break;
    default:
      //      halwarn ("Invalid I/O port size %d", size);
      val = (UINTN) -1;
    }

  return val;
}

VOID
hal_cpu_out (
  IN UINT8   size,
  IN UINT32  port,
  IN UINTN   val
  )
{
  switch (size)
    {
    case 1:
      OutB (port, val);
      break;
    case 2:
      OutW (port, val);
      break;
    case 4:
      OutL (port, val);
      break;
    default:
      //      halwarn ("Invalid I/O port size %d", size);
      break;
    }
}

VOID
hal_cpu_relax (
  VOID
  )
{
  asm volatile ("pause");
}

VOID
hal_cpu_trap (
  VOID
  )
{
  asm volatile ("ud2");
}

UINT64
hal_cpu_cycles (
  VOID
  )
{
  UINT32 hi, lo;
  asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));
  return ((UINT64)hi << 32) | lo;
}

__dead VOID
hal_cpu_idle (
  VOID
  )
{
  while (1)
    {
      asm volatile ("sti; hlt");
    }
}

__dead VOID
hal_cpu_halt (
  VOID
  )
{
  Halt ();
}

VOID
hal_cpu_tlbop (
  IN hal_tlbop_t  tlbop
  )
{
  if (tlbop == HAL_TLBOP_NONE)
    return;

  if (tlbop == HAL_TLBOP_FLUSHALL)
    TlbFlushGlobal ();
  else
    TlbFlushLocal ();
}

VOID
hal_useraccess_start (
  VOID
  )
{
  /* TODO: SMEP */
}

VOID
hal_useraccess_end (
  VOID
  )
{
  /* TODO: SMEP */
}

VIRTUAL_ADDRESS
hal_virtmem_dmapbase (
  VOID
  )
{
  return (UINT64) (UINTN) & _physmap_start;
}

CONST UINTN
hal_virtmem_dmapsize (
  VOID
  )
{
  return (UINTN) ((void *) &_physmap_end - (void *) &_physmap_start);
}

VIRTUAL_ADDRESS
hal_virtmem_pfn$base (
  VOID
  )
{
  return (UINT64) (UINTN) & _pfncache_start;
}

CONST UINTN
hal_virtmem_pfn$size (
  VOID
  )
{
  return (UINTN) ((void *) &_pfncache_end - (void *) &_pfncache_start);
}

CONST VIRTUAL_ADDRESS
hal_virtmem_userbase (
  VOID
  )
{
  return pt_umap_minaddr ();
}

CONST UINTN
hal_virtmem_usersize (
  VOID
  )
{
  return pt_umap_maxaddr ();
}

CONST VIRTUAL_ADDRESS
hal_virtmem_userentry (
  VOID
  )
{
  return (CONST VIRTUAL_ADDRESS) bootinfo->uentry;
}

UINTN
hal_physmem_maxpfn (
  VOID
  )
{
  return (UINTN) bootinfo->maxpfn;
}

UINTN
hal_physmem_maxrampfn (
  VOID
  )
{
  return (UINTN) bootinfo->maxrampfn;
}

UINT32
hal_physmem_numregions (
  VOID
  )
{

  return (UINT32) bootinfo->numregions + PINNED_MEMREGS;
}

struct apxh_region *
hal_physmem_region (
  IN UINT32  i
  )
{
  struct apxh_region *ptr;

  if (i >= hal_physmem_numregions ())
    return NULL;

  if (i < (UINT32) bootinfo->numregions)
    {
      ptr = (struct apxh_region *) &_memregs_start + i;
      assert (ptr < (struct apxh_region *) &_memregs_end);
    }
  else
    {
      ptr = _memregs_pinned + i - bootinfo->numregions;
    }

  return ptr;
}

VOID *
hal_physmem_stree (
  OUT UINT32  *Order OPTIONAL
  )
{
  if (Order)
    *Order = gHalStreeOrder;
  return gHalStreePtr;
}

VIRTUAL_ADDRESS
hal_virtmem_kvabase (
  VOID
  )
{
  return (VIRTUAL_ADDRESS) & _kva_start;
}

CONST UINTN
hal_virtmem_kvasize (
  VOID
  )
{
  return (UINTN) ((void *) &_kva_end - (void *) &_kva_start);
}

VIRTUAL_ADDRESS
hal_virtmem_kmembase (
  VOID
  )
{
  return (VIRTUAL_ADDRESS) & _kmem_start;
}

CONST UINTN
hal_virtmem_kmemsize (
  VOID
  )
{
  return (UINTN) ((void *) &_kmem_end - (void *) &_kmem_start);
}

/**
  Print a string during early boot.

  @param[in] Str  String to print.
**/
static VOID
EarlyPrint (
  IN CONST CHAR8  *Str
  )
{
  UINTN i;
  UINTN len = strlen (Str);
  for (i = 0; i < len; i++)
    hal_putchar (Str[i]);
}

CONST struct apxh_platformdesc *
hal_pltinfo (
  VOID
  )
{
  return &pltdesc;
}

/**
  Initialize x86 hardware abstraction layer.

  Validates boot info, initializes serial port, framebuffer,
  physical memory tree, page tables, and architecture-specific components.
**/
VOID
X86Initialize (
  VOID
  )
{
  UINTN StreeMemsize;
  struct apxh_stree *StreeHdr;

  if (bootinfo->magic != APXH_BOOTINFO_MAGIC)
    {
      /* Only way to let know that things are wrong. */
      hal_cpu_trap ();
    }

  SerialInitialize ();

  fbdesc = bootinfo->fbdesc;
  fbdesc.addr = (UINT64) (UINTN) & _fbuf_start;
  gUseFb = framebuffer_init (&fbdesc);

  /* Check  APXH stree. */
  StreeHdr = (struct apxh_stree *) _stree_start;
  if (StreeHdr->magic != APXH_STREE_MAGIC)
    {
      EarlyPrint ("ERROR: Unrecognised stree magic!");
      hal_cpu_halt ();
    }
  if (StreeHdr->size != 8 * STREE_SIZE (StreeHdr->order))
    {
      EarlyPrint ("ERROR: stree size doesn't match!");
      hal_cpu_halt ();
    }
  StreeMemsize = (UINTN) ((void *) _stree_end - (void *) _stree_start);
  if (StreeHdr->size + StreeHdr->offset > StreeMemsize)
    {
      EarlyPrint ("ERROR: stree doesn't fit in allocated memory!");
      hal_cpu_halt ();
    }
  gHalStreeOrder = StreeHdr->order;
  gHalStreePtr = (UINT8 *) StreeHdr + StreeHdr->offset;

  /* Do not allow allocation in non-RAM pinned memory regions. */
  for (INT32 i = 0; i < PINNED_MEMREGS; i++)
    {
      struct apxh_region *r = _memregs_pinned + i;
      if (r->type != APXH_REGION_RAM)
	for (INT32 j = 0; j < r->len; j++)
	  stree_clrbit (gHalStreePtr, gHalStreeOrder, r->pfn + j);
    }

  pltdesc = bootinfo->pltdesc;

  pmap_init ();

#ifdef __i386__
  EarlyPrint ("i386 HAL booting from APXH.\n");
#endif
#ifdef __amd64__
  EarlyPrint ("AMD64 HAL booting from APXH.\n");
  amd64_init ();
#endif
}

VOID
hal_init_done (
  VOID
  )
{
#ifdef __i386__
  i386_init_done ();
#endif
#ifdef __amd64__
  amd64_init_done ();
#endif
  gNuxInitialized = 1;
}

/**
  Stack Frame Structure

  Represents a call stack frame for stack unwinding and tracing.
  Contains frame pointer and return address.
**/
typedef struct _STACKFRAME
{
  struct _STACKFRAME  *Rbp;   ///< Frame pointer (base pointer)
  UINTN               Ra;     ///< Return address
} STACKFRAME, *PSTACKFRAME, *PCSTACKFRAME;

/** Legacy type alias for compatibility **/
#define stackframe STACKFRAME

/**
  Print stack trace.

  @param[in] Rbp  Base pointer to start trace from.
**/
VOID
StackFrame (
  IN UINTN  Rbp
  )
{
  STACKFRAME *Sf = (STACKFRAME *)Rbp;
  UINT32 i = 1;

  while (sf != NULL && i < 32 && ((UINTN)sf % (sizeof(VOID *)) == 0))
    {
      printf ("    [%d]: %lx <%s>\n", i, Sf->Ra, NuxSymbolResolve(Sf->Ra));

      if (Sf->Rbp <= sf)
	break;
      Sf = Sf->Rbp;
      i++;
    }
}

__dead VOID
hal_panic (
  IN UINT32            cpu,
  IN CONST CHAR8       *Error,
  IN struct hal_frame  *Frame
  )
{
  if (gUseFb)
    {
      /*
         Reset frame buffer. This will unlock in case any CPU was
         holding the spinlock.
       */
      framebuffer_reset ();
    }

  printf ("\n"
	  "----------------------------------------"
	  "---------------------------------------\n"
	  "Fatal error on CPU%d: %s\n", cpu, Error);
  if (Frame != NULL)
    {
      hal_frame_print (Frame);
    }
  printf ("\n");
  printf ("Stack Trace:\n\n");
  printf ("    [0]: %lx <%s>\n", hal_frame_getip(Frame), NuxSymbolResolve(hal_frame_getip(Frame)));
  StackFrame (frame_bp(Frame));
  printf ("\n");
  printf ("PTE Walk for CR2 [%lx]\n", frame_cr2(Frame));
  printf ("\n");
  pt_umap_debugwalk (NULL, frame_cr2(Frame));
  printf ("\n");
  printf ("----------------------------------------"
	  "---------------------------------------\n");

  Halt ();
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use ReadMsr instead **/
UINT64 rdmsr (UINT32 ecx) {
  return ReadMsr (ecx);
}

/** @deprecated Use WriteMsr instead **/
void wrmsr (UINT32 ecx, UINT64 val) {
  WriteMsr (ecx, val);
}

/** @deprecated Use ReadCr4 instead **/
unsigned long read_cr4 (void) {
  return ReadCr4 ();
}

/** @deprecated Use WriteCr4 instead **/
void write_cr4 (unsigned INTN r) {
  WriteCr4 (r);
}

/** @deprecated Use ReadCr3 instead **/
unsigned long read_cr3 (void) {
  return ReadCr3 ();
}

/** @deprecated Use WriteCr3 instead **/
void write_cr3 (unsigned INTN r) {
  WriteCr3 (r);
}

/** @deprecated Use InB instead **/
int inb (INT32 port) {
  return InB (port);
}

/** @deprecated Use InW instead **/
int inw (UINT32 port) {
  return InW (port);
}

/** @deprecated Use InL instead **/
int inl (UINT32 port) {
  return InL (port);
}

/** @deprecated Use OutB instead **/
void outb (INT32 port, INT32 val) {
  OutB (port, val);
}

/** @deprecated Use OutW instead **/
void outw (UINT32 port, INT32 val) {
  OutW (port, val);
}

/** @deprecated Use OutL instead **/
void outl (UINT32 port, INT32 val) {
  OutL (port, val);
}

/** @deprecated Use TlbFlushGlobal instead **/
void tlbflush_global (void) {
  TlbFlushGlobal ();
}

/** @deprecated Use TlbFlushLocal instead **/
void tlbflush_local (void) {
  TlbFlushLocal ();
}

/** @deprecated Use X86Initialize instead **/
void x86_init (void) {
  X86Initialize ();
}

/** @deprecated Use StackFrame instead **/
void stackframe (unsigned INTN rbp) {
  StackFrame (rbp);
}

// Legacy global variable alias
int nux_initialized = 0;
