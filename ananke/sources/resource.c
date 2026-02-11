/** @file
  Classic Macintosh Resource Fork Implementation

  Provides universal resource format based on Classic Mac resource fork.
  Can be embedded in any executable format via .rsrc sections, __RSRC
  segments, or adapted to native resource directories.

  Resource Fork Structure:
  - Resource Header (16 bytes): data offset, map offset, data length, map length
  - Resource Data: Raw resource data with length prefixes
  - Resource Map: Type list → Reference lists → Name list

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/resource.h>
#include <ananke/base.h>

// For memcmp, strlen
extern int memcmp(const void *, const void *, unsigned long);
extern unsigned long strlen(const char *);
extern void *memset(void *, int, unsigned long);

//
// Classic Mac Resource Fork Structures (Big Endian)
//

#pragma pack(push, 1)

/**
  Resource Fork Header (16 bytes)
**/
typedef struct _RESOURCE_HEADER {
  UINT32  DataOffset;      ///< Offset from beginning to resource data
  UINT32  MapOffset;       ///< Offset from beginning to resource map
  UINT32  DataLength;      ///< Length of resource data
  UINT32  MapLength;       ///< Length of resource map
} RESOURCE_HEADER;

/**
  Resource Map Header
**/
typedef struct _RESOURCE_MAP_HEADER {
  UINT8   Reserved1[16];   ///< Reserved (copy of header)
  UINT32  Reserved2;       ///< Reserved
  UINT16  Reserved3;       ///< Reserved
  UINT16  Attributes;      ///< Resource fork attributes
  UINT16  TypeListOffset;  ///< Offset from map start to type list
  UINT16  NameListOffset;  ///< Offset from map start to name list
} RESOURCE_MAP_HEADER;

/**
  Resource Type List Entry
**/
typedef struct _RESOURCE_TYPE {
  UINT32  TypeCode;        ///< 4-character type code (e.g., 'ICON', 'TEXT')
  UINT16  Count;           ///< Number of resources of this type minus 1
  UINT16  RefListOffset;   ///< Offset from type list to reference list
} RESOURCE_TYPE;

/**
  Resource Reference Entry
**/
typedef struct _RESOURCE_REFERENCE {
  UINT16  Id;              ///< Resource ID
  UINT16  NameOffset;      ///< Offset from name list to name (-1 if none)
  UINT32  Attributes;      ///< Resource attributes (bits 0-7) + data offset (bits 8-31)
  UINT32  Reserved;        ///< Reserved for handle to resource
} RESOURCE_REFERENCE;

#pragma pack(pop)

//
// Resource Attributes
//

#define RSRC_ATTR_SYSHEAP    0x40  ///< Read into system heap
#define RSRC_ATTR_PURGEABLE  0x20  ///< Purgeable resource
#define RSRC_ATTR_LOCKED     0x10  ///< Locked resource
#define RSRC_ATTR_PROTECTED  0x08  ///< Protected resource
#define RSRC_ATTR_PRELOAD    0x04  ///< Load into memory at open
#define RSRC_ATTR_CHANGED    0x02  ///< Resource has been changed

//
// Helper Functions
//

/**
  Swap bytes for big-endian 16-bit value.
**/
static INLINE UINT16
Swap16 (
  UINT16  Value
  )
{
  return ((Value & 0xFF) << 8) | ((Value >> 8) & 0xFF);
}

/**
  Swap bytes for big-endian 32-bit value.
**/
static INLINE UINT32
Swap32 (
  UINT32  Value
  )
{
  return ((Value & 0xFF) << 24) |
         (((Value >> 8) & 0xFF) << 16) |
         (((Value >> 16) & 0xFF) << 8) |
         ((Value >> 24) & 0xFF);
}

/**
  Read big-endian 16-bit value.
**/
static INLINE UINT16
ReadBE16 (
  CONST VOID  *Ptr
  )
{
#if ANX_BIG_ENDIAN
  return *(CONST UINT16 *)Ptr;
#else
  return Swap16(*(CONST UINT16 *)Ptr);
#endif
}

/**
  Read big-endian 32-bit value.
**/
static INLINE UINT32
ReadBE32 (
  CONST VOID  *Ptr
  )
{
#if ANX_BIG_ENDIAN
  return *(CONST UINT32 *)Ptr;
#else
  return Swap32(*(CONST UINT32 *)Ptr);
#endif
}

/**
  Validate resource fork structure.

  @param[in] ResourceData  Pointer to resource fork data.
  @param[in] DataSize      Size of resource fork data.

  @return S_OK if valid, error code otherwise.
**/
HRESULT
AnxResourceValidate (
  IN CONST VOID  *ResourceData,
  IN UINT64      DataSize
  )
{
  CONST RESOURCE_HEADER  *Header;
  UINT32                 DataOffset;
  UINT32                 MapOffset;
  UINT32                 DataLength;
  UINT32                 MapLength;

  if (ResourceData == NULL || DataSize < sizeof(RESOURCE_HEADER)) {
    return E_INVALIDARG;
  }

  Header = (CONST RESOURCE_HEADER *)ResourceData;

  // Read header fields (big endian)
  DataOffset = ReadBE32(&Header->DataOffset);
  MapOffset = ReadBE32(&Header->MapOffset);
  DataLength = ReadBE32(&Header->DataLength);
  MapLength = ReadBE32(&Header->MapLength);

  // Validate offsets and lengths
  if (DataOffset + DataLength > DataSize) {
    return E_FAIL;
  }

  if (MapOffset + MapLength > DataSize) {
    return E_FAIL;
  }

  if (MapLength < sizeof(RESOURCE_MAP_HEADER)) {
    return E_FAIL;
  }

  return S_OK;
}

/**
  Find resource in resource fork.

  @param[in]  ResourceData  Pointer to resource fork data.
  @param[in]  TypeCode      4-character type code.
  @param[in]  ResourceId    Resource ID to find.
  @param[out] Data          Receives pointer to resource data.
  @param[out] Size          Receives size of resource data.

  @return S_OK if found, S_FALSE if not found, error code otherwise.
**/
HRESULT
AnxResourceFindById (
  IN  CONST VOID  *ResourceData,
  IN  UINT32      TypeCode,
  IN  UINT16      ResourceId,
  OUT VOID        **Data,
  OUT UINT64      *Size
  )
{
  CONST RESOURCE_HEADER      *Header;
  CONST RESOURCE_MAP_HEADER  *MapHeader;
  CONST UINT8                *MapStart;
  CONST UINT8                *TypeListStart;
  CONST RESOURCE_TYPE        *TypeList;
  UINT16                     TypeCount;
  UINT16                     i, j;
  UINT32                     DataOffset;
  UINT32                     MapOffset;

  if (ResourceData == NULL || Data == NULL || Size == NULL) {
    return E_POINTER;
  }

  Header = (CONST RESOURCE_HEADER *)ResourceData;
  DataOffset = ReadBE32(&Header->DataOffset);
  MapOffset = ReadBE32(&Header->MapOffset);

  MapStart = (CONST UINT8 *)ResourceData + MapOffset;
  MapHeader = (CONST RESOURCE_MAP_HEADER *)MapStart;

  TypeListStart = MapStart + ReadBE16(&MapHeader->TypeListOffset);
  TypeCount = ReadBE16((CONST UINT16 *)TypeListStart) + 1;
  TypeList = (CONST RESOURCE_TYPE *)(TypeListStart + 2);

  // Search for matching type
  for (i = 0; i < TypeCount; i++) {
    UINT32  CurrentType = ReadBE32(&TypeList[i].TypeCode);

    if (CurrentType == TypeCode) {
      // Found type, search references
      UINT16  RefCount = ReadBE16(&TypeList[i].Count) + 1;
      UINT16  RefOffset = ReadBE16(&TypeList[i].RefListOffset);
      CONST RESOURCE_REFERENCE  *RefList;

      RefList = (CONST RESOURCE_REFERENCE *)(TypeListStart + RefOffset);

      for (j = 0; j < RefCount; j++) {
        UINT16  Id = ReadBE16(&RefList[j].Id);

        if (Id == ResourceId) {
          // Found resource
          UINT32  AttrAndOffset = ReadBE32(&RefList[j].Attributes);
          UINT32  ResDataOffset = (AttrAndOffset & 0x00FFFFFF);
          CONST UINT8  *ResData;
          UINT32  ResLength;

          ResData = (CONST UINT8 *)ResourceData + DataOffset + ResDataOffset;
          ResLength = ReadBE32((CONST UINT32 *)ResData);
          
          *Data = (VOID *)(ResData + 4);
          *Size = ResLength;

          return S_OK;
        }
      }

      return S_FALSE;  // Type found but ID not found
    }
  }

  return S_FALSE;  // Type not found
}

/**
  Find resource in resource fork by name.

  @param[in]  ResourceData  Pointer to resource fork data.
  @param[in]  TypeCode      4-character type code.
  @param[in]  Name          Resource name to find.
  @param[out] Data          Receives pointer to resource data.
  @param[out] Size          Receives size of resource data.

  @return S_OK if found, S_FALSE if not found, error code otherwise.
**/
HRESULT
AnxResourceFindByName (
  IN  CONST VOID   *ResourceData,
  IN  UINT32       TypeCode,
  IN  CONST CHAR8  *Name,
  OUT VOID         **Data,
  OUT UINT64       *Size
  )
{
  CONST RESOURCE_HEADER      *Header;
  CONST RESOURCE_MAP_HEADER  *MapHeader;
  CONST UINT8                *MapStart;
  CONST UINT8                *TypeListStart;
  CONST UINT8                *NameListStart;
  CONST RESOURCE_TYPE        *TypeList;
  UINT16                     TypeCount;
  UINT16                     i, j;
  UINT32                     DataOffset;
  UINT32                     MapOffset;
  UINTN                      NameLen;

  if (ResourceData == NULL || Name == NULL || Data == NULL || Size == NULL) {
    return E_POINTER;
  }

  NameLen = strlen(Name);
  Header = (CONST RESOURCE_HEADER *)ResourceData;
  DataOffset = ReadBE32(&Header->DataOffset);
  MapOffset = ReadBE32(&Header->MapOffset);

  MapStart = (CONST UINT8 *)ResourceData + MapOffset;
  MapHeader = (CONST RESOURCE_MAP_HEADER *)MapStart;

  TypeListStart = MapStart + ReadBE16(&MapHeader->TypeListOffset);
  NameListStart = MapStart + ReadBE16(&MapHeader->NameListOffset);
  TypeCount = ReadBE16((CONST UINT16 *)TypeListStart) + 1;
  TypeList = (CONST RESOURCE_TYPE *)(TypeListStart + 2);

  // Search for matching type
  for (i = 0; i < TypeCount; i++) {
    UINT32  CurrentType = ReadBE32(&TypeList[i].TypeCode);

    if (CurrentType == TypeCode) {
      // Found type, search references
      UINT16  RefCount = ReadBE16(&TypeList[i].Count) + 1;
      UINT16  RefOffset = ReadBE16(&TypeList[i].RefListOffset);
      CONST RESOURCE_REFERENCE  *RefList;

      RefList = (CONST RESOURCE_REFERENCE *)(TypeListStart + RefOffset);

      for (j = 0; j < RefCount; j++) {
        UINT16  NameOffset = ReadBE16(&RefList[j].NameOffset);

        if (NameOffset != 0xFFFF) {
          // Has name, check it
          CONST UINT8  *ResName = NameListStart + NameOffset;
          UINT8  ResNameLen = *ResName;

          if (ResNameLen == NameLen && 
              memcmp(ResName + 1, Name, NameLen) == 0) {
            // Found resource
            UINT32  AttrAndOffset = ReadBE32(&RefList[j].Attributes);
            UINT32  ResDataOffset = (AttrAndOffset & 0x00FFFFFF);
            CONST UINT8  *ResData;
            UINT32  ResLength;

            ResData = (CONST UINT8 *)ResourceData + DataOffset + ResDataOffset;
            ResLength = ReadBE32((CONST UINT32 *)ResData);
            
            *Data = (VOID *)(ResData + 4);
            *Size = ResLength;

            return S_OK;
          }
        }
      }

      return S_FALSE;  // Type found but name not found
    }
  }

  return S_FALSE;  // Type not found
}

/**
  Count resources of a given type.

  @param[in]  ResourceData  Pointer to resource fork data.
  @param[in]  TypeCode      4-character type code (0 for all types).
  @param[out] Count         Receives resource count.

  @return S_OK on success, error code otherwise.
**/
HRESULT
AnxResourceCount (
  IN  CONST VOID  *ResourceData,
  IN  UINT32      TypeCode,
  OUT UINT32      *Count
  )
{
  CONST RESOURCE_HEADER      *Header;
  CONST RESOURCE_MAP_HEADER  *MapHeader;
  CONST UINT8                *MapStart;
  CONST UINT8                *TypeListStart;
  CONST RESOURCE_TYPE        *TypeList;
  UINT16                     TypeCount;
  UINT16                     i;
  UINT32                     TotalCount;
  UINT32                     MapOffset;

  if (ResourceData == NULL || Count == NULL) {
    return E_POINTER;
  }

  Header = (CONST RESOURCE_HEADER *)ResourceData;
  MapOffset = ReadBE32(&Header->MapOffset);

  MapStart = (CONST UINT8 *)ResourceData + MapOffset;
  MapHeader = (CONST RESOURCE_MAP_HEADER *)MapStart;

  TypeListStart = MapStart + ReadBE16(&MapHeader->TypeListOffset);
  TypeCount = ReadBE16((CONST UINT16 *)TypeListStart) + 1;
  TypeList = (CONST RESOURCE_TYPE *)(TypeListStart + 2);

  TotalCount = 0;

  if (TypeCode == 0) {
    // Count all resources
    for (i = 0; i < TypeCount; i++) {
      TotalCount += ReadBE16(&TypeList[i].Count) + 1;
    }
  } else {
    // Count resources of specific type
    for (i = 0; i < TypeCount; i++) {
      UINT32  CurrentType = ReadBE32(&TypeList[i].TypeCode);

      if (CurrentType == TypeCode) {
        TotalCount = ReadBE16(&TypeList[i].Count) + 1;
        break;
      }
    }
  }

  *Count = TotalCount;
  return S_OK;
}

/**
  Get resource info by type and index.
**/
HRESULT
AnxResourceGetByIndex (
  IN  CONST VOID   *Fork,
  IN  UINT32       TypeCode,
  IN  UINT32       Index,
  OUT UINT16       *Id,
  OUT CONST CHAR8  **Name,
  OUT VOID         **Data,
  OUT UINT64       *Size
  )
{
  CONST RESOURCE_HEADER      *Header;
  CONST RESOURCE_MAP_HEADER  *MapHeader;
  CONST UINT8                *MapStart;
  CONST UINT8                *TypeListStart;
  CONST UINT8                *NameListStart;
  CONST RESOURCE_TYPE        *TypeList;
  UINT16                     TypeCount;
  UINT16                     i;
  UINT32                     DataOffset;
  UINT32                     MapOffset;

  if (Fork == NULL || Id == NULL || Data == NULL || Size == NULL) {
    return E_POINTER;
  }

  Header = (CONST RESOURCE_HEADER *)Fork;
  DataOffset = ReadBE32(&Header->DataOffset);
  MapOffset = ReadBE32(&Header->MapOffset);

  MapStart = (CONST UINT8 *)Fork + MapOffset;
  MapHeader = (CONST RESOURCE_MAP_HEADER *)MapStart;

  TypeListStart = MapStart + ReadBE16(&MapHeader->TypeListOffset);
  NameListStart = MapStart + ReadBE16(&MapHeader->NameListOffset);
  TypeCount = ReadBE16((CONST UINT16 *)TypeListStart) + 1;
  TypeList = (CONST RESOURCE_TYPE *)(TypeListStart + 2);

  // Find matching type
  for (i = 0; i < TypeCount; i++) {
    UINT32  CurrentType = ReadBE32(&TypeList[i].TypeCode);

    if (CurrentType == TypeCode) {
      // Found type, check index
      UINT16  RefCount = ReadBE16(&TypeList[i].Count) + 1;
      UINT16  RefOffset = ReadBE16(&TypeList[i].RefListOffset);
      CONST RESOURCE_REFERENCE  *RefList;
      CONST RESOURCE_REFERENCE  *Ref;
      UINT32  AttrAndOffset;
      UINT32  ResDataOffset;
      CONST UINT8  *ResData;
      UINT32  ResLength;
      UINT16  NameOffset;

      if (Index >= RefCount) {
        return S_FALSE;  // Index out of range
      }

      RefList = (CONST RESOURCE_REFERENCE *)(TypeListStart + RefOffset);
      Ref = &RefList[Index];

      *Id = ReadBE16(&Ref->Id);

      // Get name if present
      NameOffset = ReadBE16(&Ref->NameOffset);
      if (Name != NULL) {
        if (NameOffset != 0xFFFF) {
          CONST UINT8 *ResName = NameListStart + NameOffset;
          *Name = (CONST CHAR8 *)(ResName + 1);  // Skip length byte
        } else {
          *Name = NULL;
        }
      }

      // Get data
      AttrAndOffset = ReadBE32(&Ref->Attributes);
      ResDataOffset = (AttrAndOffset & 0x00FFFFFF);
      ResData = (CONST UINT8 *)Fork + DataOffset + ResDataOffset;
      ResLength = ReadBE32((CONST UINT32 *)ResData);
      
      *Data = (VOID *)(ResData + 4);
      *Size = ResLength;

      return S_OK;
    }
  }

  return S_FALSE;  // Type not found
}

/**
  Enumerate all resource types in fork.
**/
HRESULT
AnxResourceEnumTypes (
  IN  CONST VOID  *Fork,
  IN  UINT32      Index,
  OUT UINT32      *Type,
  OUT UINT32      *Count
  )
{
  CONST RESOURCE_HEADER      *Header;
  CONST RESOURCE_MAP_HEADER  *MapHeader;
  CONST UINT8                *MapStart;
  CONST UINT8                *TypeListStart;
  CONST RESOURCE_TYPE        *TypeList;
  UINT16                     TypeCount;
  UINT32                     MapOffset;

  if (Fork == NULL || Type == NULL || Count == NULL) {
    return E_POINTER;
  }

  Header = (CONST RESOURCE_HEADER *)Fork;
  MapOffset = ReadBE32(&Header->MapOffset);

  MapStart = (CONST UINT8 *)Fork + MapOffset;
  MapHeader = (CONST RESOURCE_MAP_HEADER *)MapStart;

  TypeListStart = MapStart + ReadBE16(&MapHeader->TypeListOffset);
  TypeCount = ReadBE16((CONST UINT16 *)TypeListStart) + 1;

  if (Index >= TypeCount) {
    return S_FALSE;  // No more types
  }

  TypeList = (CONST RESOURCE_TYPE *)(TypeListStart + 2);

  *Type = ReadBE32(&TypeList[Index].TypeCode);
  *Count = ReadBE16(&TypeList[Index].Count) + 1;

  return S_OK;
}
