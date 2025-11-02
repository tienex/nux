/** @file
  VINIL - Vincent Intermediate Language Unified Library

  Clean, modern COM-based API for unified graphics and compute IL.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#pragma once
#include <ananke/types.h>
#include <ananke/hresult.h>
#include <ananke/com.h>

//
// Version Information
//

#define VINIL_VERSION_MAJOR     1
#define VINIL_VERSION_MINOR     0
#define VINIL_VERSION_PATCH     0

//
// GUIDs
//

ANX_DEFINE_GUID(IID_IVinilContext, 0x12345678, 0x1234, 0x1234, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0);
ANX_DEFINE_GUID(IID_IVinilProgram, 0x23456789, 0x2345, 0x2345, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01);

//
// Forward Declarations
//

typedef struct IVinilContext IVinilContext;
typedef struct IVinilProgram IVinilProgram;

//
// Execution Modes
//

typedef enum _VINIL_EXECUTION_MODE {
    VinilExecutionModeGraphics = 0,
    VinilExecutionModeCompute  = 1,
} VINIL_EXECUTION_MODE;

//
// Execution Backends
//

typedef enum _VINIL_EXECUTION_BACKEND {
    VinilBackendInterpreter = 0,  /* Software interpreter */
    VinilBackendJit         = 1,  /* JIT compiler */
    VinilBackendAot         = 2,  /* Pre-compiled native */
} VINIL_EXECUTION_BACKEND;

//
// IVinilContext Interface
//

ANX_BEGIN_INTERFACE(IVinilContext, IUnknown, IID_IVinilContext, "12345678-1234-1234-1234-56789ABCDEF0")
    /**
      Execute IL program.

      @param[in]  Program  IL program to execute.
      @param[in]  Backend  Execution backend to use.
      @param[in]  Inputs   Input data array.
      @param[out] Outputs  Output data array.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
      @retval  E_FAIL     Execution failed.
    **/
    ANX_IFACE_METHOD(HRESULT, Execute, (IVinilProgram *Program, VINIL_EXECUTION_BACKEND Backend, VOID *Inputs, VOID *Outputs))

    /**
      Execute compute kernel with work-group configuration.

      @param[in]  Program     IL program (kernel).
      @param[in]  Backend     Execution backend to use.
      @param[in]  GlobalSize  Global work size [x, y, z].
      @param[in]  LocalSize   Local work size [x, y, z].
      @param[in]  Args        Kernel arguments.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
      @retval  E_FAIL     Execution failed.
    **/
    ANX_IFACE_METHOD(HRESULT, ExecuteKernel, (IVinilProgram *Program, VINIL_EXECUTION_BACKEND Backend, CONST UINT32 *GlobalSize, CONST UINT32 *LocalSize, VOID *Args))
ANX_END_INTERFACE(IVinilContext, IID_IVinilContext)

//
// IVinilProgram Interface
//

ANX_BEGIN_INTERFACE(IVinilProgram, IUnknown, IID_IVinilProgram, "23456789-2345-2345-2345-6789ABCDEF01")
    /**
      Get program mode (graphics or compute).

      @param[out]  Mode  Program mode.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, GetMode, (VINIL_EXECUTION_MODE *Mode))

    /**
      Get number of instructions in program.

      @param[out]  Count  Instruction count.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, GetInstructionCount, (UINT32 *Count))
ANX_END_INTERFACE(IVinilProgram, IID_IVinilProgram)

//
// Factory Functions
//

/**
  Create execution context.

  @param[out]  Context  Created context interface.

  @retval  S_OK           Success.
  @retval  E_POINTER      Invalid pointer.
  @retval  E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
VinilCreateContext (
    IVinilContext  **Context
    );

//
// Utility Functions
//

/**
  Get VINIL version.

  @param[out]  Major  Major version.
  @param[out]  Minor  Minor version.
  @param[out]  Patch  Patch version.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
**/
HRESULT
VinilGetVersion (
    UINT32  *Major,
    UINT32  *Minor,
    UINT32  *Patch
    );

/**
  Get supported backends.

  @param[out]  Backends  Bitmask of supported backends.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
**/
HRESULT
VinilGetSupportedBackends (
    UINT32  *Backends
    );

