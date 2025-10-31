/** @file
  Generic Image Resource COM Helper

  Internal header for resource COM implementation helpers.

  Provides unified resource loading for all executable formats:
  - Formats WITH native resources (PE, Mach-O): Look for AUR resource within native format
  - Formats WITHOUT native resources (ELF, COFF, a.out): Look for .rsrc section
  - Merges multiple AUR sub-resources if present

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

#include <apxh/imgload.h>
#include <ananke/resource.h>

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
  Universal resource extraction strategies.
**/
typedef enum _RESOURCE_STRATEGY {
  ResourceStrategyDirect,      ///< Direct .rsrc section (ELF, COFF, a.out)
  ResourceStrategyNativeAUR,   ///< AUR embedded in native resources (PE, Mach-O)
  ResourceStrategyBoth         ///< Try native first, fallback to direct
} RESOURCE_STRATEGY;

/**
  Native resource lookup function.

  Format-specific function to find a resource by type and ID/name within
  the native resource system (e.g., PE resource directory, Mach-O __RSRC).

  @param[in]  ImageBase    Pointer to image.
  @param[in]  TypeCode     Resource type (4-char code or native type).
  @param[in]  Id           Resource ID (0 if using name).
  @param[in]  Name         Resource name (NULL if using ID).
  @param[out] Data         Receives pointer to resource data.
  @param[out] Size         Receives size of resource data.

  @return S_OK if found, S_FALSE if not found, error code otherwise.
**/
typedef HRESULT (*FindNativeResourceFunc)(
  IN  VOID         *ImageBase,
  IN  UINT32       TypeCode,
  IN  UINT32       Id,
  IN  CONST CHAR8  *Name,
  OUT VOID         **Data,
  OUT UINT64       *Size
  );

/**
  Section lookup function.

  Format-specific function to find a section/segment by name.

  @param[in]  ImageBase    Pointer to image.
  @param[in]  Name         Section name (".rsrc", "__RSRC", etc.).
  @param[out] Data         Receives pointer to section data.
  @param[out] Size         Receives size of section.

  @return S_OK if found, S_FALSE if not found, error code otherwise.
**/
typedef HRESULT (*FindSectionFunc)(
  IN  VOID         *ImageBase,
  IN  CONST CHAR8  *Name,
  OUT VOID         **Data,
  OUT UINT64       *Size
  );

/**
  Find universal resource fork in image.

  Unified helper that handles both native-embedded and direct section approaches:
  1. If format has native resources: Look for AUR resource(s) and merge if multiple
  2. Otherwise: Look for .rsrc section containing resource fork directly
  3. Validate resulting resource fork

  @param[in]  ImageBase          Pointer to image.
  @param[in]  Strategy           Resource extraction strategy.
  @param[in]  FindNativeFunc     Native resource lookup (NULL if not applicable).
  @param[in]  FindSectionFunc    Section lookup function.
  @param[in]  SectionName        Section name (".rsrc", "__RSRC", etc.).
  @param[out] ResourceFork       Receives pointer to resource fork (may need free if merged).
  @param[out] Size               Receives size of resource fork.
  @param[out] NeedsFree          Receives TRUE if caller must free ResourceFork.

  @return S_OK if found, S_FALSE if not found, error code otherwise.
**/
HRESULT
FindUniversalResourceFork (
  IN  VOID                   *ImageBase,
  IN  RESOURCE_STRATEGY      Strategy,
  IN  FindNativeResourceFunc FindNativeFunc,
  IN  FindSectionFunc        FindSectionFunc,
  IN  CONST CHAR8            *SectionName,
  OUT VOID                   **ResourceFork,
  OUT UINT64                 *Size,
  OUT BOOLEAN                *NeedsFree
  );
