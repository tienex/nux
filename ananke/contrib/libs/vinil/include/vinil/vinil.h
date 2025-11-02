/** @file
  VINIL - Vincent Intermediate Language Unified Library

  COM-based API for unified execution engine supporting graphics and compute
  workloads across OpenGL ES, OpenCL, CUDA, HIP, and SYCL.

  Copyright (C) 2003-2007 Hans-Martin Will
  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#ifndef __vinil_vinil_h__
#define __vinil_vinil_h__ 1

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>

//
// Version Information
//

#define VINIL_VERSION_MAJOR     0
#define VINIL_VERSION_MINOR     2
#define VINIL_VERSION_PATCH     0

//
// Compilation Flags
//

typedef enum _VINIL_COMPILE_FLAGS {
  VinilCompileFlagNone        = 0,
  VinilCompileFlagUseJit      = (1 << 0),  /* Use JIT compiler */
  VinilCompileFlagOptimize    = (1 << 1),  /* Enable optimizations */
  VinilCompileFlagDebugInfo   = (1 << 2),  /* Include debug information */
} VINIL_COMPILE_FLAGS;

//
// Execution Modes
//

typedef enum _VINIL_EXEC_MODE {
  VinilExecModeGraphics   = 0,  /* Graphics shader execution */
  VinilExecModeCompute    = 1,  /* Compute kernel execution */
} VINIL_EXEC_MODE;

/* --------------------------------------------------------------- */
/*  IVinilContext - Main execution context                         */
/* --------------------------------------------------------------- */

// {VINIL001-0000-0000-C000-000000000046}
#define ANX_IID_IVinilContext "VINIL001-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IVinilContext,
    0xFFFFFF01, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IVinilContext, IUnknown,
    IID_IVinilContext, ANX_IID_IVinilContext)

    /**
      Create a new IL program.

      @param[out]  Program  Receives the new program interface.

      @retval  S_OK               Success.
      @retval  E_OUTOFMEMORY      Out of memory.
    **/
    ANX_IFACE_METHOD(HRESULT, CreateProgram, (
        OUT struct IVinilProgram **Program))

    /**
      Get the last error message.

      @param[out]  ErrorMessage  Receives error message string.

      @retval  S_OK  Success.
    **/
    ANX_IFACE_METHOD(HRESULT, GetLastError, (
        OUT CONST CHAR8 **ErrorMessage))

    /**
      Get library version information.

      @param[out]  Major  Major version number.
      @param[out]  Minor  Minor version number.
      @param[out]  Patch  Patch version number.

      @retval  S_OK  Success.
    **/
    ANX_IFACE_METHOD(HRESULT, GetVersion, (
        OUT UINT32 *Major,
        OUT UINT32 *Minor,
        OUT UINT32 *Patch))

ANX_END_INTERFACE(IVinilContext)

/* --------------------------------------------------------------- */
/*  IVinilProgram - IL program builder                             */
/* --------------------------------------------------------------- */

// {VINIL002-0000-0000-C000-000000000046}
#define ANX_IID_IVinilProgram "VINIL002-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IVinilProgram,
    0xFFFFFF02, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IVinilProgram, IUnknown,
    IID_IVinilProgram, ANX_IID_IVinilProgram)

    /**
      Compile the IL program to executable code.

      @param[in]   Flags       Compilation flags.
      @param[out]  Executable  Receives the compiled executable.

      @retval  S_OK               Success.
      @retval  E_FAIL             Compilation failed.
      @retval  E_OUTOFMEMORY      Out of memory.
    **/
    ANX_IFACE_METHOD(HRESULT, Compile, (
        IN VINIL_COMPILE_FLAGS Flags,
        OUT struct IVinilExecutable **Executable))

    /**
      Get compilation log/errors.

      @param[out]  Log  Receives compilation log string.

      @retval  S_OK  Success.
    **/
    ANX_IFACE_METHOD(HRESULT, GetCompileLog, (
        OUT CONST CHAR8 **Log))

ANX_END_INTERFACE(IVinilProgram)

/* --------------------------------------------------------------- */
/*  IVinilExecutable - Compiled executable                         */
/* --------------------------------------------------------------- */

// {VINIL003-0000-0000-C000-000000000046}
#define ANX_IID_IVinilExecutable "VINIL003-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IVinilExecutable,
    0xFFFFFF03, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IVinilExecutable, IUnknown,
    IID_IVinilExecutable, ANX_IID_IVinilExecutable)

    /**
      Execute the compiled code.

      @param[in]  Mode      Execution mode (graphics or compute).
      @param[in]  UserData  User-provided execution context.

      @retval  S_OK       Success.
      @retval  E_FAIL     Execution failed.
      @retval  E_NOTIMPL  Not implemented.
    **/
    ANX_IFACE_METHOD(HRESULT, Execute, (
        IN VINIL_EXEC_MODE Mode,
        IN VOID *UserData))

    /**
      Get execution statistics.

      @param[out]  CyclesExecuted  Number of cycles executed.

      @retval  S_OK  Success.
    **/
    ANX_IFACE_METHOD(HRESULT, GetStats, (
        OUT UINT64 *CyclesExecuted))

ANX_END_INTERFACE(IVinilExecutable)

/* --------------------------------------------------------------- */
/*  Factory Function                                                */
/* --------------------------------------------------------------- */

/**
  Create the main VINIL context.

  @param[out]  Context  Receives the context interface.

  @retval  S_OK               Success.
  @retval  E_OUTOFMEMORY      Out of memory.
  @retval  E_POINTER          Context is NULL.
**/
HRESULT
VinilCreateContext (
  OUT IVinilContext **Context
  );

#endif // __vinil_vinil_h__
