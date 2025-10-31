/** @file
  APXH Architecture Registry

  Manages registration and selection of architecture handlers.
  Provides unified COM-based interface for different CPU architectures.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>

//
// Maximum number of registered architectures
//

#define MAX_ARCHITECTURES 16

//
// Architecture Registry
//

static IVirtualAddressSpace *gArchitectures[MAX_ARCHITECTURES];
static ARCH gArchTypes[MAX_ARCHITECTURES];
static UINT32 gNumArchitectures = 0;

//
// Current active architecture
//

static IVirtualAddressSpace *gCurrentArchitecture = NULL;

/**
  Register an architecture handler.

  @param[in] Architecture  Pointer to architecture instance.
  @param[in] ArchType      Architecture type (Arch386, ArchAmd64, etc).

  @return S_OK on success, error code otherwise.
**/
HRESULT
ArchitectureRegister (
  IN IVirtualAddressSpace  *Architecture,
  IN ARCH           ArchType
  )
{
  HRESULT Status;
  VOID *TestInterface = NULL;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  if (gNumArchitectures >= MAX_ARCHITECTURES) {
    printf("ERROR: Too many architectures registered (max %d)\n", MAX_ARCHITECTURES);
    return E_OUTOFMEMORY;
  }

  // Verify architecture implements IVirtualAddressSpace
  Status = Architecture->lpVtbl->QueryInterface(Architecture, &IID_IVirtualAddressSpace, &TestInterface);
  if (FAILED(Status) || TestInterface == NULL) {
    printf("WARNING: Architecture does not implement IVirtualAddressSpace interface\n");
    return E_NOINTERFACE;
  }

  gArchitectures[gNumArchitectures] = Architecture;
  gArchTypes[gNumArchitectures] = ArchType;
  gNumArchitectures++;

  printf("Registered architecture #%d: %s\n", gNumArchitectures, ArchitectureGetName(ArchType));

  return S_OK;
}

/**
  Get architecture handler for specified architecture.

  @param[in] Arch  Architecture type.

  @return Pointer to architecture instance, or NULL if not found.
**/
IVirtualAddressSpace *
ArchitectureGet (
  IN ARCH  Arch
  )
{
  UINT32 i;

  for (i = 0; i < gNumArchitectures; i++) {
    if (gArchTypes[i] == Arch) {
      return gArchitectures[i];
    }
  }

  return NULL;
}

/**
  Set current active architecture.

  @param[in] Arch  Architecture type to activate.

  @return S_OK on success, error code otherwise.
**/
HRESULT
ArchitectureSetCurrent (
  IN ARCH  Arch
  )
{
  gCurrentArchitecture = ArchitectureGet(Arch);
  if (gCurrentArchitecture == NULL) {
    printf("WARNING: Architecture %s not registered\n", ArchitectureGetName(Arch));
    return E_NOTFOUND;
  }

  return S_OK;
}

/**
  Get current active architecture.

  @return Pointer to current architecture instance, or NULL if none set.
**/
IVirtualAddressSpace *
ArchitectureGetCurrent (
  VOID
  )
{
  return gCurrentArchitecture;
}

/**
  Get architecture name string.

  @param[in] Arch  Architecture type.

  @return Pointer to architecture name string.
**/
CONST CHAR *
ArchitectureGetName (
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
    case ArchRiscV32:
      return "RISCV32";
    case ArchRiscV64:
      return "RISCV64";
    case ArchPpc32:
      return "PowerPC-32";
    case ArchPpc64:
      return "PowerPC-64";
    case ArchArm:
      return "ARM";
    case ArchArm64:
      return "ARM64";
    case ArchMips32:
      return "MIPS-32";
    case ArchMips64:
      return "MIPS-64";
    case ArchAlpha:
      return "Alpha";
    case ArchIa64:
      return "IA-64";
    case ArchAmd64_32:
      return "x32";
    case ArchArm64_32:
      return "ARM64-32";
    case ArchRiscV64_32:
      return "RISCV64-32";
    default:
      return "unknown";
    }
}

/**
  Get the host/native architecture at compile time.

  @return Native architecture type for this build.
**/
ARCH
ArchitectureGetNative (
  VOID
  )
{
#if defined(ANX_ARCH_X86_64)
  return ArchAmd64;
#elif defined(ANX_ARCH_X86)
  return Arch386;
#elif defined(ANX_ARCH_ARM64)
  return ArchArm64;
#elif defined(ANX_ARCH_ARM)
  return ArchArm;
#elif defined(ANX_ARCH_RISCV)
  #if __riscv_xlen == 64
    return ArchRiscV64;
  #else
    return ArchRiscV32;
  #endif
#else
  return ArchUnsupported;
#endif
}

/**
  Check if guest architecture can run on host architecture.

  Implements architecture compatibility similar to XNU (macOS kernel),
  allowing 32-bit executables to run on compatible 64-bit systems.

  Compatibility rules:
  - Exact match always works
  - x86 (32-bit) can run on x64 (64-bit)
  - x32 (ILP32 on x86-64) can run on x64
  - rv32 can run on rv64
  - rv64-32 (ILP32 on RISC-V 64) can run on rv64
  - 32-bit ARM can run on 64-bit ARM (kernel decides)
  - 32-bit PowerPC can run on 64-bit PowerPC (kernel decides)
  - Hybrid modes can run on their 64-bit counterparts

  @param[in] GuestArch  Architecture of the image to load.
  @param[in] HostArch   Architecture of the host system.
  @param[in] IsKernel   TRUE for kernel mode, FALSE for user mode.

  @return TRUE if compatible, FALSE otherwise.
**/
BOOLEAN
ArchitectureIsCompatible (
  IN ARCH     GuestArch,
  IN ARCH     HostArch,
  IN BOOLEAN  IsKernel
  )
{
  // Exact match always works
  if (GuestArch == HostArch) {
    return TRUE;
  }

  // Invalid/unsupported architectures never compatible
  if (GuestArch == ArchInvalid || GuestArch == ArchUnsupported ||
      HostArch == ArchInvalid || HostArch == ArchUnsupported) {
    return FALSE;
  }

  // x86 compatibility on x64
  if (HostArch == ArchAmd64) {
    // x86 32-bit can run on x64 (both kernel and user)
    if (GuestArch == Arch386) {
      return TRUE;
    }
    // x32 (ILP32 on x86-64) can run on x64
    if (GuestArch == ArchAmd64_32) {
      return TRUE;
    }
  }

  // RISC-V compatibility
  if (HostArch == ArchRiscV64) {
    // rv32 can run on rv64 (both kernel and user)
    if (GuestArch == ArchRiscV32) {
      return TRUE;
    }
    // rv64-32 (ILP32 on RISC-V 64) can run on rv64
    if (GuestArch == ArchRiscV64_32) {
      return TRUE;
    }
  }

  // ARM compatibility
  if (HostArch == ArchArm64) {
    // ARM 32-bit can run on ARM64 (both kernel and user)
    if (GuestArch == ArchArm) {
      return TRUE;
    }
    // ARM64-32 (ILP32 on AArch64) can run on ARM64
    if (GuestArch == ArchArm64_32) {
      return TRUE;
    }
  }

  // PowerPC compatibility
  if (HostArch == ArchPpc64) {
    // PPC 32-bit can run on PPC64 (both kernel and user)
    if (GuestArch == ArchPpc32) {
      return TRUE;
    }
    // PPC64-32 (32-bit mode on PPC64) can run on PPC64
    if (GuestArch == ArchPpc64_32) {
      return TRUE;
    }
  }

  // MIPS compatibility
  if (HostArch == ArchMips64) {
    // MIPS 32-bit can run on MIPS64 (both kernel and user)
    if (GuestArch == ArchMips32) {
      return TRUE;
    }
    // MIPS64-32 (n32 ABI) can run on MIPS64
    if (GuestArch == ArchMips64_32) {
      return TRUE;
    }
  }

  // LoongArch compatibility
  if (HostArch == ArchLoongArch64) {
    // LA32 can run on LA64
    if (GuestArch == ArchLoongArch32) {
      return TRUE;
    }
    // LA64-32 (32-bit mode on LA64) can run on LA64
    if (GuestArch == ArchLoongArch64_32) {
      return TRUE;
    }
  }

  // SPARC compatibility
  if (HostArch == ArchSparc64) {
    // SPARC 32-bit can run on SPARC64
    if (GuestArch == ArchSparc32) {
      return TRUE;
    }
    // SPARC64-32 (32-bit mode on SPARC64) can run on SPARC64
    if (GuestArch == ArchSparc64_32) {
      return TRUE;
    }
  }

  // s390x compatibility
  if (HostArch == ArchS390x) {
    // s390 32-bit can run on s390x
    if (GuestArch == ArchS390) {
      return TRUE;
    }
    // s390x-32 (32-bit mode on s390x) can run on s390x
    if (GuestArch == ArchS390x_32) {
      return TRUE;
    }
  }

  // PA-RISC compatibility
  if (HostArch == ArchPaRisc64) {
    // PA-RISC 32-bit can run on PA-RISC 64
    if (GuestArch == ArchPaRisc32) {
      return TRUE;
    }
    // PA-RISC64-32 (32-bit mode on PA-RISC 64) can run on PA-RISC 64
    if (GuestArch == ArchPaRisc64_32) {
      return TRUE;
    }
  }

  // Itanium compatibility
  if (HostArch == ArchIa64) {
    // IA-64 can run x86 code (though rarely used in practice)
    if (GuestArch == Arch386) {
      return TRUE;
    }
    // IA-64-32 (32-bit compatibility mode) can run on IA-64
    if (GuestArch == ArchIa64_32) {
      return TRUE;
    }
  }

  // Alpha compatibility
  if (HostArch == ArchAlpha) {
    // Alpha32 (32-bit mode) can run on Alpha
    if (GuestArch == ArchAlpha32) {
      return TRUE;
    }
  }

  // No compatibility found
  return FALSE;
}

/**
  Initialize all architecture handlers.

  Auto-registers all architectures from the .architectures section.
**/
VOID
ArchitecturesInit (
  VOID
  )
{
  extern CONST ARCHITECTURE_REGISTRATION __start_architectures[];
  extern CONST ARCHITECTURE_REGISTRATION __stop_architectures[];
  CONST ARCHITECTURE_REGISTRATION *Reg;

  printf("Initializing architecture handlers...\n");

  // Auto-register all architectures from .architectures section
  for (Reg = __start_architectures; Reg < __stop_architectures; Reg++) {
    ArchitectureRegister(Reg->Architecture, Reg->ArchType);
  }

  printf("Architecture handlers initialized (%d handlers)\n", gNumArchitectures);
}
