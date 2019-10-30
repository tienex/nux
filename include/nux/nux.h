/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	GPL2.0+
*/

#ifndef _NUX_H
#define _NUX_H

#include <nux/defs.h>
#include <nux/types.h>
#include <nux/locks.h>

void * pfn_get (pfn_t pfn);
void pfn_put (pfn_t pfn, void *va);
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
void *kva_map (int low, pfn_t pfn, unsigned no, unsigned prot);
void *kva_physmap (int low, paddr_t paddr, size_t size, unsigned prot);
void kva_unmap (void *va);

pfn_t kmap_map (vaddr_t va, pfn_t pfn, unsigned prot);
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

void cpu_startall (void);
unsigned cpu_id (void);
unsigned cpu_num (void);
cpumask_t cpu_activemask (void);
void cpu_setdata (void *ptr);
void *cpu_getdata (void);

void cpu_nmi (int cpu);
void cpu_nmi_broadcast (void);
void cpu_nmi_mask (cpumask_t map);

unsigned cpu_ipi_avail (void);
void cpu_ipi (int cpu, uint8_t vct);
void cpu_ipi_mask (cpumask_t map, uint8_t vct);
void cpu_ipi_broadcast (uint8_t vct);

void cpu_tlbflush (int cpu, tlbop_t op, bool sync);
void cpu_tlbflush_mask (cpumask_t mask, tlbop_t op, bool sync);
void cpu_tlbflush_broadcast (tlbop_t op, bool sync);
void cpu_tlbflush_sync_broadcast (void);

bool uaddr_valid (uaddr_t);
bool uaddr_validrange (uaddr_t a, size_t size);
bool uaddr_copyfrom (void *dst, uaddr_t src, size_t size,
		     bool (*pf_handler)(uaddr_t va, hal_pfinfo_t info));
bool uaddr_copyto (uaddr_t dst, void *src, size_t size,
		   bool (*pf_handler)(uaddr_t va, hal_pfinfo_t info));
bool uaddr_memset (uaddr_t dst, int ch, size_t size,
		   bool (*pf_handler)(uaddr_t va, hal_pfinfo_t info));

bool uctxt_signal (uctxt_t *uctxt, unsigned long ip, unsigned long arg,
		   bool (*pf_handler)(uaddr_t va, hal_pfinfo_t info));

#define LOGL_DEBUG -1
#define LOGL_INFO  0
#define LOGL_WARN  1
#define LOGL_ERROR 2
#define LOGL_FATAL 3

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

static inline void __printflike(2, 3)
__log(const int level, const char *fmt, ...)
{
  int r;
  va_list ap;

  if (level == LOGL_WARN)
    printf("Warning: ");
  else if (level == LOGL_FATAL)
    printf("Fatal: ");
  else if (level == LOGL_ERROR)
    printf("ERROR: ");

  va_start (ap, fmt);
  r = vprintf (fmt, ap);
  va_end (ap);

  putchar('\n');

  if (level == LOGL_FATAL)
    exit(-1);
}

#define debug(...) //__log(LOGL_DEBUG, __VA_ARGS__)
#define info(...) __log(LOGL_INFO, __VA_ARGS__)
#define warn(...) __log(LOGL_WARN, __VA_ARGS__)
#define error(...) __log(LOGL_ERROR, __VA_ARGS__)
#define fatal(...) __log(LOGL_FATAL, __VA_ARGS__)
       
#endif
