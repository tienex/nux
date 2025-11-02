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
    VINIL_STATUS_SUCCESS          = 0,
    VINIL_STATUS_INVALID_ARG      = 1,
    VINIL_STATUS_OUT_OF_MEMORY    = 2,
    VINIL_STATUS_NOT_FOUND        = 3,
    VINIL_STATUS_NOT_IMPLEMENTED  = 4,
} VINIL_STATUS;

//
// Forward Declarations
//

typedef struct _VINIL_MEMORY_POOL VINIL_MEMORY_POOL;

//
// Precision Qualifiers (from IL)
//

typedef enum _VINIL_PRECISION {
    VINIL_PRECISION_UNDEFINED,  /* No precision (e.g., void, bool) */
    VINIL_PRECISION_LOW,
    VINIL_PRECISION_MEDIUM,
    VINIL_PRECISION_HIGH,
} VINIL_PRECISION;

//
// Address Space Qualifiers (for compute)
//

typedef enum _VINIL_ADDRESS_SPACE {
    VINIL_ADDRESS_SPACE_PRIVATE,     /* Per work-item, registers/stack */
    VINIL_ADDRESS_SPACE_GLOBAL,      /* All work-items, main memory */
    VINIL_ADDRESS_SPACE_LOCAL,       /* Work-group shared memory */
    VINIL_ADDRESS_SPACE_CONSTANT,    /* Read-only global memory */
} VINIL_ADDRESS_SPACE;
