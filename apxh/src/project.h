#ifndef _PROJECT_H
#define _PROJECT_H

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define BOOTINFO_REGION_UNKNOWN 0	/* Unusable address. */
#define BOOTINFO_REGION_RAM 1 		/* Available RAM. */
#define BOOTINFO_REGION_OTHER 2		/* Non-RAM physical address. */
#define BOOTINFO_REGION_BSY 3 		/* Boot allocated RAM. */

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
#define PHT_APXH_LINEAR   0xAF10FFFF

#define PFNMAP_ENTRY_SIZE 64

#define PAGE_SHIFT 12
#define PAGE_SIZE (1LL << PAGE_SHIFT)
#define PAGE_MASK ~(PAGE_SIZE - 1)

#define PAGE2M_SHIFT 21
#define PAGE2M_SIZE (1LL << PAGE2M_SHIFT)
#define PAGE2M_MASK ~(PAGE2M_SIZE - 1)

#define MB(_x) ((unsigned long)(_x) << 20)
#define BITMAP_SZ(_s) ((_s) >> 3) // POW2
#define PAGEMAP_SZ(_s) BITMAP_SZ((_s) >> PAGE_SHIFT) // POW2

#define PAGE_ROUND(_a) (((_a) + (PAGE_SIZE-1)) & PAGE_MASK)
#define PAGE_CEILING(_a) (((_a) + PAGE_SIZE) & PAGE_MASK)

typedef enum {
  ARCH_INVALID,
  ARCH_UNSUPPORTED,
  ARCH_386,
} arch_t;

void md_init(void);
uint64_t md_maxpfn (void);
unsigned md_memregions (void);
struct bootinfo_region *md_getmemregion (unsigned i);
void md_verify(unsigned long va, size_t size);

void * payload_get (unsigned i, size_t *size);

void * get_payload_start (int argc, char *argv[]);
size_t get_payload_size (void);

arch_t get_elf_arch (void *elf);
uintptr_t load_elf32 (void *elf);

uintptr_t get_page (void);
uintptr_t get_payload_page (void);

void va_init (void);
uintptr_t va_getphys (unsigned long va);
void va_verify (unsigned long va, size_t size);
void va_populate (unsigned long va, size_t size, int w, int x);
void va_copy (unsigned long va, void *addr, size_t size, int w, int x);
void va_memset (unsigned long va, int c, size_t size, int w, int x);
void va_physmap (unsigned long va, size_t size);
void va_linear (unsigned long va, size_t size);
void va_info (unsigned long va, size_t size);
void va_pfnmap (unsigned long va, size_t size);
void va_stree (unsigned long va, size_t size);
void va_entry (unsigned long entry);

void pae_init (void);
uintptr_t pae_getphys (unsigned long va);
void pae_verify (unsigned long va, size_t size);
void pae_populate (unsigned long va, size_t size, int w, int x);
void pae_physmap (unsigned long va, size_t size);
void pae_linear (unsigned long va, size_t size);
void pae_entry (unsigned long entry);

#endif
