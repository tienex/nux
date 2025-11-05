/** @file
  VINIL SPIR-V Loader

  Loads SPIR-V binary modules and converts to VINIL IL.
  Supports Vulkan and OpenCL SPIR-V flavors.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#pragma once
#include <ananke/types.h>
#include <ananke/hresult.h>

//
// SPIR-V Magic Number
//

#define VINIL_SPIRV_MAGIC       0x07230203

//
// Execution Models
//

typedef enum _VINIL_SPIRV_EXEC_MODEL {
    VinilSpvVertex                  = 0,
    VinilSpvTessellationControl     = 1,
    VinilSpvTessellationEvaluation  = 2,
    VinilSpvGeometry                = 3,
    VinilSpvFragment                = 4,
    VinilSpvGLCompute               = 5,
    VinilSpvKernel                  = 6,  /* OpenCL */
} VINIL_SPIRV_EXEC_MODEL;

//
// Loader Options
//

typedef enum _VINIL_SPIRV_FLAGS {
    VinilSpvNone            = 0,
    VinilSpvOptimize        = (1 << 0),  /* Optimize during load */
    VinilSpvValidate        = (1 << 1),  /* Validate SPIR-V */
    VinilSpvDebugInfo       = (1 << 2),  /* Preserve debug info */
} VINIL_SPIRV_FLAGS;

//
// Error Information
//

typedef struct _VINIL_SPIRV_ERROR {
    UINT32      Instruction;    /* Instruction offset */
    CONST CHAR8 *Message;       /* Error message */
} VINIL_SPIRV_ERROR;

//
// Loader Functions
//

/**
  Load SPIR-V binary and convert to VINIL IL.

  @param[in]   SpirVData   SPIR-V binary data.
  @param[in]   DataSize    Size of SPIR-V data in bytes.
  @param[in]   Flags       Loader flags.
  @param[out]  Program     Converted IL program.
  @param[out]  Error       Error information (optional).

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Load or conversion failed.
**/
HRESULT
VinilLoadSPIRV (
    CONST UINT32        *SpirVData,
    UINTN               DataSize,
    VINIL_SPIRV_FLAGS   Flags,
    VOID                **Program,
    VINIL_SPIRV_ERROR   *Error
    );

/**
  Load SPIR-V file and convert to VINIL IL.

  @param[in]   FilePath  Path to SPIR-V binary file.
  @param[in]   Flags     Loader flags.
  @param[out]  Program   Converted IL program.
  @param[out]  Error     Error information (optional).

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Load, I/O, or conversion failed.
**/
HRESULT
VinilLoadSPIRVFile (
    CONST CHAR8         *FilePath,
    VINIL_SPIRV_FLAGS   Flags,
    VOID                **Program,
    VINIL_SPIRV_ERROR   *Error
    );

/**
  Validate SPIR-V binary.

  @param[in]   SpirVData  SPIR-V binary data.
  @param[in]   DataSize   Size of SPIR-V data in bytes.
  @param[out]  Error      Error information (optional).

  @retval  S_OK   Valid SPIR-V.
  @retval  E_FAIL Invalid SPIR-V.
**/
HRESULT
VinilValidateSPIRV (
    CONST UINT32        *SpirVData,
    UINTN               DataSize,
    VINIL_SPIRV_ERROR   *Error
    );

/**
  Get execution model from SPIR-V module.

  @param[in]   SpirVData  SPIR-V binary data.
  @param[in]   DataSize   Size of SPIR-V data in bytes.
  @param[out]  Model      Execution model.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Failed to determine model.
**/
HRESULT
VinilGetSPIRVExecutionModel (
    CONST UINT32            *SpirVData,
    UINTN                   DataSize,
    VINIL_SPIRV_EXEC_MODEL  *Model
    );

