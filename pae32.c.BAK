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

typedef uint64_t l1e_t;
typedef uint64_t l2e_t;
typedef uint64_t l3e_t;


#define L3_SHIFT (9 + 9 + PAGE_SHIFT)
#define L2_SHIFT (9 + PAGE_SHIFT)
#define L1_SHIFT PAGE_SHIFT

#define mkpte(_p, _f) (((uint64_t)(_p) << PAGE_SHIFT) | (_f))
#define pte_present(_pte) ((_pte) & PTE_P)

#define l2e_present(_l2e) pte_present((uint64_t)(_l2e))
#define l2e_leaf(_l2e) (((_l2e) & (PTE_PS|PTE_P)) == (PTE_PS|PTE_P))

extern int _linear_start;
extern int _linear_l2table;
const vaddr_t linaddr = (vaddr_t) & _linear_start;
const vaddr_t l2_linaddr = (vaddr_t) & _linear_l2table;


static l2e_t *
get_curl2p (vaddr_t va)
{
  l2e_t *ptr;

  va &= ~((1L << L2_SHIFT) - 1);
  ptr = (l3e_t *) (l2_linaddr + (va >> 18));
  return ptr;
}

static l1e_t *
get_curl1p (vaddr_t va)
{
  l1e_t *ptr;

  va &= ~((1L << L1_SHIFT) - 1);

  ptr = (l1e_t *) (linaddr + (va >> 9));
  return ptr;
}

void
set_pte (uint64_t * ptep, uint64_t pte)
{
  volatile uint32_t *ptr = (volatile uint32_t *) ptep;

  /* 32-bit specific.  This or atomic 64 bit write.  */
  *ptr = 0;
  *(ptr + 1) = pte >> 32;
  *ptr = pte & 0xffffffff;
}

l1e_t *
get_l1p (void *pmap, unsigned long va, int alloc)
{
  l2e_t *l2p, l2e;

  assert (pmap == NULL && "Cross-pmap not currently supported.");

  /* Assuming all L3 PTE are present. */

  l2p = get_curl2p (va);
  l2e = *l2p;
  if (!l2e_present (l2e))
    {
      if (!alloc)
	return NULL;

      /* Populate L1. */
      pfn_t pfn;
      uint64_t pte;

      pfn = pfn_alloc (0);
      if (pfn == PFN_INVALID)
	return NULL;

      /* Create an l2e with max permissions. L1 will filter. */
      pte = mkpte (pfn, PTE_U | PTE_P | PTE_W);
      set_pte ((uint64_t *) l2p, pte);
      /* Not present, no TLB flush necessary. */

      l2e = (l2e_t) pte;
    }

  assert (!l2e_leaf (l2e) && "Splintering big pages not supported.");
  /* This is a page walk: check for reserved bit as well. */

  return get_curl1p (va);
}
