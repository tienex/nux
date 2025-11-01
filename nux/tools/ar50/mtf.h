/** @file
  Move-To-Front (MTF) Encoding Header

  Move-To-Front is a transform that exploits data locality by maintaining
  a list of symbols and moving each encoded symbol to the front. This tends
  to produce many small values which compress well with range encoding.

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __mtf_h__
#define __mtf_h__

#include "types.h"
#include <stddef.h>

/**
  Apply Move-To-Front encoding.

  Transforms data by encoding each byte as its position in a dynamically
  maintained list, then moving it to the front.

  @param[in]  pInput      Input data buffer.
  @param[in]  InputSize   Size of input data.
  @param[out] pOutput     Output buffer for MTF encoded data.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
MTFEncode (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput
  );

/**
  Apply inverse Move-To-Front decoding.

  Reconstructs original data from MTF encoded data.

  @param[in]  pInput      MTF encoded data.
  @param[in]  InputSize   Size of encoded data.
  @param[out] pOutput     Output buffer for decoded data.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
MTFDecode (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput
  );

#endif /* __mtf_h__ */
