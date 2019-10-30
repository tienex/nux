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

#define mkpte(_p, _f) (((uint64_t)(_p) << PAGE_SHIFT) | (_f))
#define pte_present(_pte) ((_pte) & PTE_P)

extern int _linear_start;
static const uintptr_t linaddr = (uintptr_t)&_linear_start;
static uintptr_t linoff;
static uintptr_t linaddr_l2;
static uintptr_t linaddr_l3;
static uintptr_t linaddr_l4;

static pte_t *
get_curl4p (unsigned long va)
{

  return (pte_t *)(linaddr_l4 + L4OFF(va));
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

      pte = mkpte (pfn, user ? PTE_U : 0 |PTE_P|PTE_W);
      set_pte (l4p, pte);
      /* Not present, no TLB flush necessary. */

      l4e = pte;
    }

  assert (!l4e_reserved (l4e) || "Invalid L4E.");

  return (pte_t *)(linaddr_l3 + L3OFF(va));
}

static inline pte_t *
get_curl2p (unsigned long va, const int alloc, const int user)
{
  pte_t *l3p, l3e;

  l3p = get_curl3p (va, alloc, user);
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

      pte = mkpte (pfn, user ? PTE_U : 0 |PTE_P|PTE_W);
      set_pte (l3p, pte);
      /* Not present, no TLB flush necessary. */

      l3e = pte;
    }

  assert (!l3e_reserved (l3e) || "Invalid L3E.");
  assert (!l3_bigpage (l3e) || "Splintering not supported at L3");

  return (pte_t *)(linaddr_l2 + L2OFF(va));
}

void set_pte (uint64_t *ptep, uint64_t pte)
{
  *ptep = pte;
}

hal_l1e_t * get_l1p (void *pmap, unsigned long va, int alloc)
{
  return NULL;
}

void
pae64_init (void)
{
  linoff = L4OFF(linaddr);
  linaddr_l2 = linaddr + (linoff << L3_SHIFT);
  linaddr_l3 = linaddr_l2 + (linoff << L2_SHIFT);
  linaddr_l4 = linaddr_l3 + (linoff << L1_SHIFT);
}
