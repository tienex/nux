/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef __apxh_internal_h__
#define __apxh_internal_h__

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <framebuffer.h>
#include <apxh/apxh.h>

#define BOOTMEM MB(512)		/* We won't be using more than 512Mb to boot. Promise. */

typedef int64_t ssize64_t;
typedef uint64_t size64_t;
typedef uint64_t vaddr_t;

/**
  Boot Info Region Type
**/
typedef enum _BOOTINFO_REGION_TYPE {
  BootInfoRegionUnknown = 0,  ///< Unusable address
  BootInfoRegionRam     = 1,  ///< Available RAM
  BootInfoRegionOther   = 2,  ///< Non-RAM physical address
  BootInfoRegionBusy    = 3   ///< Boot allocated RAM
} BOOTINFO_REGION_TYPE;

/** Legacy compatibility **/
#define BOOTINFO_REGION_UNKNOWN BootInfoRegionUnknown
#define BOOTINFO_REGION_RAM     BootInfoRegionRam
#define BOOTINFO_REGION_OTHER   BootInfoRegionOther
#define BOOTINFO_REGION_BSY     BootInfoRegionBusy

/**
  Boot Info Region Descriptor
**/
typedef struct _BOOTINFO_REGION
{
  int type;
  unsigned len;
  unsigned long pfn;
} BOOTINFO_REGION, *PBOOTINFO_REGION, *PCBOOTINFO_REGION;

/** Legacy compatibility **/
#define bootinfo_region BOOTINFO_REGION

/**
  Memory Type for Page Mappings
**/
typedef enum _MEMORY_TYPE
{
  MemTypeWriteCombining = 0,  ///< Write Combining
  MemTypeWriteBack      = 1,  ///< Write Back (cacheable)
  MemTypeUncached       = 2   ///< Uncached
} MEMORY_TYPE;

/** Legacy compatibility **/
#define memory_type MEMORY_TYPE
#define MEMTYPE_WC  MemTypeWriteCombining
#define MEMTYPE_WB  MemTypeWriteBack
#define MEMTYPE_UC  MemTypeUncached

/* APXH ELF extensions. */
#define PHT_APXH_INFO       0xAF100000	/* Info Page. */
#define PHT_APXH_EMPTY      0xAF100001	/* Empty (no page tables). */
#define PHT_APXH_PHYSMAP    0xAF100002	/* 1:1 Memory Map. */
#define PHT_APXH_PFNMAP     0xAF100003	/* PFN Map. */
#define PHT_APXH_STREE      0xAF100004	/* Allocated Pages Bitmap. */
#define PHT_APXH_PTALLOC    0xAF100005	/* Empty (alloc all page tables). */
#define PHT_APXH_FRAMEBUF   0xAF100006	/* Frame Buffer. */
#define PHT_APXH_REGIONS    0xAF100007	/* Region List. */
#define PHT_APXH_TOPPTALLOC 0xAF100008	/* Empty (alloc all top-level PTs). */
#define PHT_APXH_LINEAR     0xAF10FFFF	/* Linear map. */

#define PFNMAP_ENTRY_SIZE 64

#define PAGE_SHIFT 12
#define PAGE_SIZE (1LL << PAGE_SHIFT)
#define PAGE_MASK ~(PAGE_SIZE - 1)

#define PAGE2M_SHIFT 21
#define PAGE2M_SIZE (1LL << PAGE2M_SHIFT)
#define PAGE2M_MASK ~(PAGE2M_SIZE - 1)

#define MB(_x) ((unsigned long)(_x) << 20)
#define BITMAP_SZ(_s) ((_s) >> 3)	// POW2
#define PAGEMAP_SZ(_s) BITMAP_SZ((_s) >> PAGE_SHIFT)	// POW2

#define PAGE_ROUND(_a) (((_a) + (PAGE_SIZE-1)) & PAGE_MASK)
#define PAGE_CEILING(_a) (((_a) + PAGE_SIZE) & PAGE_MASK)

typedef enum
{
  ARCH_INVALID,
  ARCH_UNSUPPORTED,
  ARCH_386,
  ARCH_AMD64,
  ARCH_RISCV64,
} arch_t;

void md_init (void);
uint64_t md_maxpfn (void);
uint64_t md_minrampfn (void);
uint64_t md_maxrampfn (void);
unsigned md_memregions (void);
BOOTINFO_REGION *md_getmemregion (unsigned i);
struct fbdesc *md_getframebuffer (void);
APXH_PLATFORM_DESCRIPTOR *md_getplatformdesc (void);
void md_verify (vaddr_t va, size64_t size);
void md_entry (arch_t arch, vaddr_t pt, vaddr_t entry);

void *payload_get (unsigned i, size_t *size);

typedef enum
{
  PAYLOAD_KERNEL,
  PAYLOAD_USER,
} plid_t;

void *get_payload_start (int argc, char *argv[], plid_t id);
size_t get_payload_size (plid_t id);

arch_t get_elf_arch (void *elf);
vaddr_t load_elf32 (void *elf, int u);
vaddr_t load_elf64 (void *elf, int u);

uintptr_t get_page (void);
uintptr_t get_payload_page (void);

void va_init (void);
uintptr_t va_getphys (vaddr_t va);
void va_verify (vaddr_t va, size64_t size);
void va_populate (vaddr_t va, size64_t size, int u, int w, int x);
void va_copy (vaddr_t va, void *addr, size64_t size, int u, int w, int x);
void va_memset (vaddr_t va, int c, size64_t size, int u, int w, int x);
void va_physmap (vaddr_t va, size64_t size, MEMORY_TYPE Type);
void va_linear (vaddr_t va, size64_t size);
void va_info (vaddr_t va, size64_t size);
void va_pfnmap (vaddr_t va, size64_t size);
void va_stree (vaddr_t va, size64_t size);
void va_topptalloc (vaddr_t va, size64_t size);
void va_ptalloc (vaddr_t va, size64_t size);
void va_framebuf (vaddr_t va, size64_t size, MEMORY_TYPE Type);
void va_regions (vaddr_t va, size64_t size);
void va_ktls (vaddr_t va, size64_t initsize, size64_t size);
void va_utls (vaddr_t va, size64_t initsize, size64_t size);
void va_entry (vaddr_t entry);

void pae_init (void);
uintptr_t pae_getphys (vaddr_t va);
void pae_verify (vaddr_t va, size64_t size);
void pae_populate (vaddr_t va, size64_t size, int u, int w, int x);
void pae_physmap (vaddr_t va, size64_t size, uint64_t pa, MEMORY_TYPE Type);
void pae_ptalloc (vaddr_t va, size64_t size);
void pae_topptalloc (vaddr_t va, size64_t size);
void pae_linear (vaddr_t va, size64_t size);
void pae_entry (vaddr_t entry);

/* Internal PAE functions. */
void pae_directmap (void *pt, uint64_t pa, vaddr_t va, size64_t size,
		    MEMORY_TYPE Type, int payload, int x);
void pae_map_page (void *pt, vaddr_t va, uintptr_t pa, int payload, int w,
		   int x);

void pae64_init (void);
uintptr_t pae64_getphys (vaddr_t va);
void pae64_verify (vaddr_t va, size64_t size);
void pae64_populate (vaddr_t va, size64_t size, int u, int w, int x);
void pae64_physmap (vaddr_t va, size64_t size, uint64_t pa, MEMORY_TYPE Type);
void pae64_ptalloc (vaddr_t va, size64_t size);
void pae64_topptalloc (vaddr_t va, size64_t size);
void pae64_linear (vaddr_t va, size64_t size);
void pae64_entry (vaddr_t entry);

/* Internal PAE64 functions. */
void pae64_directmap (void *pt, uint64_t pa, vaddr_t va, size64_t size,
		      MEMORY_TYPE Type, int payload, int x);
void pae64_map_page (void *pt, vaddr_t va, uintptr_t pa, int payload, int w,
		     int x);

void sv48_init (void);
uintptr_t sv48_getphys (vaddr_t va);
void sv48_verify (vaddr_t va, size64_t size);
void sv48_populate (vaddr_t va, size64_t size, int u, int w, int x);
void sv48_physmap (vaddr_t va, size64_t size, uint64_t pa, MEMORY_TYPE Type);
void sv48_ptalloc (vaddr_t va, size64_t size);
void sv48_topptalloc (vaddr_t va, size64_t size);
void sv48_linear (vaddr_t va, size64_t size);
void sv48_entry (vaddr_t entry);

/* Internal SV48 functions. */
void sv48_directmap (void *pt, uint64_t pa, vaddr_t va, size64_t size,
		     MEMORY_TYPE Type, int payload, int x);
void sv48_map_page (void *pt, vaddr_t va, uintptr_t pa, int payload, int w,
		    int x);


#define info(...) do { printf (__VA_ARGS__); putchar('\n'); } while (0)
#define debug(...) do { printf (__VA_ARGS__); putchar('\n'); } while (0)
#define warn(...) do { printf ("Warning: "); printf (__VA_ARGS__); putchar('\n'); } while (0)
#define fatal(...) do { printf ("Fatal: "); printf (__VA_ARGS__); putchar('\n'); exit(-1); } while (0)

#endif
