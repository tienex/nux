/** @file
  Classic Macintosh Resource Fork Library

  Provides platform-neutral resource reading based on Classic Mac
  resource fork format (big-endian). Resources identified by 4-character
  type code and 16-bit ID or name string.

  Resource Fork Structure:
  - Header (16 bytes): offsets and lengths
  - Data Area: Raw resource data with length prefixes
  - Resource Map: Type list → Reference lists → Name list

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

#include <ananke/base.h>
#include <ananke/hresult.h>

/**
  Validate resource fork structure.

  @param[in] Data  Pointer to resource fork data.
  @param[in] Size  Size of resource fork data in bytes.

  @retval S_OK                Valid resource fork.
  @retval E_INVALIDARG        Invalid parameters.
  @retval E_FAIL              Invalid resource fork structure.
**/
HRESULT
AnxResourceValidate (
  IN CONST VOID  *Data,
  IN UINT64      Size
  );

/**
  Find resource by type and ID.

  @param[in]  Fork      Pointer to resource fork data.
  @param[in]  Type      4-character type code (e.g., 'ICON', 'TEXT').
  @param[in]  Id        Resource ID.
  @param[out] Data      Receives pointer to resource data.
  @param[out] Size      Receives size of resource data in bytes.

  @retval S_OK         Resource found.
  @retval S_FALSE      Resource not found.
  @retval E_POINTER    NULL pointer parameter.
**/
HRESULT
AnxResourceFindById (
  IN  CONST VOID  *Fork,
  IN  UINT32      Type,
  IN  UINT16      Id,
  OUT VOID        **Data,
  OUT UINT64      *Size
  );

/**
  Find resource by type and name.

  @param[in]  Fork      Pointer to resource fork data.
  @param[in]  Type      4-character type code.
  @param[in]  Name      Resource name (null-terminated).
  @param[out] Data      Receives pointer to resource data.
  @param[out] Size      Receives size of resource data in bytes.

  @retval S_OK         Resource found.
  @retval S_FALSE      Resource not found.
  @retval E_POINTER    NULL pointer parameter.
**/
HRESULT
AnxResourceFindByName (
  IN  CONST VOID   *Fork,
  IN  UINT32       Type,
  IN  CONST CHAR8  *Name,
  OUT VOID         **Data,
  OUT UINT64       *Size
  );

/**
  Count resources of a given type.

  @param[in]  Fork   Pointer to resource fork data.
  @param[in]  Type   4-character type code (0 for all types).
  @param[out] Count  Receives number of resources.

  @retval S_OK         Success.
  @retval E_POINTER    NULL pointer parameter.
**/
HRESULT
AnxResourceCount (
  IN  CONST VOID  *Fork,
  IN  UINT32      Type,
  OUT UINT32      *Count
  );

/**
  Get resource info by type and index.

  @param[in]  Fork   Pointer to resource fork data.
  @param[in]  Type   4-character type code.
  @param[in]  Index  Zero-based index within type.
  @param[out] Id     Receives resource ID.
  @param[out] Name   Receives pointer to resource name (NULL if unnamed).
  @param[out] Data   Receives pointer to resource data.
  @param[out] Size   Receives size of resource data.

  @retval S_OK         Success.
  @retval S_FALSE      Index out of range.
  @retval E_POINTER    NULL pointer parameter.
**/
HRESULT
AnxResourceGetByIndex (
  IN  CONST VOID   *Fork,
  IN  UINT32       Type,
  IN  UINT32       Index,
  OUT UINT16       *Id,
  OUT CONST CHAR8  **Name,
  OUT VOID         **Data,
  OUT UINT64       *Size
  );

/**
  Enumerate all resource types in fork.

  @param[in]  Fork       Pointer to resource fork data.
  @param[in]  Index      Zero-based type index.
  @param[out] Type       Receives 4-character type code.
  @param[out] Count      Receives number of resources of this type.

  @retval S_OK         Success.
  @retval S_FALSE      Index out of range (no more types).
  @retval E_POINTER    NULL pointer parameter.
**/
HRESULT
AnxResourceEnumTypes (
  IN  CONST VOID  *Fork,
  IN  UINT32      Index,
  OUT UINT32      *Type,
  OUT UINT32      *Count
  );

/**
  Merge multiple resource forks into one.

  Combines resources from multiple forks, handling conflicts by:
  - Taking first occurrence for duplicate type/ID combinations
  - Preserving all unique resources
  - Maintaining type organization

  @param[in]  Forks      Array of pointers to resource fork data.
  @param[in]  Count      Number of forks to merge.
  @param[out] Merged     Receives pointer to merged fork (caller must free).
  @param[out] Size       Receives size of merged fork.

  @retval S_OK           Success.
  @retval E_INVALIDARG   Invalid parameters.
  @retval E_OUTOFMEMORY  Memory allocation failed.
  @retval E_POINTER      NULL pointer parameter.
**/
HRESULT
AnxResourceMerge (
  IN  CONST VOID  **Forks,
  IN  UINT32      Count,
  OUT VOID        **Merged,
  OUT UINT64      *Size
  );

//
// Type Code Helpers
//

/**
  Make 4-character type code from string.

  @param[in] String  4-character string (not null-terminated required).

  @return Type code value.
**/
static INLINE UINT32
ANX_MAKE_TYPE (
  IN CONST CHAR8  *String
  )
{
  return ((UINT32)String[0] << 24) |
         ((UINT32)String[1] << 16) |
         ((UINT32)String[2] << 8) |
         ((UINT32)String[3]);
}

/**
  Convert type code to 4-character string.

  @param[in]  Type    Type code value.
  @param[out] String  Buffer for 4-character string (5 bytes for null term).
**/
static INLINE VOID
ANX_TYPE_TO_STRING (
  IN  UINT32  Type,
  OUT CHAR8   *String
  )
{
  String[0] = (CHAR8)((Type >> 24) & 0xFF);
  String[1] = (CHAR8)((Type >> 16) & 0xFF);
  String[2] = (CHAR8)((Type >> 8) & 0xFF);
  String[3] = (CHAR8)(Type & 0xFF);
  String[4] = '\0';
}

//
// APXH Universal Resource (AUR) Type Codes
//
// When embedding universal resource fork within native format resources:
// - Name-based: "AUR" (APXH Universal Resource)
// - 32-bit ID:  "AUR " (with space, for 32-bit resource systems)
// - 16-bit ID:  "Au" (for 16-bit resource systems like OS/2)
// - 64-bit ID:  "APXHURSC" (APXH Universal Resource Container, for extended systems)
//

#define ANX_RSRC_TYPE_AUR        ANX_MAKE_TYPE("AUR ")  ///< Universal resource (name/32-bit)
#define ANX_RSRC_TYPE_AUR_16BIT  ANX_MAKE_TYPE("Au\0\0") ///< Universal resource (16-bit)
#define ANX_RSRC_TYPE_AUR_64BIT  ANX_MAKE_TYPE("APXH")  ///< Universal resource (64-bit prefix)

#define ANX_RSRC_ID_AUR_16BIT    0x4175  ///< "Au" as 16-bit ID
#define ANX_RSRC_ID_AUR_32BIT    0x41555220  ///< "AUR " as 32-bit ID
#define ANX_RSRC_NAME_AUR        "AUR"   ///< Universal resource name

//
// Common Type Codes
//

#define ANX_RSRC_TYPE_ICON  ANX_MAKE_TYPE("ICON")  ///< Icon
#define ANX_RSRC_TYPE_TEXT  ANX_MAKE_TYPE("TEXT")  ///< Text
#define ANX_RSRC_TYPE_STR   ANX_MAKE_TYPE("STR ")  ///< String
#define ANX_RSRC_TYPE_PICT  ANX_MAKE_TYPE("PICT")  ///< Picture
#define ANX_RSRC_TYPE_SND   ANX_MAKE_TYPE("snd ")  ///< Sound
#define ANX_RSRC_TYPE_CODE  ANX_MAKE_TYPE("CODE")  ///< Code segment
#define ANX_RSRC_TYPE_DATA  ANX_MAKE_TYPE("DATA")  ///< Data
#define ANX_RSRC_TYPE_CURS  ANX_MAKE_TYPE("CURS")  ///< Cursor
#define ANX_RSRC_TYPE_MENU  ANX_MAKE_TYPE("MENU")  ///< Menu
#define ANX_RSRC_TYPE_WIND  ANX_MAKE_TYPE("WIND")  ///< Window
#define ANX_RSRC_TYPE_DLOG  ANX_MAKE_TYPE("DLOG")  ///< Dialog
#define ANX_RSRC_TYPE_DITL  ANX_MAKE_TYPE("DITL")  ///< Dialog item list
#define ANX_RSRC_TYPE_VERS  ANX_MAKE_TYPE("vers")  ///< Version

#ifdef ANX_RESOURCE_WRITE

//
// Resource Fork Writing (Optional - requires ANX_RESOURCE_WRITE)
//

/**
  Resource fork builder context.
**/
typedef struct _ANX_RESOURCE_BUILDER ANX_RESOURCE_BUILDER;

/**
  Create new resource fork builder.

  @param[out] Builder  Receives builder context.

  @retval S_OK           Success.
  @retval E_OUTOFMEMORY  Memory allocation failed.
  @retval E_POINTER      NULL pointer parameter.
**/
HRESULT
AnxResourceBuilderCreate (
  OUT ANX_RESOURCE_BUILDER  **Builder
  );

/**
  Add resource to builder.

  @param[in] Builder  Builder context.
  @param[in] Type     4-character type code.
  @param[in] Id       Resource ID.
  @param[in] Name     Resource name (optional, NULL if unnamed).
  @param[in] Data     Pointer to resource data.
  @param[in] Size     Size of resource data.

  @retval S_OK           Success.
  @retval E_INVALIDARG   Invalid parameters.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
AnxResourceBuilderAdd (
  IN ANX_RESOURCE_BUILDER  *Builder,
  IN UINT32                Type,
  IN UINT16                Id,
  IN CONST CHAR8           *Name,
  IN CONST VOID            *Data,
  IN UINT64                Size
  );

/**
  Build resource fork from added resources.

  @param[in]  Builder  Builder context.
  @param[out] Fork     Receives pointer to resource fork data (caller must free).
  @param[out] Size     Receives size of resource fork data.

  @retval S_OK           Success.
  @retval E_OUTOFMEMORY  Memory allocation failed.
  @retval E_POINTER      NULL pointer parameter.
**/
HRESULT
AnxResourceBuilderBuild (
  IN  ANX_RESOURCE_BUILDER  *Builder,
  OUT VOID                  **Fork,
  OUT UINT64                *Size
  );

/**
  Destroy resource fork builder.

  @param[in] Builder  Builder context.
**/
VOID
AnxResourceBuilderDestroy (
  IN ANX_RESOURCE_BUILDER  *Builder
  );

#endif // ANX_RESOURCE_WRITE
