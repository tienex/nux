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
// Error Codes
//

typedef enum _VINIL_ERROR {
    VINIL_SUCCESS           = 0,
    VINIL_ERROR_INVALID     = 1,
    VINIL_ERROR_NOMEM       = 2,
    VINIL_ERROR_NOTFOUND    = 3,
    VINIL_ERROR_NOTIMPL     = 4,
} VINIL_ERROR;

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
    VINIL_ADDR_PRIVATE,     /* Per work-item, registers/stack */
    VINIL_ADDR_GLOBAL,      /* All work-items, main memory */
    VINIL_ADDR_LOCAL,       /* Work-group shared memory */
    VINIL_ADDR_CONSTANT,    /* Read-only global memory */
} VINIL_ADDRESS_SPACE;
