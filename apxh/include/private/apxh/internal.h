/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/
#pragma once

#include <ananke/ananke.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <nux/framebuffer.h>
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
  INT32 Type;
  UINT32 Length;
  UINTN PageFrameNumber;
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
typedef enum _APXH_PROGRAM_HEADER_TYPE {
  ApxhProgramHeaderInfo          = 0xAF100000,  ///< Info Page
  ApxhProgramHeaderEmpty         = 0xAF100001,  ///< Empty (no page tables)
  ApxhProgramHeaderPhysicalMap   = 0xAF100002,  ///< 1:1 Memory Map
  ApxhProgramHeaderPfnMap        = 0xAF100003,  ///< PFN Map
  ApxhProgramHeaderBatree        = 0xAF100004,  ///< Allocated Pages Bitmap
  ApxhProgramHeaderPageTableAlloc = 0xAF100005, ///< Empty (alloc all page tables)
  ApxhProgramHeaderFramebuffer   = 0xAF100006,  ///< Frame Buffer
  ApxhProgramHeaderRegions       = 0xAF100007,  ///< Region List
  ApxhProgramHeaderTopPageTableAlloc = 0xAF100008, ///< Empty (alloc all top-level PTs)
  ApxhProgramHeaderLinear        = 0xAF10FFFF   ///< Linear map
} APXH_PROGRAM_HEADER_TYPE;

// Legacy compatibility
#define PHT_APXH_INFO       ApxhProgramHeaderInfo
#define PHT_APXH_EMPTY      ApxhProgramHeaderEmpty
#define PHT_APXH_PHYSMAP    ApxhProgramHeaderPhysicalMap
#define PHT_APXH_PFNMAP     ApxhProgramHeaderPfnMap
#define PHT_APXH_BATREE     ApxhProgramHeaderBatree
#define PHT_APXH_PTALLOC    ApxhProgramHeaderPageTableAlloc
#define PHT_APXH_FRAMEBUF   ApxhProgramHeaderFramebuffer
#define PHT_APXH_REGIONS    ApxhProgramHeaderRegions
#define PHT_APXH_TOPPTALLOC ApxhProgramHeaderTopPageTableAlloc
#define PHT_APXH_LINEAR     ApxhProgramHeaderLinear

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

VOID PlatformInit (VOID);
UINT64 PlatformGetMaxPageFrameNumber (VOID);
UINT64 PlatformGetMinRamPageFrameNumber (VOID);
UINT64 PlatformGetMaxRamPageFrameNumber (VOID);
UINT32 PlatformGetMemoryRegionCount (VOID);
BOOTINFO_REGION *PlatformGetMemoryRegion (IN UINT32 Index);
FRAMEBUFFER_DESC *PlatformGetFramebuffer (VOID);
APXH_PLATFORM_DESCRIPTOR *PlatformGetDescriptor (VOID);
VOID PlatformVerify (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID PlatformEntry (IN ARCH Architecture, IN VIRTUAL_ADDRESS PageTable, IN VIRTUAL_ADDRESS EntryPoint);

VOID *PayloadGet (IN UINT32 Index, OUT OPTIONAL UINTN *Size);

typedef enum _PAYLOAD_ID
{
  PayloadKernel,
  PayloadUser,
} PAYLOAD_ID;

VOID *GetPayloadStart (IN INT32 ArgumentCount, IN char *ArgumentVector[], IN PAYLOAD_ID PayloadId);
UINTN GetPayloadSize (IN PAYLOAD_ID PayloadId);

ARCH GetElfArch (IN VOID *ElfImage);
VIRTUAL_ADDRESS LoadElf32 (IN VOID *ElfImage, IN INT32 IsUserMode);
VIRTUAL_ADDRESS LoadElf64 (IN VOID *ElfImage, IN INT32 IsUserMode);

UINTN GetPage (VOID);
UINTN GetPayloadPage (VOID);

VOID VirtualAddressInit (VOID);
UINTN VirtualAddressGetPhysical (IN VIRTUAL_ADDRESS VirtualAddress);
VOID VirtualAddressVerify (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VirtualAddressPopulate (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable);
VOID VirtualAddressCopy (IN VIRTUAL_ADDRESS VirtualAddress, IN VOID *SourceAddress, IN SIZE64 Size, IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable);
VOID VirtualAddressMemset (IN VIRTUAL_ADDRESS VirtualAddress, IN INT32 FillChar, IN SIZE64 Size, IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable);
VOID VirtualAddressMapPhysical (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN MEMORY_TYPE Type);
VOID VirtualAddressMapLinear (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VirtualAddressMapInfo (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VirtualAddressMapPageFrameNumbers (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VirtualAddressMapBatree (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VirtualAddressAllocateTopPageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VirtualAddressAllocatePageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VirtualAddressMapFramebuffer (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN MEMORY_TYPE Type);
VOID VirtualAddressMapRegions (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VirtualAddressMapKernelTls (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 InitializedSize, IN SIZE64 TotalSize);
VOID VirtualAddressMapUserTls (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 InitializedSize, IN SIZE64 TotalSize);
VOID VirtualAddressSetEntry (IN VIRTUAL_ADDRESS EntryPoint);

VOID PaeInit (VOID);
UINTN PaeGetPhysical (IN VIRTUAL_ADDRESS VirtualAddress);
VOID PaeVerify (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID PaePopulate (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable);
VOID PaeMapPhysical (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN UINT64 PhysicalAddress, IN MEMORY_TYPE Type);
VOID PaeAllocatePageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID PaeAllocateTopPageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID PaeMapLinear (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID PaeEntry (IN VIRTUAL_ADDRESS EntryPoint);

/* Internal PAE functions. */
VOID PaeDirectMap (IN VOID *PageTable, IN UINT64 PhysicalAddress, IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size,
		    IN MEMORY_TYPE Type, IN INT32 IsPayload, IN INT32 IsExecutable);
VOID PaeMapPage (IN VOID *PageTable, IN VIRTUAL_ADDRESS VirtualAddress, IN UINTN PhysicalAddress, IN INT32 IsPayload, IN INT32 IsWritable,
		   IN INT32 IsExecutable);

VOID Pae64Init (VOID);
UINTN Pae64GetPhysical (IN VIRTUAL_ADDRESS VirtualAddress);
VOID Pae64Verify (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Pae64Populate (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable);
VOID Pae64MapPhysical (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN UINT64 PhysicalAddress, IN MEMORY_TYPE Type);
VOID Pae64AllocatePageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Pae64AllocateTopPageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Pae64MapLinear (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Pae64Entry (IN VIRTUAL_ADDRESS EntryPoint);

/* Internal PAE64 functions. */
VOID Pae64DirectMap (IN VOID *PageTable, IN UINT64 PhysicalAddress, IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size,
		      IN MEMORY_TYPE Type, IN INT32 IsPayload, IN INT32 IsExecutable);
VOID Pae64MapPage (IN VOID *PageTable, IN VIRTUAL_ADDRESS VirtualAddress, IN UINTN PhysicalAddress, IN INT32 IsPayload, IN INT32 IsWritable,
		     IN INT32 IsExecutable);

VOID Sv48Init (VOID);
UINTN Sv48GetPhysical (IN VIRTUAL_ADDRESS VirtualAddress);
VOID Sv48Verify (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Sv48Populate (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable);
VOID Sv48MapPhysical (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN UINT64 PhysicalAddress, IN MEMORY_TYPE Type);
VOID Sv48AllocatePageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Sv48AllocateTopPageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Sv48MapLinear (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Sv48Entry (IN VIRTUAL_ADDRESS EntryPoint);

/* Internal SV48 functions. */
VOID Sv48DirectMap (IN VOID *PageTable, IN UINT64 PhysicalAddress, IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size,
		     IN MEMORY_TYPE Type, IN INT32 IsPayload, IN INT32 IsExecutable);
VOID Sv48MapPage (IN VOID *PageTable, IN VIRTUAL_ADDRESS VirtualAddress, IN UINTN PhysicalAddress, IN INT32 IsPayload, IN INT32 IsWritable,
		    IN INT32 IsExecutable);


#define info(...) do { printf (__VA_ARGS__); putchar('\n'); } while (0)
#define debug(...) do { printf (__VA_ARGS__); putchar('\n'); } while (0)
#define warn(...) do { printf ("Warning: "); printf (__VA_ARGS__); putchar('\n'); } while (0)
#define fatal(...) do { printf ("Fatal: "); printf (__VA_ARGS__); putchar('\n'); exit(-1); } while (0)

