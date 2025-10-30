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

typedef INT64 SSIZE64;
typedef UINT64 SIZE64;
typedef UINT64 VIRTUAL_ADDRESS;

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
  INT32 type;
  UINT32 len;
  UINTN pfn;
} BOOTINFO_REGION, *PBOOTINFO_REGION, *PCBOOTINFO_REGION;

/**
  Memory Type for Page Mappings
**/
typedef enum _MEMORY_TYPE
{
  MemTypeWriteCombining = 0,  ///< Write Combining
  MemTypeWriteBack      = 1,  ///< Write Back (cacheable)
  MemTypeUncached       = 2   ///< Uncached
} MEMORY_TYPE;

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

#define MB(_x) ((UINTN)(_x) << 20)
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
} ARCH;

VOID MdInit (VOID);
UINT64 MdMaxPfn (VOID);
UINT64 MdMinRamPfn (VOID);
UINT64 MdMaxRamPfn (VOID);
UINT32 MdMemRegions (VOID);
BOOTINFO_REGION *MdGetMemRegion (IN UINT32 i);
FRAMEBUFFER_DESC *MdGetFramebuffer (VOID);
APXH_PLATFORM_DESCRIPTOR *MdGetPlatformDesc (VOID);
VOID MdVerify (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID MdEntry (IN ARCH Arch, IN VIRTUAL_ADDRESS Pt, IN VIRTUAL_ADDRESS Entry);

VOID *PayloadGet (IN UINT32 i, OUT OPTIONAL UINTN *Size);

typedef enum _PAYLOAD_ID
{
  PayloadKernel,
  PayloadUser,
} PAYLOAD_ID;

VOID *GetPayloadStart (IN INT32 Argc, IN char *Argv[], IN PAYLOAD_ID Id);
UINTN GetPayloadSize (IN PAYLOAD_ID Id);

ARCH GetElfArch (IN VOID *Elf);
VIRTUAL_ADDRESS LoadElf32 (IN VOID *Elf, IN INT32 U);
VIRTUAL_ADDRESS LoadElf64 (IN VOID *Elf, IN INT32 U);

UINTN GetPage (VOID);
UINTN GetPayloadPage (VOID);

VOID VaInit (VOID);
UINTN VaGetPhys (IN VIRTUAL_ADDRESS Va);
VOID VaVerify (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID VaPopulate (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size, IN INT32 U, IN INT32 W, IN INT32 X);
VOID VaCopy (IN VIRTUAL_ADDRESS Va, IN VOID *Addr, IN SIZE64 Size, IN INT32 U, IN INT32 W, IN INT32 X);
VOID VaMemset (IN VIRTUAL_ADDRESS Va, IN INT32 C, IN SIZE64 Size, IN INT32 U, IN INT32 W, IN INT32 X);
VOID VaPhysmap (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size, IN MEMORY_TYPE Type);
VOID VaLinear (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID VaInfo (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID VaPfnmap (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID VaStree (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID VaTopPtAlloc (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID VaPtAlloc (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID VaFramebuf (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size, IN MEMORY_TYPE Type);
VOID VaRegions (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID VaKtls (IN VIRTUAL_ADDRESS Va, IN SIZE64 InitSize, IN SIZE64 Size);
VOID VaUtls (IN VIRTUAL_ADDRESS Va, IN SIZE64 InitSize, IN SIZE64 Size);
VOID VaEntry (IN VIRTUAL_ADDRESS Entry);

VOID PaeInit (VOID);
UINTN PaeGetPhys (IN VIRTUAL_ADDRESS Va);
VOID PaeVerify (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID PaePopulate (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size, IN INT32 U, IN INT32 W, IN INT32 X);
VOID PaePhysmap (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size, IN UINT64 Pa, IN MEMORY_TYPE Type);
VOID PaePtAlloc (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID PaeTopPtAlloc (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID PaeLinear (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID PaeEntry (IN VIRTUAL_ADDRESS Entry);

/* Internal PAE functions. */
VOID PaeDirectMap (IN VOID *Pt, IN UINT64 Pa, IN VIRTUAL_ADDRESS Va, IN SIZE64 Size,
		    IN MEMORY_TYPE Type, IN INT32 Payload, IN INT32 X);
VOID PaeMapPage (IN VOID *Pt, IN VIRTUAL_ADDRESS Va, IN UINTN Pa, IN INT32 Payload, IN INT32 W,
		   IN INT32 X);

VOID Pae64Init (VOID);
UINTN Pae64GetPhys (IN VIRTUAL_ADDRESS Va);
VOID Pae64Verify (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID Pae64Populate (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size, IN INT32 U, IN INT32 W, IN INT32 X);
VOID Pae64Physmap (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size, IN UINT64 Pa, IN MEMORY_TYPE Type);
VOID Pae64PtAlloc (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID Pae64TopPtAlloc (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID Pae64Linear (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID Pae64Entry (IN VIRTUAL_ADDRESS Entry);

/* Internal PAE64 functions. */
VOID Pae64DirectMap (IN VOID *Pt, IN UINT64 Pa, IN VIRTUAL_ADDRESS Va, IN SIZE64 Size,
		      IN MEMORY_TYPE Type, IN INT32 Payload, IN INT32 X);
VOID Pae64MapPage (IN VOID *Pt, IN VIRTUAL_ADDRESS Va, IN UINTN Pa, IN INT32 Payload, IN INT32 W,
		     IN INT32 X);

VOID Sv48Init (VOID);
UINTN Sv48GetPhys (IN VIRTUAL_ADDRESS Va);
VOID Sv48Verify (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID Sv48Populate (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size, IN INT32 U, IN INT32 W, IN INT32 X);
VOID Sv48Physmap (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size, IN UINT64 Pa, IN MEMORY_TYPE Type);
VOID Sv48PtAlloc (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID Sv48TopPtAlloc (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID Sv48Linear (IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
VOID Sv48Entry (IN VIRTUAL_ADDRESS Entry);

/* Internal SV48 functions. */
VOID Sv48DirectMap (IN VOID *Pt, IN UINT64 Pa, IN VIRTUAL_ADDRESS Va, IN SIZE64 Size,
		     IN MEMORY_TYPE Type, IN INT32 Payload, IN INT32 X);
VOID Sv48MapPage (IN VOID *Pt, IN VIRTUAL_ADDRESS Va, IN UINTN Pa, IN INT32 Payload, IN INT32 W,
		    IN INT32 X);


#define info(...) do { printf (__VA_ARGS__); putchar('\n'); } while (0)
#define debug(...) do { printf (__VA_ARGS__); putchar('\n'); } while (0)
#define warn(...) do { printf ("Warning: "); printf (__VA_ARGS__); putchar('\n'); } while (0)
#define fatal(...) do { printf ("Fatal: "); printf (__VA_ARGS__); putchar('\n'); exit(-1); } while (0)

#endif
