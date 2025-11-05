/** @file
  VINIL Utility Functions

  Version information and backend capability queries.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#include <vinil/vinil.h>

//
// Utility Functions
//

HRESULT
VinilGetVersion (
  UINT32  *Major,
  UINT32  *Minor,
  UINT32  *Patch
  )
{
  if (Major == NULL || Minor == NULL || Patch == NULL) {
    return E_POINTER;
  }

  *Major = VINIL_VERSION_MAJOR;
  *Minor = VINIL_VERSION_MINOR;
  *Patch = VINIL_VERSION_PATCH;

  return S_OK;
}

HRESULT
VinilGetSupportedBackends (
  UINT32  *Backends
  )
{
  if (Backends == NULL) {
    return E_POINTER;
  }

  *Backends = (1 << VinilBackendInterpreter);  /* Interpreter always available */

  /* Check for JIT support */
#if defined(__x86_64__) || defined(__i386__) || defined(__aarch64__) || defined(__arm__)
  *Backends |= (1 << VinilBackendJit);
#endif

  return S_OK;
}
