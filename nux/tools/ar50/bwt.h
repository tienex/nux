/** @file
  Burrows-Wheeler Transform Header

  The Burrows-Wheeler Transform (BWT) rearranges data to make it more
  compressible by grouping similar characters together. It's commonly
  used as a preprocessing step before compression.

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __bwt_h__
#define __bwt_h__

#include "types.h"
#include <stddef.h>

/**
  Apply Burrows-Wheeler Transform to data.

  Transforms input data by creating rotations and sorting them.
  Returns the transformed data and the original index.

  @param[in]  pInput      Input data buffer.
  @param[in]  InputSize   Size of input data.
  @param[out] pOutput     Output buffer for transformed data.
  @param[out] pOrigIndex  Original rotation index (for inverse transform).

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
BWTTransform (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput,
  OUT UINT32       *pOrigIndex
  );

/**
  Apply inverse Burrows-Wheeler Transform.

  Reconstructs original data from BWT transformed data.

  @param[in]  pInput      BWT transformed data.
  @param[in]  InputSize   Size of transformed data.
  @param[in]  OrigIndex   Original rotation index from transform.
  @param[out] pOutput     Output buffer for original data.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
BWTInverse (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  IN  UINT32       OrigIndex,
  OUT UINT8        *pOutput
  );

#endif /* __bwt_h__ */
