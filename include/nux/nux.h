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

#endif
