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

#endif
