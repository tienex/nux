/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "project.h"

#define PTE_V (1LL << 0)
#define PTE_R (1LL << 1)
#define PTE_W (1LL << 2)
#define PTE_X (1LL << 3)
#define PTE_U (1LL << 4)

typedef uint64_t pte_t;

static void
set_pte (pte_t * ptep, uint64_t pfn, uint64_t flags)
{
  *ptep = (pfn << 10) | flags;
}

static void *
pte_getaddr (pte_t * ptep)
{
  pte_t pte = *ptep;

  if (!(pte & PTE_V))
    return NULL;

  return (void *) (uintptr_t) ((pte & 0x7ffffffffffffc00LL) << 2);
}

static uint64_t
pte_getflags (pte_t * ptep)
{
  pte_t pte = *ptep;

  return pte & ~0x7ffffffffffffc00LL;
}

static uint64_t
pte_mergeflags (uint64_t fl1, uint64_t fl2)
{
  uint64_t newf;


  //PTE_R and V always present
  assert (fl1 & fl2 & PTE_V);
  assert (fl1 & fl2 & PTE_R);
  newf = PTE_R | PTE_V;

  //PTE_U is either present on both or absent on both
  if ((fl1 & PTE_U) != (fl2 & PTE_U))
    fatal ("Mixed user/kernel addresses not allowed.");
  newf |= fl1 & PTE_U;

  // Write must be OR'd.
  newf |= (fl1 | fl2) & PTE_W;

  // Executable must be OR'd.
  newf |= (fl1 | fl2) & PTE_X;

#if 0
  printf ("Merging: %llx (+) %llx = %llx\n", fl1, fl2, newf);
#endif
  return newf;
}

/*
  RISC-V SV48 Support.
*/

#define SV48_DIRECTMAP_START 0
#define SV48_DIRECTMAP_END (1LL << 30)

#define L4OFF64(_va) (((_va) >> 39) & 0x1ff)
#define L3OFF64(_va) (((_va) >> 30) & 0x1ff)
#define L2OFF64(_va) (((_va) >> 21) & 0x1ff)
#define L1OFF64(_va) (((_va) >> 12) & 0x1ff)

static pte_t *sv48_root;

static pte_t *
sv48_get_l3p (pte_t * root, vaddr_t va, int payload)
{
  pte_t *l4p, *l3p;
  unsigned l4off = L4OFF64 (va);
  unsigned l3off = L3OFF64 (va);

  l4p = root + l4off;

  l3p = (pte_t *) pte_getaddr (l4p);
  if (l3p == NULL)
    {
      uintptr_t l3page;

      /* Populating L3. */
      l3page = payload ? get_payload_page () : get_page ();

      set_pte (l4p, l3page >> PAGE_SHIFT, PTE_V);
      l3p = (pte_t *) l3page;
    }

  return l3p + l3off;
}

static pte_t *
sv48_get_l2p (pte_t * root, vaddr_t va, int payload)
{
  pte_t *l3p, *l2p;

  unsigned l2off = L2OFF64 (va);

  l3p = sv48_get_l3p (root, va, payload);

  l2p = (pte_t *) pte_getaddr (l3p);
  if (l2p == NULL)
    {
      uintptr_t l2page;

      /* Populating L2. */
      l2page = payload ? get_payload_page () : get_page ();

      set_pte (l3p, l2page >> PAGE_SHIFT, PTE_V);
      l2p = (pte_t *) l2page;
    }

  return l2p + l2off;
}

static pte_t *
sv48_get_l1p (pte_t * root, vaddr_t va, int payload)
{
  pte_t *l2p, *l1p;
  unsigned l1off = L1OFF64 (va);

  l2p = sv48_get_l2p (root, va, payload);

  l1p = (pte_t *) pte_getaddr (l2p);
  if (l1p == NULL)
    {
      uintptr_t l1page;

      /* Populating L1. */
      l1page = payload ? get_payload_page () : get_page ();

      set_pte (l2p, l1page >> PAGE_SHIFT, PTE_V);
      l1p = (pte_t *) l1page;
    }

  return l1p + l1off;
}

void
sv48_verify (vaddr_t va, size64_t size)
{
  /* Nothing to check. */
}


void
sv48_init (void)
{

  sv48_root = (pte_t *) get_payload_page ();

  printf ("Using SV48 paging (root: %08lx).\n", sv48_root);
}

void
sv48_map_page (void *pt, vaddr_t va, uintptr_t pa, int payload, int w, int x)
{
  pte_t *l1p, *root;
  uint64_t l1f;
  uintptr_t page;

  root = (pte_t *) pt;

  printf ("Mapping at va %llx PA %lx (p:%d, w:%d, x:%d)\n", va, pa, payload,
	  w, x);

  l1p = sv48_get_l1p (root, va, payload);
  l1f = (w ? PTE_W : 0) | (x ? PTE_X : 0) | PTE_R | PTE_V;

  page = (uintptr_t) pte_getaddr (l1p);
  assert (page == 0);
  page = pa >> PAGE_SHIFT;
  set_pte (l1p, page, l1f);
}

static uintptr_t
sv48_populate_page (pte_t * root, vaddr_t va, int u, int w, int x,
		    int payload)
{
  pte_t *l1p;
  uint64_t l1f;
  uintptr_t page;

  l1p = sv48_get_l1p (root, va, payload);
  l1f = (u ? PTE_U : 0) | (w ? PTE_W : 0) | (x ? PTE_X : 0) | PTE_R | PTE_V;

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
sv48_getphys (vaddr_t va)
{
  uintptr_t page;
  pte_t *l4p, *l3p, *l2p, *l1p;
  unsigned l4off = L4OFF64 (va);
  unsigned l3off = L3OFF64 (va);
  unsigned l2off = L2OFF64 (va);
  unsigned l1off = L1OFF64 (va);

  l4p = sv48_root + l4off;

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
sv48_directmap (void *pt, uint64_t pabase, vaddr_t va, size64_t size,
		enum memory_type mt, int payload, int x)
{
  ssize64_t len;
  uint64_t pa;
  pte_t *root = (pte_t *) pt;
  unsigned long l3cnt = 0, l2cnt = 0, l1cnt = 0;

#define GB1ALIGNED(_a) (((_a) & ((1L << 30) - 1)) == 0)
#define MB2ALIGNED(_a) (((_a) & ((1L << 21) - 1)) == 0)


  /* Signed to unsigned: no one will ask us a 1<<64 bytes physmap. */
  len = (ssize64_t) size;
  pa = pabase;

  while (len > 0)
    {

      if (GB1ALIGNED (pa) && GB1ALIGNED (va) && len >= (1L << 30))
	{
	  pte_t *l3p = sv48_get_l3p (root, va, payload);

	  set_pte (l3p, pa >> PAGE_SHIFT,
		   (x ? PTE_X : 0) | PTE_W | PTE_R | PTE_V);
	  va += (1L << 30);
	  pa += (1L << 30);
	  len -= (1L << 30);
	  l3cnt++;
	}
      else if (MB2ALIGNED (pa) && MB2ALIGNED (va) && len >= (1 << 21))
	{
	  pte_t *l2p = sv48_get_l2p (root, va, payload);

	  set_pte (l2p, pa >> PAGE_SHIFT,
		   (x ? PTE_X : 0) | PTE_W | PTE_R | PTE_V);
	  va += (1L << 21);
	  pa += (1L << 21);
	  len -= (1L << 21);
	  l2cnt++;
	}
      else
	{
	  pte_t *l1p = sv48_get_l1p (root, va, payload);

	  set_pte (l1p, pa >> PAGE_SHIFT,
		   (x ? PTE_X : 0) | PTE_W | PTE_R | PTE_V);
	  va += (1L << PAGE_SHIFT);
	  pa += (1L << PAGE_SHIFT);
	  len -= (1L << PAGE_SHIFT);
	  l1cnt++;
	}
    }
}

void
sv48_physmap (vaddr_t va, size64_t size, uint64_t pa, enum memory_type mt)
{
  sv48_directmap (sv48_root, pa, va, size, mt, 1, 0);
}

void
sv48_topptalloc (vaddr_t va, size64_t size)
{
  unsigned i, n;

  n = (size + (1 << 30) - 1) >> 30;

  for (i = 0; i < n; i++)
    (void) sv48_get_l3p (sv48_root, va + (i << PAGE_SHIFT), 1);
}

void
sv48_ptalloc (vaddr_t va, size64_t size)
{
  unsigned i, n;

  n = size >> PAGE_SHIFT;

  for (i = 0; i < n; i++)
    (void) sv48_get_l1p (sv48_root, va + (i << PAGE_SHIFT), 1);
}

#define SV48_LINEAR_SHIFT (PAGE_SHIFT + 9 + 9 + 9)
#define SV48_LINEAR_SIZE  (1LL << SV48_LINEAR_SHIFT)
#define SV48_LINEAR_ALIGN (SV48_LINEAR_SIZE - 1)

void
sv48_linear (vaddr_t va, size64_t size)
{
  unsigned l4off = L4OFF64 (va);
  pte_t *l4p;

  if (va & SV48_LINEAR_ALIGN)
    {
      printf ("PAE Linear VA %llx not aligned (align mask: %llx).\n",
	      va, SV48_LINEAR_ALIGN);
      exit (-1);
    }

  if (size < SV48_LINEAR_SIZE)
    {
      printf ("PAE Linear size %llx too small.\n", size);
      exit (-1);
    }

  l4p = sv48_root + l4off;
  set_pte (l4p, (uintptr_t) sv48_root >> PAGE_SHIFT, PTE_V);
  printf ("Wrote %llx at %p\n", *l4p, l4p);
}

void
sv48_populate (vaddr_t va, size64_t size, int u, int w, int x)
{
  ssize64_t len = size;

  while (len > 0)
    {
      sv48_populate_page (sv48_root, va, u, w, x, 1);

      len -= PAGE_CEILING (va) - va;
      va += PAGE_CEILING (va) - va;

    }
}

void
sv48_entry (vaddr_t entry)
{
  md_entry (ARCH_RISCV64, (vaddr_t) (uintptr_t) sv48_root, entry);
}
