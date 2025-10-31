/** @file
  APXH Virtual Address Space Management

  Provides virtual address space management functions for bootloader.
  Handles page table setup, memory mapping, and virtual-to-physical
  address translation for all supported architectures using COM-based
  IArchitecture interface.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org
  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/arch.h>

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

//
// Current architecture handler
//

static IArchitecture *gCurrentArch = NULL;

/**
  Initialize virtual address subsystem.

  Sets up paging structures for the target architecture using
  IArchitecture COM interface.
**/
VOID
VasInitialize (
  VOID
  )
{
  HRESULT Status;

  // Get architecture handler for target architecture
  gCurrentArch = ArchitectureGet(gImageArch);
  if (gCurrentArch == NULL) {
    printf("ERROR: No architecture handler for %s\n", ArchitectureGetName(gImageArch));
    exit(-1);
  }

  // Initialize architecture-specific paging
  Status = gCurrentArch->lpVtbl->Initialize(gCurrentArch);
  if (FAILED(Status)) {
    printf("ERROR: Architecture initialization failed: 0x%08x\n", Status);
    exit(-1);
  }
}

/**
  Get architecture name.

  Returns human-readable architecture name for display.
  Delegates to ArchitectureGetName from arch.c.

  @param[in] Arch  Architecture enumeration value.

  @return Pointer to architecture name string.
**/
CONST CHAR *
GetArchName (
  IN ARCH  Arch
  )
{
  return ArchitectureGetName(Arch);
}

/**
  Verify virtual address range.

  Validates that virtual address range is suitable for target
  architecture using IArchitecture interface.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
VasVerify (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  if (gCurrentArch == NULL) {
    printf("ERROR: Architecture not initialized\n");
    exit(-1);
  }

  gCurrentArch->lpVtbl->Verify(gCurrentArch, Va, Size);
}

/**
  Get physical address from virtual.

  Translates virtual address to physical address using current
  page tables via IArchitecture interface.

  @param[in] Va  Virtual address.

  @return Physical address.
**/
UINTN
VasGetPhysical (
  IN VIRTUAL_ADDRESS  Va
  )
{
  HRESULT Status;
  UINTN PhysicalAddress;

  if (gCurrentArch == NULL) {
    printf("ERROR: Architecture not initialized\n");
    exit(-1);
  }

  Status = gCurrentArch->lpVtbl->GetPhysical(gCurrentArch, Va, &PhysicalAddress);
  if (FAILED(Status)) {
    printf("ERROR: GetPhysical failed: 0x%08x\n", Status);
    exit(-1);
  }

  return PhysicalAddress;
}

/**
  Populate virtual address range.

  Allocates and maps pages for the specified virtual address range
  with given permissions using IArchitecture interface.

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
  HRESULT Status;

  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  if (gCurrentArch == NULL) {
    printf("ERROR: Architecture not initialized\n");
    exit(-1);
  }

  Status = gCurrentArch->lpVtbl->Populate(gCurrentArch, Va, Size, U, W, X);
  if (FAILED(Status)) {
    printf("ERROR: Populate failed: 0x%08x\n", Status);
    exit(-1);
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

  Creates identity mapping of physical memory starting at PA 0
  using IArchitecture interface.

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
  HRESULT Status;

  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  if (gCurrentArch == NULL) {
    printf("ERROR: Architecture not initialized\n");
    exit(-1);
  }

  Status = gCurrentArch->lpVtbl->MapPhysical(gCurrentArch, Va, Size, 0, Mt);
  if (FAILED(Status)) {
    printf("ERROR: MapPhysical failed: 0x%08x\n", Status);
    exit(-1);
  }
}

/**
  Map framebuffer at virtual address.

  Maps platform framebuffer memory to specified virtual address
  with appropriate memory type using IArchitecture interface.

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
  HRESULT Status;

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

  if (gCurrentArch == NULL) {
    printf("ERROR: Architecture not initialized\n");
    exit(-1);
  }

  Status = gCurrentArch->lpVtbl->MapPhysical(gCurrentArch, Va, Size, Pa, Mt);
  if (FAILED(Status)) {
    printf("ERROR: MapPhysical (framebuffer) failed: 0x%08x\n", Status);
    exit(-1);
  }
}

/**
  Set up linear (recursive) mapping.

  Creates recursive page table mapping allowing page tables to be
  accessed as regular memory using IArchitecture interface.

  @param[in] Va    Virtual address for linear mapping.
  @param[in] Size  Size of region.
**/
VOID
VasMapLinear (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  HRESULT Status;

  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  if (gCurrentArch == NULL) {
    printf("ERROR: Architecture not initialized\n");
    exit(-1);
  }

  Status = gCurrentArch->lpVtbl->MapLinear(gCurrentArch, Va, Size);
  if (FAILED(Status)) {
    printf("ERROR: MapLinear failed: 0x%08x\n", Status);
    exit(-1);
  }
}

/**
  Allocate top-level page tables.

  Pre-allocates top-level page table structures for address range
  using IArchitecture interface.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
VasAllocTopPageTable (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  HRESULT Status;

  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  if (gCurrentArch == NULL) {
    printf("ERROR: Architecture not initialized\n");
    exit(-1);
  }

  Status = gCurrentArch->lpVtbl->AllocateTopPageTable(gCurrentArch, Va, Size);
  if (FAILED(Status)) {
    printf("ERROR: AllocateTopPageTable failed: 0x%08x\n", Status);
    exit(-1);
  }
}

/**
  Allocate page tables.

  Pre-allocates page table structures for address range
  using IArchitecture interface.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
VasAllocPageTable (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  HRESULT Status;

  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  if (gCurrentArch == NULL) {
    printf("ERROR: Architecture not initialized\n");
    exit(-1);
  }

  Status = gCurrentArch->lpVtbl->AllocatePageTable(gCurrentArch, Va, Size);
  if (FAILED(Status)) {
    printf("ERROR: AllocatePageTable failed: 0x%08x\n", Status);
    exit(-1);
  }
}

/**
  Transfer control to kernel.

  Performs final setup and transfers control to loaded kernel
  entry point using IArchitecture interface. Does not return.

  @param[in] Entry  Kernel entry point address.
**/
VOID
VasSetEntry (
  IN VIRTUAL_ADDRESS  Entry
  )
{
  if (gCurrentArch == NULL) {
    printf("ERROR: Architecture not initialized\n");
    exit(-1);
  }

  // This call never returns
  gCurrentArch->lpVtbl->Entry(gCurrentArch, Entry);

  // Should never reach here
  printf("ERROR: Entry function returned unexpectedly\n");
  exit(-1);
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
