/** @file
  Generic Image Resource COM Helper

  Internal header for resource COM implementation helpers.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

#include <apxh/imgload.h>

/**
  Create IImageResource from resource fork entry.

  @param[in]  ResourceFork  Pointer to Classic Mac resource fork data.
  @param[in]  TypeCode      4-character type code.
  @param[in]  ResourceId    Resource ID.
  @param[in]  Name          Resource name (optional, NULL if ID-based).
  @param[out] Resource      Receives IImageResource interface.

  @return S_OK on success, error code otherwise.
**/
HRESULT
CreateImageResource (
  IN  CONST VOID       *ResourceFork,
  IN  UINT32           TypeCode,
  IN  UINT16           ResourceId,
  IN  CONST CHAR8      *Name,
  OUT IImageResource   **Resource
  );

/**
  Create IEnumImageResource for resource fork.

  @param[in]  ResourceFork  Pointer to Classic Mac resource fork data.
  @param[in]  TypeCode      4-character type code (0 for all types).
  @param[out] Enumerator    Receives IEnumImageResource interface.

  @return S_OK on success, error code otherwise.
**/
HRESULT
CreateImageResourceEnumerator (
  IN  CONST VOID          *ResourceFork,
  IN  UINT32              TypeCode,
  OUT IEnumImageResource  **Enumerator
  );

/**
  Find resource fork in image by section/segment name.

  Helper for locating .rsrc or __RSRC sections in executable formats.

  @param[in]  ImageBase        Pointer to image.
  @param[in]  FindSectionFunc  Format-specific function to find section by name.
  @param[in]  SectionName      Section name (".rsrc", "__RSRC", etc.).
  @param[out] ResourceFork     Receives pointer to resource fork data.
  @param[out] Size             Receives size of resource fork.

  @return S_OK if found, S_FALSE if not found, error code otherwise.
**/
typedef HRESULT (*FindSectionByNameFunc)(
  IN  VOID         *ImageBase,
  IN  CONST CHAR8  *Name,
  OUT VOID         **Data,
  OUT UINT64       *Size
  );
