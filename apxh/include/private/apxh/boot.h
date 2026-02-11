/** @file
  APXH Boot Information API

  Boot information management and architecture utilities.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

#include <apxh/types.h>

//
// Initialization
//

VOID Initialize (VOID);

//
// Architecture Utilities
//

CONST CHAR *GetArchName (IN ARCH Arch);
ARCH GetKernelArchitecture (VOID);
ARCH GetUserArchitecture (VOID);
ARCH GetHostArchitecture (VOID);
UINT32 GetMixedModeFlags (VOID);
IMGLOAD_ENDIAN GetKernelEndianness (VOID);
IMGLOAD_ENDIAN GetUserEndianness (VOID);
BOOLEAN GetMixedEndianMode (VOID);

//
// Image Loading
//

ARCH GetImageArch (IN VOID *ImageBase);
IMGLOAD_ENDIAN GetImageEndian (IN VOID *ImageBase);
VIRTUAL_ADDRESS LoadExecutable (IN VOID *ImageBase, IN BOOLEAN IsUserMode);

//
// Boot Memory Management
//

UINT32 CheckPayloadPage (IN UINT32 Addr);

//
// Boot Information Initialization
//

VOID ImageLoadersInit (VOID);
