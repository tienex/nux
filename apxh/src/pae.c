/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  See COPYING file for the full license.

  SPDX-License-Identifier:	GPL2.0+
*/

#include "project.h"
#include "x86.h"

bool
cpu_is_intel ()
{
  uint32_t eax, ebx, ecx, edx;

  eax = 0;
  ecx = 0;
  cpuid (&eax, &ebx, &ecx, &edx);

  // GenuineIntel?
  if (ebx == 0x756e6547 && ecx == 0x6c65746e && edx == 0x49656e69)
    return true;
  else
    return false;

  return 1;
}

unsigned
intel_cpu_family (void)
{
  unsigned family;
  uint32_t eax, ebx, ecx, edx;

  eax = 1;
  ecx = 0;
  cpuid (&eax, &ebx, &ecx, &edx);

  family = (eax & 0xf00) >> 8;
  family |= (eax & 0xf00000 >> 20);

  return family;
}

unsigned
intel_cpu_model (void)
{
  unsigned model;
  uint32_t eax, ebx, ecx, edx;

  eax = 1;
  ecx = 0;
  cpuid (&eax, &ebx, &ecx, &edx);

  model = (eax & 0xf0) >> 4;
  model |= (eax & 0xf0000) >> 16;

  return model;
}

bool
cpu_supports_pae (void)
{
  uint32_t eax, ebx, ecx, edx;

  eax = 1;
  ecx = 0;
  cpuid (&eax, &ebx, &ecx, &edx);

  return !!(edx & (1 << 6));
}

bool
cpu_supports_longmode (void)
{
  uint32_t eax, ebx, ecx, edx;

  eax = 0x80000001;
  ecx = 0;
  cpuid (&eax, &ebx, &ecx, &edx);

  return !!(edx & (1 << 29));
}

bool
cpu_supports_1gbpages (void)
{
  uint32_t eax, ebx, ecx, edx;

  eax = 0x80000001;
  ecx = 0;
  cpuid (&eax, &ebx, &ecx, &edx);

  return !!(edx & (1 << 26));
}

bool
cpu_supports_nx (void)
{
  bool nx_supported;
  uint64_t efer;
  uint32_t eax, ebx, ecx, edx;

  /* Intel CPUs might have disabled this in MSR. */
  if (cpu_is_intel ())
    {
      unsigned family = intel_cpu_family ();
      unsigned model = intel_cpu_model ();

      if ((family >= 6) && (family > 6 || model > 0xd))
	{
	  uint64_t misc_enable;

	  misc_enable = rdmsr (MSR_IA32_MISC_ENABLE);
	  if (misc_enable & _MSR_IA32_MISC_ENABLE_XD_DISABLE)
	    {
	      misc_enable &= ~_MSR_IA32_MISC_ENABLE_XD_DISABLE;
	      wrmsr (MSR_IA32_MISC_ENABLE, misc_enable);
	    }
	}
    }
  eax = 0x80000001;
  ecx = 0;
  cpuid (&eax, &ebx, &ecx, &edx);

  nx_supported = !!(edx & (1 << 20));
  if (!nx_supported)
    return false;

  efer = rdmsr (MSR_IA32_EFER);
  wrmsr (MSR_IA32_EFER, efer | _MSR_IA32_EFER_NXE);
  efer = rdmsr (MSR_IA32_EFER);

  return !!(efer & _MSR_IA32_EFER_NXE);
}


typedef uint64_t pte_t;

static bool nx_enabled;

/* 1 Gb direct map in the Payload Page Table. */
#define PAE_DIRECTMAP_START 0
#define PAE_DIRECTMAP_END   (1LL << 30)

#define PTE_P 1LL
#define PTE_W 2LL
#define PTE_U 4LL
#define PTE_PS (1L << 7)
#define PTE_NX (nx_enabled ? 1LL << 63 : 0)

#define L3OFF(_va) (((_va) >> 30) & 0x3)
#define L2OFF(_va) (((_va) >> 21) & 0x1ff)
#define L1OFF(_va) (((_va) >> 12) & 0x1ff)

static pte_t *pae_cr3;
static pte_t *l2s[4];

static void
set_pte (pte_t * ptep, uint64_t pfn, uint64_t flags)
{
  *ptep = (pfn << PAGE_SHIFT) | flags;
}

/* Beware: this returns va, not valid in 32 where va space is 32 and PAE is bigger. */
static void *
pte_getaddr (pte_t * ptep)
{
  pte_t pte = *ptep;

  if (!(pte & PTE_P))
    return NULL;

  return (void *) (uintptr_t) (pte & 0x7ffffffffffff000LL);
}

static uint64_t
pte_getflags (pte_t * ptep)
{
  pte_t pte = *ptep;

  return pte & ~0x7ffffffffffff000LL;
}

static uint64_t
pte_mergeflags (uint64_t fl1, uint64_t fl2)
{
  uint64_t newf;


  //PTE_P always present
  assert (fl1 & fl2 & PTE_P);
  newf = PTE_P;

  //PTE_U is either present on both or absent on both
  if ((fl1 & PTE_U) != (fl2 & PTE_U))
    fatal ("Mixed user/kernel addresses not allowed.");
  newf |= fl1 & PTE_U;

  // Write must be OR'd.
  newf |= (fl1 | fl2) & PTE_W;

  // NX must be AND'd.
  newf |= fl1 & fl2 & PTE_NX;

  printf ("Merging: %llx (+) %llx = %llx\n", fl1, fl2, newf);
  return newf;
}

void
pae_verify (vaddr_t va, size64_t size)
{
  /* Nothing to check. */
}

void
pae_init (void)
{
  int i;

  /* Enable NX */
  assert (cpu_supports_pae ());

  nx_enabled = cpu_supports_nx ();

  /* In PAE is only 64 bytes, but we allocate a full page for it. */
  pae_cr3 = (pte_t *) get_payload_page ();

  /* Set PDPTEs */
  for (i = 0; i < 4; i++)
    {
      uintptr_t l2page = get_payload_page ();

      set_pte (pae_cr3 + i, l2page >> PAGE_SHIFT, PTE_P);
      l2s[i] = (pte_t *) l2page;
    }

  printf ("Using PAE paging (CR3: %08lx, NX: %d).\n", pae_cr3, nx_enabled);
}

static pte_t *
pae_get_l2p (pte_t * cr3, vaddr_t va, int payload)
{
  pte_t *l3p, *l2p;
  unsigned l3off = L3OFF (va);
  unsigned l2off = L2OFF (va);

  l3p = cr3 + l3off;

  l2p = (pte_t *) pte_getaddr (l3p);
  if (l2p == NULL)
    {
      uintptr_t l2page;

      /* Populating L2. */
      l2page = payload ? get_payload_page () : get_page ();

      set_pte (l3p, l2page >> PAGE_SHIFT, PTE_P);
      l2p = (pte_t *) l2page;
    }

  return l2p + l2off;
}

static pte_t *
pae_get_l1p (pte_t * cr3, vaddr_t va, int payload)
{
  pte_t *l2p, *l1p;
  unsigned l1off = L1OFF (va);

  l2p = pae_get_l2p (cr3, va, payload);

  l1p = (pte_t *) pte_getaddr (l2p);
  if (l1p == NULL)
    {
      uintptr_t l1page;

      /* Populating L1. */
      l1page = payload ? get_payload_page () : get_page ();

      set_pte (l2p, l1page >> PAGE_SHIFT, PTE_W | PTE_P);
      l1p = (pte_t *) l1page;
    }

  return l1p + l1off;
}

void
pae_map_page (void *pt, vaddr_t va, uintptr_t pa, int payload, int w, int x)
{
  pte_t *l1p, *cr3;
  uint64_t l1f;
  uintptr_t page;

  cr3 = (pte_t *) pt;

  l1p = pae_get_l1p (cr3, va, payload);
  l1f = (w ? PTE_W : 0) | (x ? 0 : PTE_NX) | PTE_P;

  page = (uintptr_t) pte_getaddr (l1p);
  assert (page == 0);
  page = pa >> PAGE_SHIFT;
  set_pte (l1p, page, l1f);
}

static uintptr_t
pae_populate_page (vaddr_t va, int u, int w, int x)
{
  pte_t *l1p;
  uint64_t l1f;
  uintptr_t page;

  l1p = pae_get_l1p (pae_cr3, va, 1);

  l1f = (u ? PTE_U : 0) | (w ? PTE_W : 0) | (x ? 0 : PTE_NX) | PTE_P;

  page = (uintptr_t) pte_getaddr (l1p);
  if (pte_getaddr (l1p) == NULL)
    {
      page = get_payload_page ();
      set_pte (l1p, page >> PAGE_SHIFT, l1f);
    }
  else
    {
      uint64_t newf;
      uint64_t oldf = pte_getflags (l1p);

      newf = pte_mergeflags (l1f, oldf);
      if (newf != oldf)
	{
	  printf ("Flags changed from %llx to %llx\n", oldf, newf);
	  set_pte (l1p, page >> PAGE_SHIFT, newf);
	}
    }

  return page;
}

uintptr_t
pae_getphys (vaddr_t va)
{
  uintptr_t page;
  pte_t *l2e, *l1, *l1e;
  unsigned l3off = L3OFF (va);
  unsigned l2off = L2OFF (va);
  unsigned l1off = L1OFF (va);

  l2e = l2s[l3off] + l2off;

  l1 = (pte_t *) pte_getaddr (l2e);
  assert (l1 != NULL);

  l1e = l1 + l1off;

  page = (uintptr_t) pte_getaddr (l1e);
  assert (page != 0);

  return page | (va & ~(PAGE_MASK));
}

void
pae_directmap (void *pt, uint64_t pa, vaddr_t va, size64_t size,
	       int payload, int x)
{
  uint64_t papfn = pa >> PAGE_SHIFT;
  unsigned i, n;
  pte_t *pte;

  n = size >> PAGE_SHIFT;

  for (i = 0; i < n; i++)
    {
      pte = pae_get_l1p (pt, va + (i << PAGE_SHIFT), payload);
      set_pte (pte, papfn + i, PTE_P | PTE_W | (x ? 0 : PTE_NX));
    }
}

void
pae_physmap (vaddr_t va, size64_t size, uint64_t pa)
{
  pae_directmap (pae_cr3, pa, va, size, 1, 0);
}

void
pae_ptalloc (vaddr_t va, size64_t size)
{
  unsigned i, n;

  n = size >> PAGE_SHIFT;

  for (i = 0; i < n; i++)
    (void) pae_get_l1p (pae_cr3, va + (i << PAGE_SHIFT), 1);
}

#define PAE_LINEAR_SHIFT (PAGE_SHIFT + 9 + 2)
#define PAE_LINEAR_SIZE (1L << PAE_LINEAR_SHIFT)
#define PAE_LINEAR_ALIGN (PAE_LINEAR_SIZE - 1)

void
pae_linear (vaddr_t va, size64_t size)
{
  int i;
  unsigned l3off = L3OFF (va);
  unsigned l2off = L2OFF (va);

  if (va & PAE_LINEAR_ALIGN)
    {
      printf ("PAE Linear VA %llx not aligned (align mask: %lx).\n", va,
	      PAE_LINEAR_ALIGN);
      exit (-1);
    }

  if (size < PAE_LINEAR_SIZE)
    {
      printf ("PAE Linear size %llx too small.\n", size);
      exit (-1);
    }

  for (i = 0; i < 4; i++)
    {
      pte_t *l2p;

      l2p = l2s[l3off] + l2off + i;
      set_pte (l2p, (uint64_t) (uintptr_t) l2s[i] >> PAGE_SHIFT,
	       PTE_NX | PTE_W | PTE_P);
    }
}

void
pae_populate (vaddr_t va, size64_t size, int u, int w, int x)
{
  ssize64_t len = size;

  while (len > 0)
    {
      pae_populate_page (va, u, w, x);

      len -= PAGE_CEILING (va) - va;
      va += PAGE_CEILING (va) - va;

    }
}

void
pae_entry (vaddr_t entry)
{
  md_entry (ARCH_386, (vaddr_t) (uintptr_t) pae_cr3, entry);
}


/*
  AMD64 PAE support.
*/

#define PAE64_DIRECTMAP_START 0
#define PAE64_DIRECTMAP_END   (1LL << 30)

#define L4OFF64(_va) (((_va) >> 39) & 0x1ff)
#define L3OFF64(_va) (((_va) >> 30) & 0x1ff)
#define L2OFF64(_va) (((_va) >> 21) & 0x1ff)
#define L1OFF64(_va) (((_va) >> 12) & 0x1ff)

static pte_t *pae64_cr3;

static pte_t *
pae64_get_l3p (pte_t * cr3, vaddr_t va, int payload)
{
  pte_t *l4p, *l3p;
  unsigned l4off = L4OFF64 (va);
  unsigned l3off = L3OFF64 (va);

  l4p = cr3 + l4off;

  l3p = (pte_t *) pte_getaddr (l4p);
  if (l3p == NULL)
    {
      uintptr_t l3page;

      /* Populating L3. */
      l3page = payload ? get_payload_page () : get_page ();

      set_pte (l4p, l3page >> PAGE_SHIFT, PTE_W | PTE_P);
      l3p = (pte_t *) l3page;
    }

  return l3p + l3off;
}

static pte_t *
pae64_get_l2p (pte_t * cr3, vaddr_t va, int payload)
{
  pte_t *l3p, *l2p;

  unsigned l2off = L2OFF64 (va);

  l3p = pae64_get_l3p (cr3, va, payload);

  l2p = (pte_t *) pte_getaddr (l3p);
  if (l2p == NULL)
    {
      uintptr_t l2page;

      /* Populating L2. */
      l2page = payload ? get_payload_page () : get_page ();

      set_pte (l3p, l2page >> PAGE_SHIFT, PTE_W | PTE_P);
      l2p = (pte_t *) l2page;
    }

  return l2p + l2off;
}

static pte_t *
pae64_get_l1p (pte_t * cr3, vaddr_t va, int payload)
{
  pte_t *l2p, *l1p;
  unsigned l1off = L1OFF64 (va);

  l2p = pae64_get_l2p (cr3, va, payload);

  l1p = (pte_t *) pte_getaddr (l2p);
  if (l1p == NULL)
    {
      uintptr_t l1page;

      /* Populating L1. */
      l1page = payload ? get_payload_page () : get_page ();

      set_pte (l2p, l1page >> PAGE_SHIFT, PTE_W | PTE_P);
      l1p = (pte_t *) l1page;
    }

  return l1p + l1off;
}

void
pae64_verify (vaddr_t va, size64_t size)
{
  /* Nothing to check. */
}

void
pae64_init (void)
{
  assert (cpu_supports_longmode ());

  nx_enabled = cpu_supports_nx ();

  pae64_cr3 = (pte_t *) get_payload_page ();

  printf ("Using PAE64 paging (CR3: %08lx, NX: %d).\n", pae64_cr3,
	  nx_enabled);
}

void
pae64_map_page (void *pt, vaddr_t va, uintptr_t pa, int payload, int w, int x)
{
  pte_t *l1p, *cr3;
  uint64_t l1f;
  uintptr_t page;

  cr3 = (pte_t *) pt;

  printf ("Mapping at va %llx PA %lx (p:%d, w:%d, x:%d)\n", va, pa, payload,
	  w, x);

  l1p = pae64_get_l1p (cr3, va, payload);
  l1f = (w ? PTE_W : 0) | (x ? 0 : PTE_NX) | PTE_P;

  page = (uintptr_t) pte_getaddr (l1p);
  assert (page == 0);
  page = pa >> PAGE_SHIFT;
  set_pte (l1p, page, l1f);
}

static uintptr_t
pae64_populate_page (pte_t * cr3, vaddr_t va, int u, int w, int x,
		     int payload)
{
  pte_t *l1p;
  uint64_t l1f;
  uintptr_t page;

  l1p = pae64_get_l1p (cr3, va, payload);
  l1f = (u ? PTE_U : 0) | (w ? PTE_W : 0) | (x ? 0 : PTE_NX) | PTE_P;

  page = (uintptr_t) pte_getaddr (l1p);
  if (page == 0)
    {
      page = payload ? get_payload_page () : get_page ();
      set_pte (l1p, page >> PAGE_SHIFT, l1f);
    }
  else
    {
      uint64_t newf;
      uint64_t oldf = pte_getflags (l1p);

      newf = pte_mergeflags (l1f, oldf);
      if (newf != oldf)
	{
	  printf ("Flags changed from %llx to %llx\n", oldf, newf);
	  set_pte (l1p, page >> PAGE_SHIFT, newf);
	}
    }

  return page;
}

uintptr_t
pae64_getphys (vaddr_t va)
{
  uintptr_t page;
  pte_t *l4p, *l3p, *l2p, *l1p;
  unsigned l4off = L4OFF64 (va);
  unsigned l3off = L3OFF64 (va);
  unsigned l2off = L2OFF64 (va);
  unsigned l1off = L1OFF64 (va);

  l4p = pae64_cr3 + l4off;

  l3p = (pte_t *) pte_getaddr (l4p);
  assert (l3p != NULL);
  l3p += l3off;

  l2p = (pte_t *) pte_getaddr (l3p);
  assert (l2p != NULL);
  l2p += l2off;

  l1p = (pte_t *) pte_getaddr (l2p);
  assert (l1p != NULL);
  l1p += l1off;

  page = (uintptr_t) pte_getaddr (l1p);
  assert (page != 0);

  return page |= (va & ~(PAGE_MASK));
}

void
pae64_directmap (void *pt, uint64_t pabase, vaddr_t va, size64_t size,
		 int payload, int x)
{
  ssize64_t len;
  uint64_t pa;
  pte_t *cr3 = (pte_t *) pt;
  int p1g = cpu_supports_1gbpages ();
  unsigned long l3cnt = 0, l2cnt = 0, l1cnt = 0;

#define GB1ALIGNED(_a) (((_a) & ((1L << 30) - 1)) == 0)
#define MB2ALIGNED(_a) (((_a) & ((1L << 21) - 1)) == 0)

  /*
     Not the fastest way to do this, but good enough for startup.

     Please note: it is a very bad idea to have a huge physmap
     given the costs in pagetables.
   */


  /* Signed to unsigned: no one will ask us a 1<<64 bytes physmap. */
  len = (ssize64_t) size;
  pa = pabase;

  while (len > 0)
    {

      if (p1g && GB1ALIGNED (pa) && GB1ALIGNED (va) && len >= (1L << 30))
	{
	  pte_t *l3p = pae64_get_l3p (cr3, va, payload);

	  set_pte (l3p, pa >> PAGE_SHIFT,
		   PTE_PS | PTE_W | PTE_P | (x ? 0 : PTE_NX));
	  va += (1L << 30);
	  pa += (1L << 30);
	  len -= (1L << 30);
	  l3cnt++;
	}
      else if (MB2ALIGNED (pa) && MB2ALIGNED (va) && len >= (1 << 21))
	{
	  pte_t *l2p = pae64_get_l2p (cr3, va, payload);

	  set_pte (l2p, pa >> PAGE_SHIFT,
		   PTE_PS | PTE_W | PTE_P | (x ? 0 : PTE_NX));
	  va += (1L << 21);
	  pa += (1L << 21);
	  len -= (1L << 21);
	  l2cnt++;
	}
      else
	{
	  pte_t *l1p = pae64_get_l1p (cr3, va, payload);

	  set_pte (l1p, pa >> PAGE_SHIFT, PTE_W | PTE_P | (x ? 0 : PTE_NX));
	  va += (1L << PAGE_SHIFT);
	  pa += (1L << PAGE_SHIFT);
	  len -= (1L << PAGE_SHIFT);
	  l1cnt++;
	}
    }
}

void
pae64_physmap (vaddr_t va, size64_t size, uint64_t pa)
{
  pae64_directmap (pae64_cr3, pa, va, size, 1, 0);
}

void
pae64_ptalloc (vaddr_t va, size64_t size)
{
  unsigned i, n;

  n = size >> PAGE_SHIFT;

  for (i = 0; i < n; i++)
    (void) pae64_get_l1p (pae64_cr3, va + (i << PAGE_SHIFT), 1);
}

#define PAE64_LINEAR_SHIFT (PAGE_SHIFT + 9 + 9 + 9)
#define PAE64_LINEAR_SIZE  (1LL << PAE64_LINEAR_SHIFT)
#define PAE64_LINEAR_ALIGN (PAE64_LINEAR_SIZE - 1)

void
pae64_linear (vaddr_t va, size64_t size)
{
  unsigned l4off = L4OFF64 (va);
  pte_t *l4p;

  if (va & PAE64_LINEAR_ALIGN)
    {
      printf ("PAE Linear VA %llx not aligned (align mask: %llx).\n",
	      va, PAE64_LINEAR_ALIGN);
      exit (-1);
    }

  if (size < PAE64_LINEAR_SIZE)
    {
      printf ("PAE Linear size %llx too small.\n", size);
      exit (-1);
    }

  l4p = pae64_cr3 + l4off;
  set_pte (l4p, (uintptr_t) pae64_cr3 >> PAGE_SHIFT, PTE_W | PTE_P);
  printf ("Wrote %llx at %p\n", *l4p, l4p);
}

void
pae64_populate (vaddr_t va, size64_t size, int u, int w, int x)
{
  ssize64_t len = size;

  while (len > 0)
    {
      pae64_populate_page (pae64_cr3, va, u, w, x, 1);

      len -= PAGE_CEILING (va) - va;
      va += PAGE_CEILING (va) - va;

    }
}

void
pae64_entry (vaddr_t entry)
{
  md_entry (ARCH_AMD64, (vaddr_t) (uintptr_t) pae64_cr3, entry);
}
