/** @file
  APXH OpenVMS Image Loader

  Provides OpenVMS executable image format implementation for VAX, Alpha,
  and Itanium systems. OpenVMS uses a proprietary image format with a
  512-byte Image Header Section (IHS) followed by Image Section Descriptors (ISDs).

  Documentation:
  - OpenVMS Internals and Data Structures Manual (IDSM)
  - ihsdef.h (Image Header Section) - defines IHS$_* constants
  - ihddef.h (Image Header Descriptor) - defines IHD$_* constants
  - Source: https://www.digiater.nl/openvms/freeware/v80/symbols/symbols.zip

  OpenVMS Image Structure:
  - 512-byte IHS (Image Header Section) at offset 0
  - Variable number of ISDs (Image Section Descriptors) following IHS
  - Sections: code, data, fixup, debug symbols

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// OpenVMS Image Header Section (IHS) - Simplified version
// Based on ihsdef.h structure
//

#define VMS_IHS_SIZE       512    ///< IHS size in bytes
#define VMS_IHS_SIGNATURE  0x0153  ///< IHS signature ("EXE" in RADIX-50)

ANX_PACK_PUSH(1)

typedef struct _VMS_IHS {
  UINT16  MajorId;              ///< Major structure ID
  UINT16  MinorId;              ///< Minor structure ID
  UINT32  ImageSize;            ///< Size of entire image
  UINT32  IsdOffset;            ///< Offset to first ISD
  UINT32  IsdCount;             ///< Number of ISDs
  UINT32  VirtualPageCount;     ///< Number of virtual pages
  UINT32  TransferAddress;      ///< Entry point address
  UINT32  GlobalSymbolTable;    ///< GST offset
  UINT32  GlobalSymbolTableSize; ///< GST size
  UINT32  FixupSection;         ///< Fixup section offset
  UINT32  FixupSectionSize;     ///< Fixup section size
  UINT32  DebugSymbolTable;     ///< DST offset
  UINT32  DebugSymbolTableSize; ///< DST size
  UINT8   TargetSystem;         ///< Target OS (1=VMS)
  UINT8   ImageType;            ///< Image type (1=executable, 2=shareable)
  UINT8   TargetArch;           ///< Target architecture (1=VAX, 2=Alpha)
  UINT8   Reserved1;
  CHAR8   ImageName[40];        ///< Image identification
  CHAR8   ImageId[16];          ///< Image ID string
  UINT32  LinkerId;             ///< Linker version
  UINT32  LinkerTime[2];        ///< Link timestamp (VMS format)
  UINT8   Reserved2[400];       ///< Reserved/padding to 512 bytes
} VMS_IHS;

ANX_PACK_POP()

//
// OpenVMS Image Section Descriptor (ISD)
// Based on ihddef.h structure
//

ANX_PACK_PUSH(1)

typedef struct _VMS_ISD {
  UINT16  Size;                 ///< Size of this descriptor
  UINT16  Type;                 ///< Section type
  UINT32  VirtualPageNumber;    ///< Virtual page number
  UINT32  PageCount;            ///< Number of pages
  UINT32  Flags;                ///< Section flags
  UINT32  VirtualBlockNumber;   ///< File block number
  UINT32  Reserved[4];          ///< Reserved
} VMS_ISD;

ANX_PACK_POP()

//
// ISD Type Values
//

#define VMS_ISD_NORMAL     1  ///< Normal loadable section
#define VMS_ISD_FIXUP      2  ///< Fixup section
#define VMS_ISD_GLOBAL     3  ///< Global symbol table
#define VMS_ISD_DEBUG      4  ///< Debug symbol table

//
// ISD Flags
//

#define VMS_ISD_WRITE      0x00000001  ///< Writable
#define VMS_ISD_EXEC       0x00000002  ///< Executable
#define VMS_ISD_RESIDENT   0x00000004  ///< Memory resident
#define VMS_ISD_PROTECT    0x00000008  ///< Protected

//
// Target Architecture Values
//

#define VMS_ARCH_VAX       1  ///< VAX
#define VMS_ARCH_ALPHA     2  ///< Alpha AXP
#define VMS_ARCH_IA64      3  ///< Itanium (IA-64)

//
// IImageLoader Implementation for OpenVMS
//

/**
  Detect if image is OpenVMS format.
**/
static
HRESULT
STDMETHODCALLTYPE
VmsDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  VMS_IHS *Ihs;

  if (ImageSize < sizeof(VMS_IHS)) {
    return S_FALSE;
  }

  Ihs = (VMS_IHS *)ImageBase;

  // Check IHS signature and size
  if (Ihs->MajorId == VMS_IHS_SIGNATURE && Ihs->MinorId <= 2) {
    return S_OK;
  }

  return S_FALSE;
}

/**
  Get architecture from OpenVMS image.
**/
static
HRESULT
STDMETHODCALLTYPE
VmsGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  VMS_IHS *Ihs;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  Ihs = (VMS_IHS *)ImageBase;

  switch (Ihs->TargetArch) {
    case VMS_ARCH_VAX:
      *Architecture = ARCH_VAX;
      break;
    case VMS_ARCH_ALPHA:
      *Architecture = ARCH_ALPHA;
      break;
    case VMS_ARCH_IA64:
      *Architecture = ARCH_IA64;
      break;
    default:
      *Architecture = ARCH_UNSUPPORTED;
      return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  return S_OK;
}

/**
  Get endianness from OpenVMS image.
**/
static
HRESULT
STDMETHODCALLTYPE
VmsGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  // All OpenVMS architectures are little-endian
  *Endianness = ImgEndianLittle;
  return S_OK;
}

/**
  Get entry point from OpenVMS image.
**/
static
HRESULT
STDMETHODCALLTYPE
VmsGetEntryPoint (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT VIRTUAL_ADDRESS   *EntryPoint
  )
{
  VMS_IHS *Ihs;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  Ihs = (VMS_IHS *)ImageBase;
  *EntryPoint = Ihs->TransferAddress;

  if (*EntryPoint == 0) {
    return IMGLOAD_E_INVALID_HEADER;
  }

  return S_OK;
}

/**
  Load OpenVMS image.
**/
static
HRESULT
STDMETHODCALLTYPE
VmsLoadImage (
  IN     IImageLoader     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VMS_IHS *Ihs;
  VMS_ISD *Isd;
  UINT32 i;
  HRESULT Status;

  if (Context == NULL) {
    return E_POINTER;
  }

  Ihs = (VMS_IHS *)Context->ImageBase;

  // Populate context
  Status = VmsGetArch(This, Context->ImageBase, &Context->Architecture);
  if (FAILED(Status)) {
    return Status;
  }

  Status = VmsGetEndianness(This, Context->ImageBase, &Context->Endianness);
  if (FAILED(Status)) {
    return Status;
  }

  Status = VmsGetEntryPoint(This, Context->ImageBase, &Context->EntryPoint);
  if (FAILED(Status)) {
    return Status;
  }

  // Process Image Section Descriptors (ISDs)
  if (Ihs->IsdOffset != 0 && Ihs->IsdCount > 0) {
    Isd = (VMS_ISD *)((UINT8 *)Context->ImageBase + Ihs->IsdOffset);

    for (i = 0; i < Ihs->IsdCount; i++) {
      UINT32 Vpn = Isd[i].VirtualPageNumber;
      UINT32 Pages = Isd[i].PageCount;
      UINT32 Flags = Isd[i].Flags;
      UINT32 FileBlock = Isd[i].VirtualBlockNumber;

      // Map section based on type and flags
      switch (Isd[i].Type) {
        case VMS_ISD_NORMAL:
          // Normal loadable section
          printf("VMS: Section %d at VPN %08X, %d pages, flags %08X\n",
                 i, Vpn, Pages, Flags);
          // TODO: Map virtual pages
          break;

        case VMS_ISD_FIXUP:
          // Fixup/relocation section
          printf("VMS: Fixup section at VPN %08X\n", Vpn);
          break;

        case VMS_ISD_GLOBAL:
          // Global symbol table
          printf("VMS: GST at VPN %08X\n", Vpn);
          break;

        case VMS_ISD_DEBUG:
          // Debug symbols
          printf("VMS: DST at VPN %08X\n", Vpn);
          break;

        default:
          break;
      }
    }
  }

  // Full loading not yet implemented
  return E_NOTIMPL;
}

/**
  Extract TLS information from OpenVMS image.
**/
static
HRESULT
STDMETHODCALLTYPE
VmsGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // OpenVMS has process-private sections but not traditional TLS
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from OpenVMS image.
**/
static
HRESULT
STDMETHODCALLTYPE
VmsGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  // OpenVMS has exception handling but format is system-specific
  // Would need to parse exception handler tables from ISDs
  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
VmsGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  VMS_IHS *Ihs;

  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  Ihs = (VMS_IHS *)ImageBase;

  // Would need to parse GST (Global Symbol Table) at Ihs->GlobalSymbolTable
  // GST format is complex and requires IDSM documentation
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
VmsGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (Name == NULL || SymbolInfo == NULL) {
    return E_POINTER;
  }

  // Would need to parse GST (Global Symbol Table)
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  IUnknown::QueryInterface implementation.
**/
static
HRESULT
STDMETHODCALLTYPE
VmsQueryInterface (
  IN  IImageLoader  *This,
  IN  REFIID        riid,
  OUT VOID          **ppvObject
  )
{
  if (ppvObject == NULL) {
    return E_POINTER;
  }

  *ppvObject = NULL;

  if (memcmp(riid, &IID_IImageLoader, sizeof(GUID)) == 0 ||
      memcmp(riid, &IID_IUnknown, sizeof(GUID)) == 0) {
    *ppvObject = This;
    return S_OK;
  }

  return E_NOINTERFACE;
}

/**
  IUnknown::AddRef implementation.
**/
static
UINT32
STDMETHODCALLTYPE
VmsAddRef (
  IN IImageLoader  *This
  )
{
  return 1;
}

/**
  IUnknown::Release implementation.
**/
static
UINT32
STDMETHODCALLTYPE
VmsRelease (
  IN IImageLoader  *This
  )
{
  return 1;
}

//
// OpenVMS Loader VTable
//

static CONST IImageLoaderVtbl gVmsVtbl = {
  VmsQueryInterface,
  VmsAddRef,
  VmsRelease,
  VmsDetect,
  VmsGetArch,
  VmsGetEndianness,
  VmsGetEntryPoint,
  VmsLoadImage,
  VmsGetTlsInfo,
  VmsGetUnwindInfo,
  VmsGetSymbolByAddress,
  VmsGetSymbolByName
};

//
// OpenVMS Loader Instance
//

IImageLoader gVmsLoader = {
  &gVmsVtbl
};

// Auto-register this loader
ANX_REGISTER_IMGLOADER(gVmsLoader);
