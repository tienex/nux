#ifndef _NUX_H
#define _NUX_H

#include <nux/defs.h>
#include <nux/types.h>
#include <nux/locks.h>

pfn_t pfn_alloc (void);
pfn_t pfn_alloc_low (void);
void pfn_free (pfn_t pfn);

vaddr_t kva_alloc (void);
vaddr_t kva_alloc_low (void);
void kva_free (vaddr_t va);

int kmem_brk (int low, vaddr_t vaddr);
vaddr_t kmem_sbrk (int low, long inc);
vaddr_t kmem_brkgrow (int low, unsigned size);
int kmem_brkshrink (int low, unsigned size);

#endif
