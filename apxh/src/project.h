/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	GPL2.0+
*/

#ifndef _PROJECT_H
#define _PROJECT_H

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <framebuffer.h>
#include <nux/apxh.h>

#define BOOTMEM MB(512)		/* We won't be using more than 512Mb to boot. Promise. */

typedef int64_t ssize64_t;
typedef uint64_t size64_t;
typedef uint64_t vaddr_t;

#define BOOTINFO_REGION_UNKNOWN 0	/* Unusable address. */
#define BOOTINFO_REGION_RAM 1	/* Available RAM. */
#define BOOTINFO_REGION_OTHER 2	/* Non-RAM physical address. */
#define BOOTINFO_REGION_BSY 3	/* Boot allocated RAM. */

struct bootinfo_region
{
  int type;
  unsigned len;
  unsigned long pfn;
};

/* APXH ELF extensions. */
#define PHT_APXH_INFO     0xAF100000
#define PHT_APXH_EMPTY    0xAF100001
#define PHT_APXH_PHYSMAP  0xAF100002
#define PHT_APXH_PFNMAP   0xAF100003
#define PHT_APXH_STREE    0xAF100004
#define PHT_APXH_PTALLOC  0xAF100005
#define PHT_APXH_FRAMEBUF 0xAF100006
#define PHT_APXH_LINEAR   0xAF10FFFF

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
} arch_t;

void md_init (void);
uint64_t md_maxpfn (void);
uint64_t md_acpi_rsdp (void);
unsigned md_memregions (void);
struct bootinfo_region *md_getmemregion (unsigned i);
struct fbdesc *md_getframebuffer (void);
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
void va_physmap (vaddr_t va, size64_t size);
void va_linear (vaddr_t va, size64_t size);
void va_info (vaddr_t va, size64_t size);
void va_pfnmap (vaddr_t va, size64_t size);
void va_stree (vaddr_t va, size64_t size);
void va_ptalloc (vaddr_t va, size64_t size);
void va_framebuf (vaddr_t va, size64_t size);
void va_entry (vaddr_t entry);

void pae_init (void);
uintptr_t pae_getphys (vaddr_t va);
void pae_verify (vaddr_t va, size64_t size);
void pae_populate (vaddr_t va, size64_t size, int u, int w, int x);
void pae_physmap (vaddr_t va, size64_t size, uint64_t pa);
void pae_ptalloc (vaddr_t va, size64_t size);
void pae_linear (vaddr_t va, size64_t size);
void pae_entry (vaddr_t entry);

/* Internal PAE functions. */
void pae_directmap (void *pt, uint64_t pa, vaddr_t va, size64_t size,
		    int payload, int x);
void pae_map_page (void *pt, vaddr_t va, uintptr_t pa, int payload, int w,
		   int x);

void pae64_init (void);
uintptr_t pae64_getphys (vaddr_t va);
void pae64_verify (vaddr_t va, size64_t size);
void pae64_populate (vaddr_t va, size64_t size, int u, int w, int x);
void pae64_physmap (vaddr_t va, size64_t size, uint64_t pa);
void pae64_ptalloc (vaddr_t va, size64_t size);
void pae64_linear (vaddr_t va, size64_t size);
void pae64_entry (vaddr_t entry);

/* Internal PAE64 functions. */
void pae64_directmap (void *pt, uint64_t pa, vaddr_t va, size64_t size,
		      int payload, int x);
void pae64_map_page (void *pt, vaddr_t va, uintptr_t pa, int payload, int w,
		     int x);


#define info(...) do { printf (__VA_ARGS__); putchar('\n'); } while (0)
#define debug(...) do { printf (__VA_ARGS__); putchar('\n'); } while (0)
#define warn(...) do { printf ("Warning: "); printf (__VA_ARGS__); putchar('\n'); } while (0)
#define fatal(...) do { printf ("Fatal: "); printf (__VA_ARGS__); putchar('\n'); exit(-1); } while (0)

#endif
