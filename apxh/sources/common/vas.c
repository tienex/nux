/** @file
  APXH Virtual Address Space Management

  Provides virtual address space management functions for bootloader.
  Handles page table setup, memory mapping, and virtual-to-physical
  address translation for all supported architectures.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org
  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>

// External references to main.c globals
extern ARCH gImageArch;
extern VIRTUAL_ADDRESS gReqPfnmapVa, gReqInfoVa, gReqBatreeVa, gReqRegionVa,
  gKtlsVa, gUtlsVa;
extern SIZE64 gReqPfnmapSize, gReqInfoSize, gReqBatreeSize,
  gReqRegionSize, gKtlsInitsize, gKtlsSize, gUtlsInitsize, gUtlsSize;
extern UINT32 gReqBatreeOrder, gReqRegionNum;
extern UINT64 gMinRamAddr;

// Forward declarations
VOID PlatformVerify(IN VIRTUAL_ADDRESS Va, IN SIZE64 Size);
UINTN GetPayloadPage(VOID);
UINT32 CheckPayloadPage(IN UINT32 Addr);

/**
  Initialize virtual address subsystem.

  Sets up paging structures for the target architecture.
**/
VOID
VasInitialize (
  VOID
  )
{
  switch (gImageArch)
    {
#if (EC_MACHINE_I386) || (EC_MACHINE_AMD64)
    case Arch386:
      PaeInitialize ();
      break;
    case ArchAmd64:
      Pae64Initialize ();
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48Initialize ();
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Get architecture name.

  Returns human-readable architecture name for display.

  @param[in] Arch  Architecture enumeration value.

  @return Pointer to architecture name string.
**/
CONST CHAR *
GetArchName (
  IN ARCH  Arch
  )
{
  switch (Arch)
    {
    case ArchInvalid:
      return "invalid";
    case ArchUnsupported:
      return "unsupported";
    case Arch386:
      return "i386";
    case ArchAmd64:
      return "AMD64";
    case ArchRiscV64:
      return "RISCV64";
    default:
      return "unknown";
    }
}

/**
  Verify virtual address range.

  Validates that virtual address range is suitable for target
  architecture.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
VasVerify (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  switch (gImageArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      PaeVerify (Va, Size);
      break;
    case ArchAmd64:
      Pae64Verify (Va, Size);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48Verify (Va, Size);
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Get physical address from virtual.

  Translates virtual address to physical address using current
  page tables.

  @param[in] Va  Virtual address.

  @return Physical address.
**/
UINTN
VasGetPhysical (
  IN VIRTUAL_ADDRESS  Va
  )
{
  switch (gImageArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      return PaeGetPhysical (Va);
      break;
    case ArchAmd64:
      return Pae64GetPhysical (Va);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      return Sv48GetPhysical (Va);
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Populate virtual address range.

  Allocates and maps pages for the specified virtual address range
  with given permissions.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
  @param[in] U     TRUE for user-accessible.
  @param[in] W     TRUE for writable.
  @param[in] X     TRUE for executable.
**/
VOID
VasPopulate (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size,
  IN INT32       U,
  IN INT32       W,
  IN INT32       X
  )
{
  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  switch (gImageArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      PaePopulate (Va, Size, U, W, X);
      break;
    case ArchAmd64:
      Pae64Populate (Va, Size, U, W, X);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48Populate (Va, Size, U, W, X);
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Copy data to virtual address.

  Copies data from physical address to virtual address range,
  allocating and mapping pages as needed.

  @param[in] Va    Virtual address destination.
  @param[in] Addr Physical address source.
  @param[in] Size  Size to copy.
  @param[in] U     TRUE for user-accessible.
  @param[in] W     TRUE for writable.
  @param[in] X     TRUE for executable.
**/
VOID
VasCopy (
  IN VIRTUAL_ADDRESS   Va,
  IN VOID      *Addr,
  IN SIZE64  Size,
  IN INT32       U,
  IN INT32       W,
  IN INT32       X
  )
{
  SSIZE64 Len = Size;

  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

#if 0
  printf ("Copying %08llx <- %p (u: %d, w:%d, x:%d, %d bytes)\n", Va, Addr, U,
	  W, X, Size);
#endif
  VasPopulate (Va, Size, U, W, X);

  while (Len > 0)
    {
      UINTN PAddr;
      SIZE64 CLen = PAGE_CEILING (Va) - Va;

      if (CLen > Len)
	CLen = Len;

      PAddr = VasGetPhysical (Va);

      memcpy ((VOID *) PAddr, Addr, CLen);

      Len -= CLen;
      Va += CLen;
      Addr += CLen;
    }
}

/**
  Fill virtual address range with value.

  Fills virtual address range with specified byte value,
  allocating and mapping pages as needed.

  @param[in] Va    Virtual address.
  @param[in] C     Byte value to fill.
  @param[in] Size  Size of region.
  @param[in] U     TRUE for user-accessible.
  @param[in] W     TRUE for writable.
  @param[in] X     TRUE for executable.
**/
VOID
VasFill (
  IN VIRTUAL_ADDRESS   Va,
  IN INT32       FillChar,
  IN SIZE64  Size,
  IN INT32       U,
  IN INT32       W,
  IN INT32       X
  )
{
  SSIZE64 Len = Size;

  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  printf ("Setting %08llx <- %d (u:%d, w:%d, x: %d, %d bytes)\n", Va, FillChar, U, W,
	  X, Size);
  VasPopulate (Va, Size, U, W, X);

  while (Len > 0)
    {
      UINTN PAddr;
      SIZE64 CLen = PAGE_CEILING (Va) - Va;

      PAddr = VasGetPhysical (Va);

      memset ((VOID *) PAddr, 0, CLen);

      Len -= CLen;
      Va += CLen;
    }
}

/**
  Map physical memory at virtual address.

  Creates identity mapping of physical memory starting at PA 0.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
  @param[in] Mt    Memory type (WC, WB, UC).
**/
VOID
VasMapPhysical (
  IN VIRTUAL_ADDRESS           Va,
  IN SIZE64          Size,
  IN MEMORY_TYPE  Mt
  )
{
  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  switch (gImageArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      PaeMapPhysical (Va, Size, 0, Mt);
      break;
    case ArchAmd64:
      Pae64MapPhysical (Va, Size, 0, Mt);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48MapPhysical (Va, Size, 0, Mt);
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Map framebuffer at virtual address.

  Maps platform framebuffer memory to specified virtual address
  with appropriate memory type.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
  @param[in] Mt    Memory type (typically WC for framebuffer).
**/
VOID
VasMapFramebuffer (
  IN VIRTUAL_ADDRESS           Va,
  IN SIZE64          Size,
  IN MEMORY_TYPE  Mt
  )
{
  UINT64 Pa;
  FRAMEBUFFER_DESC *FbPtr;

  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  FbPtr = PlatformGetFramebuffer ();
  if (FbPtr == NULL || FbPtr->Type == FB_INVALID)
    return;

  if (FbPtr->Size > Size)
    {
      printf ("ERROR: framebuffer too big. Shrinking int from %lx to %lx\n",
	      FbPtr->Size, Size);
      FbPtr->Size = Size;
    }

  Pa = FbPtr->Addr;

  switch (gImageArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      PaeMapPhysical (Va, Size, Pa, Mt);
      break;
    case ArchAmd64:
      Pae64MapPhysical (Va, Size, Pa, Mt);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48MapPhysical (Va, Size, Pa, Mt);
      break;
#endif
    default:
      (VOID) Pa;
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Set up linear (recursive) mapping.

  Creates recursive page table mapping allowing page tables to be
  accessed as regular memory.

  @param[in] Va    Virtual address for linear mapping.
  @param[in] Size  Size of region.
**/
VOID
VasMapLinear (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  switch (gImageArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      PaeMapLinear (Va, Size);
      break;
    case ArchAmd64:
      Pae64MapLinear (Va, Size);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48MapLinear (Va, Size);
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Allocate top-level page tables.

  Pre-allocates top-level page table structures for address range.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
VasAllocTopPageTable (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  switch (gImageArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      PaeAllocateTopPageTable (Va, Size);
      break;
    case ArchAmd64:
      Pae64AllocateTopPageTable (Va, Size);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48AllocateTopPageTable (Va, Size);
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Allocate page tables.

  Pre-allocates page table structures for address range.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
VasAllocPageTable (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  switch (gImageArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      PaeAllocatePageTable (Va, Size);
      break;
    case ArchAmd64:
      Pae64AllocatePageTable (Va, Size);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48AllocatePageTable (Va, Size);
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Transfer control to kernel.

  Performs final setup and transfers control to loaded kernel
  entry point. Does not return.

  @param[in] Entry  Kernel entry point address.
**/
VOID
VasSetEntry (
  IN VIRTUAL_ADDRESS  Entry
  )
{
  switch (gImageArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      PaeEntry (Entry);
      break;
    case ArchAmd64:
      Pae64Entry (Entry);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48Entry (Entry);
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Set kernel TLS information.

  Records kernel Thread Local Storage configuration for boot info.

  @param[in] Va        Virtual address of TLS template.
  @param[in] InitSize  Size of initialized TLS data.
  @param[in] Size      Total TLS size including BSS.
**/
VOID
VasMapKernelTls (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  InitSize,
  IN SIZE64  Size
  )
{
  gKtlsVa = Va;
  gKtlsInitsize = InitSize;
  gKtlsSize = Size;
}

/**
  Set user TLS information.

  Records user-space Thread Local Storage configuration for boot info.

  @param[in] Va        Virtual address of TLS template.
  @param[in] InitSize  Size of initialized TLS data.
  @param[in] Size      Total TLS size including BSS.
**/
VOID
VasMapUserTls (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  InitSize,
  IN SIZE64  Size
  )
{
  gUtlsVa = Va;
  gUtlsInitsize = InitSize;
  gUtlsSize = Size;
}

// NOTE: The remaining VasMap* functions (Info, Batree, Regions, PageFrameNumbers)
// and their *Copy variants need access to many main.c internals and are too
// tightly coupled to extract cleanly. They remain in main.c for now.
// Future refactoring should extract these into a separate boot-info module.
