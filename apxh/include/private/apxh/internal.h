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

VOID MdInit (VOID);
UINT64 MdMaxPfn (VOID);
UINT64 MdMinRamPfn (VOID);
UINT64 MdMaxRamPfn (VOID);
unsigned MdMemRegions (VOID);
BOOTINFO_REGION *MdGetMemRegion (unsigned i);
FRAMEBUFFER_DESC *MdGetFramebuffer (VOID);
APXH_PLATFORM_DESCRIPTOR *MdGetPlatformDesc (VOID);
VOID MdVerify (vaddr_t Va, size64_t Size);
VOID MdEntry (arch_t Arch, vaddr_t Pt, vaddr_t Entry);

VOID *PayloadGet (unsigned i, size_t *Size);

/** Legacy compatibility **/
#define md_init MdInit
#define md_maxpfn MdMaxPfn
#define md_minrampfn MdMinRamPfn
#define md_maxrampfn MdMaxRamPfn
#define md_memregions MdMemRegions
#define md_getmemregion MdGetMemRegion
#define md_getframebuffer MdGetFramebuffer
#define md_getplatformdesc MdGetPlatformDesc
#define md_verify MdVerify
#define md_entry MdEntry
#define payload_get PayloadGet

typedef enum _PAYLOAD_ID
{
  PayloadKernel,
  PayloadUser,
} PAYLOAD_ID;

/** Legacy compatibility **/
#define plid_t PAYLOAD_ID
#define PAYLOAD_KERNEL PayloadKernel
#define PAYLOAD_USER PayloadUser

VOID *GetPayloadStart (int Argc, char *Argv[], PAYLOAD_ID Id);
size_t GetPayloadSize (PAYLOAD_ID Id);

arch_t GetElfArch (VOID *Elf);
vaddr_t LoadElf32 (VOID *Elf, int U);
vaddr_t LoadElf64 (VOID *Elf, int U);

uintptr_t GetPage (VOID);
uintptr_t GetPayloadPage (VOID);

VOID VaInit (VOID);
uintptr_t VaGetPhys (vaddr_t Va);
VOID VaVerify (vaddr_t Va, size64_t Size);
VOID VaPopulate (vaddr_t Va, size64_t Size, int U, int W, int X);
VOID VaCopy (vaddr_t Va, VOID *Addr, size64_t Size, int U, int W, int X);
VOID VaMemset (vaddr_t Va, int C, size64_t Size, int U, int W, int X);
VOID VaPhysmap (vaddr_t Va, size64_t Size, MEMORY_TYPE Type);
VOID VaLinear (vaddr_t Va, size64_t Size);
VOID VaInfo (vaddr_t Va, size64_t Size);
VOID VaPfnmap (vaddr_t Va, size64_t Size);
VOID VaStree (vaddr_t Va, size64_t Size);
VOID VaTopPtAlloc (vaddr_t Va, size64_t Size);
VOID VaPtAlloc (vaddr_t Va, size64_t Size);
VOID VaFramebuf (vaddr_t Va, size64_t Size, MEMORY_TYPE Type);
VOID VaRegions (vaddr_t Va, size64_t Size);
VOID VaKtls (vaddr_t Va, size64_t InitSize, size64_t Size);
VOID VaUtls (vaddr_t Va, size64_t InitSize, size64_t Size);
VOID VaEntry (vaddr_t Entry);

/** Legacy compatibility **/
#define get_payload_start GetPayloadStart
#define get_payload_size GetPayloadSize
#define get_elf_arch GetElfArch
#define load_elf32 LoadElf32
#define load_elf64 LoadElf64
#define get_page GetPage
#define get_payload_page GetPayloadPage
#define va_init VaInit
#define va_getphys VaGetPhys
#define va_verify VaVerify
#define va_populate VaPopulate
#define va_copy VaCopy
#define va_memset VaMemset
#define va_physmap VaPhysmap
#define va_linear VaLinear
#define va_info VaInfo
#define va_pfnmap VaPfnmap
#define va_stree VaStree
#define va_topptalloc VaTopPtAlloc
#define va_ptalloc VaPtAlloc
#define va_framebuf VaFramebuf
#define va_regions VaRegions
#define va_ktls VaKtls
#define va_utls VaUtls
#define va_entry VaEntry

VOID PaeInit (VOID);
uintptr_t PaeGetPhys (vaddr_t Va);
VOID PaeVerify (vaddr_t Va, size64_t Size);
VOID PaePopulate (vaddr_t Va, size64_t Size, int U, int W, int X);
VOID PaePhysmap (vaddr_t Va, size64_t Size, uint64_t Pa, MEMORY_TYPE Type);
VOID PaePtAlloc (vaddr_t Va, size64_t Size);
VOID PaeTopPtAlloc (vaddr_t Va, size64_t Size);
VOID PaeLinear (vaddr_t Va, size64_t Size);
VOID PaeEntry (vaddr_t Entry);

/* Internal PAE functions. */
VOID PaeDirectMap (VOID *Pt, uint64_t Pa, vaddr_t Va, size64_t Size,
		    MEMORY_TYPE Type, int Payload, int X);
VOID PaeMapPage (VOID *Pt, vaddr_t Va, uintptr_t Pa, int Payload, int W,
		   int X);

VOID Pae64Init (VOID);
uintptr_t Pae64GetPhys (vaddr_t Va);
VOID Pae64Verify (vaddr_t Va, size64_t Size);
VOID Pae64Populate (vaddr_t Va, size64_t Size, int U, int W, int X);
VOID Pae64Physmap (vaddr_t Va, size64_t Size, uint64_t Pa, MEMORY_TYPE Type);
VOID Pae64PtAlloc (vaddr_t Va, size64_t Size);
VOID Pae64TopPtAlloc (vaddr_t Va, size64_t Size);
VOID Pae64Linear (vaddr_t Va, size64_t Size);
VOID Pae64Entry (vaddr_t Entry);

/* Internal PAE64 functions. */
VOID Pae64DirectMap (VOID *Pt, uint64_t Pa, vaddr_t Va, size64_t Size,
		      MEMORY_TYPE Type, int Payload, int X);
VOID Pae64MapPage (VOID *Pt, vaddr_t Va, uintptr_t Pa, int Payload, int W,
		     int X);

VOID Sv48Init (VOID);
uintptr_t Sv48GetPhys (vaddr_t Va);
VOID Sv48Verify (vaddr_t Va, size64_t Size);
VOID Sv48Populate (vaddr_t Va, size64_t Size, int U, int W, int X);
VOID Sv48Physmap (vaddr_t Va, size64_t Size, uint64_t Pa, MEMORY_TYPE Type);
VOID Sv48PtAlloc (vaddr_t Va, size64_t Size);
VOID Sv48TopPtAlloc (vaddr_t Va, size64_t Size);
VOID Sv48Linear (vaddr_t Va, size64_t Size);
VOID Sv48Entry (vaddr_t Entry);

/* Internal SV48 functions. */
VOID Sv48DirectMap (VOID *Pt, uint64_t Pa, vaddr_t Va, size64_t Size,
		     MEMORY_TYPE Type, int Payload, int X);
VOID Sv48MapPage (VOID *Pt, vaddr_t Va, uintptr_t Pa, int Payload, int W,
		    int X);

/** Legacy compatibility **/
#define pae_init PaeInit
#define pae_getphys PaeGetPhys
#define pae_verify PaeVerify
#define pae_populate PaePopulate
#define pae_physmap PaePhysmap
#define pae_ptalloc PaePtAlloc
#define pae_topptalloc PaeTopPtAlloc
#define pae_linear PaeLinear
#define pae_entry PaeEntry
#define pae_directmap PaeDirectMap
#define pae_map_page PaeMapPage
#define pae64_init Pae64Init
#define pae64_getphys Pae64GetPhys
#define pae64_verify Pae64Verify
#define pae64_populate Pae64Populate
#define pae64_physmap Pae64Physmap
#define pae64_ptalloc Pae64PtAlloc
#define pae64_topptalloc Pae64TopPtAlloc
#define pae64_linear Pae64Linear
#define pae64_entry Pae64Entry
#define pae64_directmap Pae64DirectMap
#define pae64_map_page Pae64MapPage
#define sv48_init Sv48Init
#define sv48_getphys Sv48GetPhys
#define sv48_verify Sv48Verify
#define sv48_populate Sv48Populate
#define sv48_physmap Sv48Physmap
#define sv48_ptalloc Sv48PtAlloc
#define sv48_topptalloc Sv48TopPtAlloc
#define sv48_linear Sv48Linear
#define sv48_entry Sv48Entry
#define sv48_directmap Sv48DirectMap
#define sv48_map_page Sv48MapPage


#define info(...) do { printf (__VA_ARGS__); putchar('\n'); } while (0)
#define debug(...) do { printf (__VA_ARGS__); putchar('\n'); } while (0)
#define warn(...) do { printf ("Warning: "); printf (__VA_ARGS__); putchar('\n'); } while (0)
#define fatal(...) do { printf ("Fatal: "); printf (__VA_ARGS__); putchar('\n'); exit(-1); } while (0)

#endif
