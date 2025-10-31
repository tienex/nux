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

static IArchitecture *gArchitectures[MAX_ARCHITECTURES];
static ARCH gArchTypes[MAX_ARCHITECTURES];
static UINT32 gNumArchitectures = 0;

//
// Current active architecture
//

static IArchitecture *gCurrentArchitecture = NULL;

/**
  Register an architecture handler.

  @param[in] Architecture  Pointer to architecture instance.
  @param[in] ArchType      Architecture type (Arch386, ArchAmd64, etc).

  @return S_OK on success, error code otherwise.
**/
HRESULT
ArchitectureRegister (
  IN IArchitecture  *Architecture,
  IN ARCH           ArchType
  )
{
  HRESULT Status;
  VOID *TestInterface = NULL;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  if (gNumArchitectures >= MAX_ARCHITECTURES) {
    fatal("Too many architectures registered (max %d)", MAX_ARCHITECTURES);
    return E_OUTOFMEMORY;
  }

  // Verify architecture implements IArchitecture
  Status = Architecture->lpVtbl->QueryInterface(Architecture, &IID_IArchitecture, &TestInterface);
  if (FAILED(Status) || TestInterface == NULL) {
    warn("Architecture does not implement IArchitecture interface");
    return E_NOINTERFACE;
  }

  gArchitectures[gNumArchitectures] = Architecture;
  gArchTypes[gNumArchitectures] = ArchType;
  gNumArchitectures++;

  info("Registered architecture #%d: %s", gNumArchitectures, ArchitectureGetName(ArchType));

  return S_OK;
}

/**
  Get architecture handler for specified architecture.

  @param[in] Arch  Architecture type.

  @return Pointer to architecture instance, or NULL if not found.
**/
IArchitecture *
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
    warn("Architecture %s not registered", ArchitectureGetName(Arch));
    return E_NOTFOUND;
  }

  return S_OK;
}

/**
  Get current active architecture.

  @return Pointer to current architecture instance, or NULL if none set.
**/
IArchitecture *
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
    default:
      return "unknown";
    }
}

/**
  Initialize all architecture handlers.
**/
VOID
ArchitecturesInit (
  VOID
  )
{
  info("Initializing architecture handlers...");

  // Register all supported architectures
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
  ArchitectureRegister(&gPaeArch, Arch386);
  ArchitectureRegister(&gPae64Arch, ArchAmd64);
#endif

#if EC_MACHINE_RISCV64
  ArchitectureRegister(&gSv48Arch, ArchRiscV64);
#endif

  info("Architecture handlers initialized (%d handlers)", gNumArchitectures);
}
