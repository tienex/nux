/** @file
  VINIL Base Types

  Basic type definitions used throughout VINIL.
  Clean, simple types without legacy overhead.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#pragma once

#include <ananke/types.h>
#include <stddef.h>
#include <stdint.h>

//
// Status Codes (NT Style)
//

typedef enum _VINIL_STATUS {
    VinilStatusSuccess         = 0,
    VinilStatusInvalidArg      = 1,
    VinilStatusOutOfMemory     = 2,
    VinilStatusNotFound        = 3,
    VinilStatusNotImplemented  = 4,
} VINIL_STATUS;

//
// Forward Declarations
//

typedef struct _VINIL_MEMORY_POOL VINIL_MEMORY_POOL;

//
// Precision Qualifiers (from IL)
//

typedef enum _VINIL_PRECISION {
    VinilPrecisionUndefined,  /* No precision (e.g., void, bool) */
    VinilPrecisionLow,
    VinilPrecisionMedium,
    VinilPrecisionHigh,
} VINIL_PRECISION;

//
// Address Space Qualifiers (for compute)
//

typedef enum _VINIL_ADDRESS_SPACE {
    VinilAddressSpacePrivate,   /* Per work-item, registers/stack */
    VinilAddressSpaceGlobal,    /* All work-items, main memory */
    VinilAddressSpaceLocal,     /* Work-group shared memory */
    VinilAddressSpaceConstant,  /* Read-only global memory */
} VINIL_ADDRESS_SPACE;
