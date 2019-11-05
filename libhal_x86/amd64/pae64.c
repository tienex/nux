/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  See COPYING file for the full license.

  SPDX-License-Identifier:	GPL2.0+
*/

#include <assert.h>
#include <stdint.h>
#include <nux/nux.h>
#include "../internal.h"

typedef uint64_t pte_t;

#define L1_SHIFT PAGE_SHIFT
#define L2_SHIFT (L1_SHIFT + 9)
#define L3_SHIFT (L2_SHIFT + 9)
#define L4_SHIFT (L3_SHIFT + 9)

#define L4OFF(_va) (((_va) >> L4_SHIFT) & 0x1ff)
#define L3OFF(_va) (((_va) >> L3_SHIFT) & 0x1ff)
#define L2OFF(_va) (((_va) >> L2_SHIFT) & 0x1ff)
#define L1OFF(_va) (((_va) >> L1_SHIFT) & 0x1ff)

#define UNCANON(_va) ((_va) & ((1LL << 48) - 1))

/* The following RES definitions assume 48-bit MAX PA */
#define L4_RESPT 0x000F000000000080
#define L3_RESPT 0x000F000000000000
#define L3_RES1G 0x000F00003fffe000
#define L2_RESPT 0x000F000000000000
#define L2_RES2M 0x000F0000001fe000
#define L1_RESPT 0x000F000000000000

/* Assume PTE_P is set */
#define l4e_reserved(_pte) ((_pte) & L4_RESPT)
#define l3e_bigpage(_pte) ((_pte) & PTE_PS)
#define l3e_bigpage(_pte) ((_pte) & PTE_PS)
#define l3e_reserved(_pte) ((_pte) & (l3e_bigpage(_pte) ? L3_RES1G : L3_RESPT))
#define l2e_bigpage(_pte) ((_pte) & PTE_PS)
#define l2e_reserved(_pte) ((_pte) & (l2e_bigpage(_pte) ? L2_RES2M : L2_RESPT))
#define l1e_reserved(_pte) ((_pte) & L1_RESPT);

#define mkpte(_p, _f) (((uint64_t)(_p) << PAGE_SHIFT) | (_f))
#define pte_present(_pte) ((_pte) & PTE_P)

extern int _linear_start;
static const pte_t * linaddr = (const pte_t *)&_linear_start;
static pte_t * linaddr_l2;
static pte_t * linaddr_l3;
static pte_t * linaddr_l4;

void set_pte (uint64_t *ptep, uint64_t pte)
{
  *ptep = pte;
}

static pte_t *
get_curl4p (unsigned long va)
{

  return linaddr_l4 + (UNCANON(va) >> L4_SHIFT);
}

static inline pte_t *
get_curl3p (unsigned long va, const int alloc, const int user)
{
  pte_t *l4p, l4e;

  l4p = get_curl4p (va);
  l4e = *l4p;

  if (!pte_present (l4e))
    {
      pfn_t pfn;
      uint64_t pte;

      if (!alloc)
	return NULL;

      pfn = pfn_alloc (0);
      if (pfn == PFN_INVALID)
	return NULL;

      pte = mkpte (pfn, PTE_P | PTE_W | (user ? PTE_U : 0));
      set_pte (l4p, pte);
      /* Not present, no TLB flush necessary. */

      l4e = pte;
    }

  assert (!l4e_reserved (l4e) && "Invalid L4E.");

  return linaddr_l3 + (UNCANON(va) >> L3_SHIFT);
}

static inline pte_t *
get_curl2p (unsigned long va, const int alloc, const int user)
{
  pte_t *l3p, l3e;

  l3p = get_curl3p (va, alloc, user);
  if (l3p == NULL)
    return NULL;

  l3e = *l3p;

  if (!pte_present (l3e))
    {
      pfn_t pfn;
      uint64_t pte;

      if (!alloc)
	return NULL;

      pfn = pfn_alloc (0);
      if (pfn == PFN_INVALID)
	return NULL;

      pte = mkpte (pfn, PTE_P | PTE_W | (user ? PTE_U : 0));
      set_pte (l3p, pte);
      /* Not present, no TLB flush necessary. */

      l3e = pte;
    }

  assert (!l3e_reserved (l3e) && "Invalid L3E.");
  assert (!l3e_bigpage (l3e) && "Splintering not supported at L3");

  return (pte_t *)linaddr_l2 + (UNCANON(va) >> L2_SHIFT);
}

static inline pte_t *
get_curl1p (unsigned long va, const int alloc, const int user)
{
  pte_t *l2p, l2e;

  l2p = get_curl2p (va, alloc, user);
  if (l2p == NULL)
    return NULL;

  l2e = *l2p;

  if (!pte_present (l2e))
    {
      pfn_t pfn;
      uint64_t pte;

      if (!alloc)
	return NULL;

      pfn = pfn_alloc (0);
      if (pfn == PFN_INVALID)
	return NULL;

      pte = mkpte (pfn, PTE_P | PTE_W | (user ? PTE_U : 0));
      set_pte (l2p, pte);
      /* Not present, no TLB flush necessary. */

      l2e = pte;
    }

  assert (!l2e_reserved (l2e) && "Invalid L2E.");
  assert (!l2e_bigpage (l2e) && "Splintering not supported at L2");

  return (pte_t *)linaddr + (UNCANON(va) >> L1_SHIFT);
}

hal_l1e_t *
get_l1p (void *pmap, unsigned long va, int alloc)
{
  assert (pmap == NULL && "Cross-pmap not currently supported.");
  hal_l1e_t *l1p = get_curl1p (va, alloc, 0);

  return l1p;
}

void
do_cleanboot (void)
{

  pte_t *ptep;

  /* Unmap everything down the first 512GB */
  ptep = get_curl4p (0);
  if (ptep == NULL)
    return;

  set_pte (ptep, 0);
  tlbflush_global (); /* Better safe than. */

  /* XXX: Free the unused page tables. */
}

void
pae64_init (void)
{
  linaddr_l2 = (pte_t *)linaddr + (((uintptr_t)linaddr & ((1L << 48) - 1)) >> (9 + 3));
  linaddr_l3 = linaddr_l2 + (((uintptr_t)linaddr & ((1L << 48) - 1)) >> (18 + 3));
  linaddr_l4 = linaddr_l3 + (((uintptr_t)linaddr & ((1L << 48) - 1)) >> (27 + 3));
}
