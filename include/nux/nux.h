#ifndef _NUX_H
#define _NUX_H

#include <nux/defs.h>
#include <nux/types.h>
#include <nux/locks.h>

pfn_t pfn_alloc (int low);
void pfn_free (pfn_t pfn);

/*
  Define the granularity of allocation of KVA, in pages.

  With KVA_ALLOC_ORDER 2, NUX will allocate (1 << 2) = 4 virtual pages
  at the time.
*/
#define KVA_ALLOC_ORDER 2
#define KVA_ALLOC_SIZE (1L << (KVA_ALLOC_ORDER + PAGE_SHIFT))
vaddr_t kva_allocva (int low);
void kva_freeva (vaddr_t va);

void kmap_map (vaddr_t va, pfn_t pfn, unsigned prot);
int kmap_mapped (vaddr_t va);
int kmap_mapped_range (vaddr_t va, size_t size);
int kmap_ensure (vaddr_t va, unsigned reqprot);
int kmap_ensure_range (vaddr_t va, size_t size, unsigned reqprot);
void kmap_commit (void);

int kmem_brk (int low, vaddr_t vaddr);
vaddr_t kmem_sbrk (int low, long inc);
vaddr_t kmem_brkgrow (int low, unsigned size);
int kmem_brkshrink (int low, unsigned size);
vaddr_t kmem_alloc (int low, size_t size);
void kmem_free (int low, vaddr_t vaddr, size_t size);
void kmem_trim (void);

#endif
