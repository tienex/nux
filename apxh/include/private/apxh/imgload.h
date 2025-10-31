/** @file
  APXH Image Loader COM Interface

  Defines COM-style interface for executable image loaders supporting
  multiple formats: ELF, Mach-O, PE/COFF, and NLM. Provides standardized
  loading, TLS handling, unwinding info, and entry point discovery for
  kernel and user executables.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

#include <ananke/ananke.h>
#include <apxh/internal.h>

//
// Image Loader HRESULT Codes
//

#define IMGLOAD_E_INVALID_FORMAT       ((HRESULT)0x80090001L)  ///< Invalid or corrupted image format
#define IMGLOAD_E_UNSUPPORTED_ARCH     ((HRESULT)0x80090002L)  ///< Unsupported architecture
#define IMGLOAD_E_UNSUPPORTED_VERSION  ((HRESULT)0x80090003L)  ///< Unsupported format version
#define IMGLOAD_E_INVALID_HEADER       ((HRESULT)0x80090005L)  ///< Invalid header structure
#define IMGLOAD_E_TLS_ERROR            ((HRESULT)0x80090006L)  ///< TLS setup failed

//
// Target System Enumeration
//

typedef enum _IMGLOAD_TARGET_SYSTEM {
  ImgSystemNone          = 0,   ///< No specific system
  ImgSystemUnknown       = 1,   ///< Unknown system
  ImgSystemUnix          = 2,   ///< Generic Unix
  ImgSystemLinux         = 3,   ///< Linux
  ImgSystemFreeBsd       = 4,   ///< FreeBSD
  ImgSystemOpenBsd       = 5,   ///< OpenBSD
  ImgSystemNetBsd        = 6,   ///< NetBSD
  ImgSystemDragonFly     = 7,   ///< DragonFly BSD
  ImgSystemSolaris       = 8,   ///< Solaris
  ImgSystemIrix          = 9,   ///< IRIX
  ImgSystemHpux          = 10,  ///< HP-UX
  ImgSystemAix           = 11,  ///< AIX
  ImgSystemOsf1          = 12,  ///< OSF/1 (Digital Unix, Tru64)
  ImgSystemXenix         = 13,  ///< XENIX
  ImgSystemSvr3          = 14,  ///< System V Release 3
  ImgSystemSvr4          = 15,  ///< System V Release 4
  ImgSystemBsd           = 16,  ///< Generic BSD
  ImgSystemNetware       = 17,  ///< Novell NetWare
  ImgSystemWindows       = 18,  ///< Microsoft Windows
  ImgSystemWindowsNt     = 19,  ///< Windows NT family
  ImgSystemWindowsCe     = 20,  ///< Windows CE
  ImgSystemDos           = 21,  ///< MS-DOS
  ImgSystemOs2           = 22,  ///< IBM OS/2
  ImgSystemMacOs         = 23,  ///< Classic Mac OS (pre-X)
  ImgSystemMacOsX        = 24,  ///< macOS (Mac OS X)
  ImgSystemIos           = 25,  ///< Apple iOS
  ImgSystemTvos          = 26,  ///< Apple tvOS
  ImgSystemWatchOs       = 27,  ///< Apple watchOS
  ImgSystemAmigaOs       = 28,  ///< Amiga OS
  ImgSystemAtariTos      = 29,  ///< Atari TOS
  ImgSystemOpenVms       = 30,  ///< OpenVMS
  ImgSystemPlan9         = 31,  ///< Plan 9 from Bell Labs
  ImgSystemBeOs          = 32,  ///< BeOS
  ImgSystemHaiku         = 33,  ///< Haiku
  ImgSystemQnx           = 34,  ///< QNX
  ImgSystemMinix         = 35,  ///< MINIX
  ImgSystemEmbedded      = 36,  ///< Generic embedded system
  ImgSystemUefi          = 37,  ///< UEFI firmware
  ImgSystemBios          = 38,  ///< BIOS/legacy firmware
  ImgSystemBaremetal     = 39   ///< Bare-metal/freestanding
} IMGLOAD_TARGET_SYSTEM;

//
// Target Subsystem Enumeration
//

typedef enum _IMGLOAD_TARGET_SUBSYSTEM {
  ImgSubsystemNone               = 0,   ///< No subsystem
  ImgSubsystemUnknown            = 1,   ///< Unknown subsystem
  ImgSubsystemNative             = 2,   ///< Native/kernel mode
  ImgSubsystemWindowsGui         = 3,   ///< Windows GUI application
  ImgSubsystemWindowsCui         = 4,   ///< Windows console application
  ImgSubsystemWindowsCeGui       = 5,   ///< Windows CE GUI
  ImgSubsystemEfiApplication     = 6,   ///< UEFI application
  ImgSubsystemEfiBootDriver      = 7,   ///< UEFI boot service driver
  ImgSubsystemEfiRuntimeDriver   = 8,   ///< UEFI runtime driver
  ImgSubsystemEfiRom             = 9,   ///< UEFI ROM image
  ImgSubsystemPosix              = 10,  ///< POSIX console application
  ImgSubsystemWindowsBootApp     = 11,  ///< Windows boot application
  ImgSubsystemXbox               = 12,  ///< Xbox system
  ImgSubsystemUnixCli            = 13,  ///< Unix command-line interface
  ImgSubsystemUnixDaemon         = 14,  ///< Unix daemon/service
  ImgSubsystemSharedLibrary      = 15,  ///< Shared library/DLL
  ImgSubsystemKernelModule       = 16,  ///< Kernel module/driver
  ImgSubsystemFirmware           = 17,  ///< Firmware component
  ImgSubsystemBootLoader         = 18,  ///< Boot loader
  ImgSubsystemOs2Gui             = 19,  ///< OS/2 Presentation Manager GUI
  ImgSubsystemOs2Cui             = 20   ///< OS/2 console application
} IMGLOAD_TARGET_SUBSYSTEM;

//
// System Version Structure
//

typedef struct _IMGLOAD_SYSTEM_VERSION {
  UINT16  Major;      ///< Major version
  UINT16  Minor;      ///< Minor version
  UINT32  Build;      ///< Build number (optional, 0 if not applicable)
  UINT32  Revision;   ///< Revision number (optional, 0 if not applicable)
} IMGLOAD_SYSTEM_VERSION;

//
// Symbol Type Enumeration
//

typedef enum _IMGLOAD_SYMBOL_TYPE {
  ImgSymbolTypeNone    = 0,  ///< No type specified
  ImgSymbolTypeObject  = 1,  ///< Data object (variable)
  ImgSymbolTypeFunc    = 2,  ///< Function or executable code
  ImgSymbolTypeSection = 3,  ///< Section symbol
  ImgSymbolTypeFile    = 4,  ///< File name symbol
  ImgSymbolTypeCommon  = 5,  ///< Common data object
  ImgSymbolTypeTls     = 6   ///< Thread-local storage object
} IMGLOAD_SYMBOL_TYPE;

//
// Symbol Binding Enumeration
//

typedef enum _IMGLOAD_SYMBOL_BINDING {
  ImgSymbolBindLocal  = 0,  ///< Local symbol
  ImgSymbolBindGlobal = 1,  ///< Global symbol
  ImgSymbolBindWeak   = 2   ///< Weak symbol
} IMGLOAD_SYMBOL_BINDING;

//
// Unwinding Format Enumeration
//

typedef enum _IMGLOAD_UNWIND_FORMAT {
  ImgUnwindFormatNone       = 0,  ///< No unwinding information
  ImgUnwindFormatDwarfEhFrame = 1,  ///< DWARF .eh_frame format
  ImgUnwindFormatPeExceptionTable = 2,  ///< PE .pdata exception table
  ImgUnwindFormatMachOCompactUnwind = 3,  ///< Mach-O __unwind_info
  ImgUnwindFormatArm = 4  ///< ARM Exception Index Table (.ARM.exidx)
} IMGLOAD_UNWIND_FORMAT;

//
// Relocation Format Enumeration
//

typedef enum _IMGLOAD_RELOC_FORMAT {
  ImgRelocFormatNone    = 0,   ///< No relocations
  ImgRelocFormatElfRel  = 1,   ///< ELF REL (without addend)
  ImgRelocFormatElfRela = 2,   ///< ELF RELA (with addend)
  ImgRelocFormatPe      = 3,   ///< PE/COFF base relocations
  ImgRelocFormatMachO   = 4,   ///< Mach-O relocations
  ImgRelocFormatCoff    = 5,   ///< COFF relocations
  ImgRelocFormatAout    = 6,   ///< a.out relocations
  ImgRelocFormatLe      = 7,   ///< LE/LX relocations
  ImgRelocFormatNlm     = 8,   ///< NetWare NLM relocations
  ImgRelocFormatHunk    = 9,   ///< Amiga HUNK relocations
  ImgRelocFormatAtari   = 10,  ///< Atari ST relocations
  ImgRelocFormatRose    = 11,  ///< OSF/ROSE relocations
  ImgRelocFormatXenix   = 12,  ///< XENIX relocations
  ImgRelocFormatVms     = 13,  ///< VMS relocations
  ImgRelocFormatPdp10   = 14,  ///< PDP-10 relocations
  ImgRelocFormatSom     = 15   ///< HP-UX SOM relocations
} IMGLOAD_RELOC_FORMAT;

//
// Endianness Enumeration
//

typedef enum _IMGLOAD_ENDIAN {
  ImgEndianUnknown = 0,  ///< Unknown endianness
  ImgEndianLittle  = 1,  ///< Little-endian
  ImgEndianBig     = 2   ///< Big-endian
} IMGLOAD_ENDIAN;

//
// TLS Information Structure
//

typedef struct _IMGLOAD_TLS_INFO {
  VIRTUAL_ADDRESS  InitDataAddr;     ///< Address of initialized TLS data
  UINT64           InitDataSize;     ///< Size of initialized TLS data
  UINT64           TotalSize;        ///< Total TLS size (init + BSS)
  VIRTUAL_ADDRESS  IndexAddr;        ///< TLS index address (optional)
  VIRTUAL_ADDRESS  CallbacksAddr;    ///< TLS callback array address (optional)
  UINT32           Alignment;        ///< TLS alignment requirement
} IMGLOAD_TLS_INFO, *PIMGLOAD_TLS_INFO;

//
// Unwinding Information Structure
//

typedef struct _IMGLOAD_UNWIND_INFO {
  VIRTUAL_ADDRESS        UnwindDataAddr;   ///< Address of unwinding data (.eh_frame, .pdata)
  UINT64                 UnwindDataSize;   ///< Size of unwinding data
  VIRTUAL_ADDRESS        UnwindIndexAddr;  ///< Address of unwinding index (optional)
  UINT64                 UnwindIndexSize;  ///< Size of unwinding index
  IMGLOAD_UNWIND_FORMAT  Format;           ///< Unwinding information format
} IMGLOAD_UNWIND_INFO, *PIMGLOAD_UNWIND_INFO;

//
// Symbol Information Structure
//

typedef struct _IMGLOAD_SYMBOL_INFO {
  CONST CHAR8              *Name;     ///< Symbol name (null-terminated)
  VIRTUAL_ADDRESS          Address;   ///< Symbol virtual address
  UINT64                   Size;      ///< Symbol size
  IMGLOAD_SYMBOL_TYPE      Type;      ///< Symbol type
  IMGLOAD_SYMBOL_BINDING   Binding;   ///< Symbol binding
} IMGLOAD_SYMBOL_INFO, *PIMGLOAD_SYMBOL_INFO;

//
// Relocation Information Structure
//

typedef struct _IMGLOAD_RELOC_INFO {
  VIRTUAL_ADDRESS      PreferredBase;    ///< Preferred load base address
  VIRTUAL_ADDRESS      RelocTableAddr;   ///< Address of relocation table
  UINT64               RelocTableSize;   ///< Size of relocation table
  IMGLOAD_RELOC_FORMAT Format;           ///< Relocation format
  BOOLEAN              RequiresReloc;    ///< TRUE if image requires relocation
} IMGLOAD_RELOC_INFO, *PIMGLOAD_RELOC_INFO;

//
// Image Load Context
//

typedef struct _IMGLOAD_CONTEXT {
  VOID                  *ImageBase;       ///< Base address of image in memory
  UINTN                 ImageSize;        ///< Total image size
  BOOLEAN               IsUserMode;       ///< TRUE for user-space, FALSE for kernel
  ARCH                  Architecture;     ///< Target architecture
  IMGLOAD_ENDIAN        Endianness;       ///< Image endianness
  VIRTUAL_ADDRESS       EntryPoint;       ///< Entry point virtual address
  IMGLOAD_TLS_INFO      KernelTls;        ///< Kernel TLS information
  IMGLOAD_TLS_INFO      UserTls;          ///< User TLS information
  IMGLOAD_UNWIND_INFO   UnwindInfo;       ///< Unwinding information
} IMGLOAD_CONTEXT, *PIMGLOAD_CONTEXT;

//
// Forward declarations
//

typedef struct _IImageLoader IImageLoader;

//
// Image Loader Interface GUID
// {A57F0B12-8D4E-4F1A-9C3B-2E6F8A4D7C91}
//

#define ANX_IID_IImageLoader "A57F0B12-8D4E-4F1A-9C3B-2E6F8A4D7C91"
ANX_DEFINE_GUID(IID_IImageLoader, 0xA57F0B12,0x8D4E,0x4F1A,0x9C,0x3B,0x2E,0x6F,0x8A,0x4D,0x7C,0x91);

//
// Image Loader Auto-Registration Support
//
// Loaders place a pointer in the .imgloaders section which the registry
// will discover at initialization. Use APXH_REGISTER_IMGLOADER() macro.
//

#define APXH_REGISTER_IMGLOADER(LoaderVar) \
  ANX_ATTR_SECTION(".imgloaders") ANX_ATTR_USED \
  static IImageLoader * CONST _imgloader_ptr_##LoaderVar = &LoaderVar

//
// Image Loader Interface (COM-style with ANX macros)
//

ANX_BEGIN_INTERFACE(IImageLoader, IUnknown, IID_IImageLoader, ANX_IID_IImageLoader)
  /**
    Detect if image format is supported by this loader.

    @param[in] ImageBase  Pointer to image in memory.
    @param[in] ImageSize  Size of image.

    @return S_OK if format is recognized, S_FALSE otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, Detect, (
    IN VOID          *ImageBase,
    IN UINTN         ImageSize
    ))

  /**
    Get target architecture from image.

    @param[in]  ImageBase     Pointer to image in memory.
    @param[out] Architecture  Receives architecture type (ARCH_*).

    @return S_OK on success, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, GetArch, (
    IN  VOID   *ImageBase,
    OUT ARCH   *Architecture
    ))

  /**
    Get endianness from image.

    @param[in]  ImageBase   Pointer to image in memory.
    @param[out] Endianness  Receives endianness type.

    @return S_OK on success, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, GetEndianness, (
    IN  VOID             *ImageBase,
    OUT IMGLOAD_ENDIAN   *Endianness
    ))

  /**
    Get entry point virtual address from image.

    @param[in]  ImageBase   Pointer to image in memory.
    @param[out] EntryPoint  Receives entry point virtual address.

    @return S_OK on success, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, GetEntryPoint, (
    IN  VOID              *ImageBase,
    OUT VIRTUAL_ADDRESS   *EntryPoint
    ))

  /**
    Load image segments and set up address space.

    @param[in,out] Context  Load context with image information.

    @return S_OK on success, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, LoadImage, (
    IN OUT IMGLOAD_CONTEXT   *Context
    ))

  /**
    Extract TLS information from image.

    @param[in]  ImageBase  Pointer to image in memory.
    @param[out] TlsInfo    Receives TLS information.

    @return S_OK on success, S_FALSE if no TLS, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, GetTlsInfo, (
    IN  VOID               *ImageBase,
    OUT IMGLOAD_TLS_INFO   *TlsInfo
    ))

  /**
    Extract unwinding information from image.

    @param[in]  ImageBase   Pointer to image in memory.
    @param[out] UnwindInfo  Receives unwinding information.

    @return S_OK on success, S_FALSE if no unwinding info, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, GetUnwindInfo, (
    IN  VOID                 *ImageBase,
    OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
    ))

  /**
    Look up symbol by virtual address.

    @param[in]  ImageBase    Pointer to image in memory.
    @param[in]  Address      Virtual address to look up.
    @param[out] SymbolInfo   Receives symbol information.

    @return S_OK on success, S_FALSE if not found, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, GetSymbolByAddress, (
    IN  VOID                  *ImageBase,
    IN  VIRTUAL_ADDRESS       Address,
    OUT IMGLOAD_SYMBOL_INFO   *SymbolInfo
    ))

  /**
    Look up symbol by name.

    @param[in]  ImageBase    Pointer to image in memory.
    @param[in]  Name         Symbol name to look up.
    @param[out] SymbolInfo   Receives symbol information.

    @return S_OK on success, S_FALSE if not found, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, GetSymbolByName, (
    IN  VOID                  *ImageBase,
    IN  CONST CHAR8           *Name,
    OUT IMGLOAD_SYMBOL_INFO   *SymbolInfo
    ))

  /**
    Extract relocation information from image.

    @param[in]  ImageBase   Pointer to image in memory.
    @param[out] RelocInfo   Receives relocation information.

    @return S_OK on success, S_FALSE if no relocations, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, GetRelocInfo, (
    IN  VOID                 *ImageBase,
    OUT IMGLOAD_RELOC_INFO   *RelocInfo
    ))

  /**
    Apply relocations to image loaded at different address.

    @param[in] ImageBase      Pointer to image in memory.
    @param[in] LoadAddress    Actual load address.
    @param[in] PreferredBase  Preferred base from image headers.

    @return S_OK on success, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, ApplyRelocations, (
    IN VOID              *ImageBase,
    IN VIRTUAL_ADDRESS   LoadAddress,
    IN VIRTUAL_ADDRESS   PreferredBase
    ))

  /**
    Get target operating system from image.

    @param[in]  ImageBase      Pointer to image in memory.
    @param[out] TargetSystem   Receives target system type.

    @return S_OK on success, S_FALSE if unknown, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, GetTargetSystem, (
    IN  VOID                    *ImageBase,
    OUT IMGLOAD_TARGET_SYSTEM   *TargetSystem
    ))

  /**
    Get minimum required system version from image.

    @param[in]  ImageBase       Pointer to image in memory.
    @param[out] MinimumVersion  Receives minimum system version.

    @return S_OK on success, S_FALSE if not specified, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, GetMinimumSystemVersion, (
    IN  VOID                     *ImageBase,
    OUT IMGLOAD_SYSTEM_VERSION   *MinimumVersion
    ))

  /**
    Get target subsystem from image.

    @param[in]  ImageBase        Pointer to image in memory.
    @param[out] TargetSubsystem  Receives target subsystem type.

    @return S_OK on success, S_FALSE if unknown, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, GetTargetSubsystem, (
    IN  VOID                       *ImageBase,
    OUT IMGLOAD_TARGET_SUBSYSTEM   *TargetSubsystem
    ))

  /**
    Get minimum required subsystem version from image.

    @param[in]  ImageBase       Pointer to image in memory.
    @param[out] MinimumVersion  Receives minimum subsystem version.

    @return S_OK on success, S_FALSE if not specified, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, GetMinimumSubsystemVersion, (
    IN  VOID                     *ImageBase,
    OUT IMGLOAD_SYSTEM_VERSION   *MinimumVersion
    ))

ANX_END_INTERFACE(IImageLoader)

//
// Loader Registration
//

/**
  Register an image loader.

  @param[in] Loader  Pointer to loader instance.

  @return S_OK on success, error code otherwise.
**/
HRESULT
ImageLoaderRegister (
  IN IImageLoader  *Loader
  );

/**
  Find loader for image format.

  @param[in] ImageBase  Pointer to image in memory.
  @param[in] ImageSize  Size of image.

  @return Pointer to loader instance, or NULL if format not recognized.
**/
IImageLoader *
ImageLoaderFind (
  IN VOID   *ImageBase,
  IN UINTN  ImageSize
  );

/**
  Load image using appropriate loader.

  @param[in,out] Context  Load context with image information.

  @return S_OK on success, error code otherwise.
**/
HRESULT
ImageLoad (
  IN OUT IMGLOAD_CONTEXT  *Context
  );

//
// Specific Loader Instances
//

extern IImageLoader gFatElfLoader;
extern IImageLoader gElfLoader;
extern IImageLoader gEcoffLoader;
extern IImageLoader gMachoLoader;
extern IImageLoader gPeLoader;
extern IImageLoader gPefLoader;
extern IImageLoader gLeLoader;
extern IImageLoader gNlmLoader;
extern IImageLoader gXcoffLoader;
extern IImageLoader gAoutLoader;
extern IImageLoader gCoffLoader;
extern IImageLoader gXenixLoader;
extern IImageLoader gHunkLoader;
extern IImageLoader gAtariLoader;
extern IImageLoader gPlan9Loader;
extern IImageLoader gVmsLoader;
extern IImageLoader gSomLoader;
extern IImageLoader gPdp10Loader;
extern IImageLoader gTeLoader;

//
// Initialization
//

/**
  Initialize all image loaders.
**/
VOID
ImageLoadersInit (
  VOID
  );
