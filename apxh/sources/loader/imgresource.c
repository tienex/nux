/** @file
  Generic Image Resource COM Implementation

  Provides IImageResource and IEnumImageResource implementations
  that wrap ananke resource fork library for use by all image loaders.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>
#include <ananke/resource.h>

//
// IImageResource Implementation
//

typedef struct _ImageResourceImpl {
  IImageResourceVtbl  *Vtbl;
  UINT32              RefCount;
  CONST VOID          *ResourceFork;
  VOID                *Data;
  UINT64              Size;
  UINT32              TypeCode;
  IMGLOAD_RESOURCE_ID Id;
} ImageResourceImpl;

static
HRESULT
STDMETHODCALLTYPE
ImageResource_QueryInterface (
  IN  IImageResource  *This,
  IN  CONST GUID      *Riid,
  OUT VOID            **Object
  )
{
  if (Object == NULL) {
    return E_POINTER;
  }

  if (GuidEquals(Riid, &IID_IUnknown) ||
      GuidEquals(Riid, &IID_IImageResource)) {
    *Object = This;
    This->Vtbl->AddRef(This);
    return S_OK;
  }

  *Object = NULL;
  return E_NOINTERFACE;
}

static
UINT32
STDMETHODCALLTYPE
ImageResource_AddRef (
  IN IImageResource  *This
  )
{
  ImageResourceImpl *Impl = (ImageResourceImpl *)This;
  return ++Impl->RefCount;
}

static
UINT32
STDMETHODCALLTYPE
ImageResource_Release (
  IN IImageResource  *This
  )
{
  ImageResourceImpl *Impl = (ImageResourceImpl *)This;
  UINT32 RefCount = --Impl->RefCount;

  if (RefCount == 0) {
    // Free resource object
    free(Impl);
  }

  return RefCount;
}

static
HRESULT
STDMETHODCALLTYPE
ImageResource_GetData (
  IN  IImageResource  *This,
  OUT VOID            **Data,
  OUT UINT64          *Size
  )
{
  ImageResourceImpl *Impl = (ImageResourceImpl *)This;

  if (Data == NULL || Size == NULL) {
    return E_POINTER;
  }

  *Data = Impl->Data;
  *Size = Impl->Size;

  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
ImageResource_GetType (
  IN  IImageResource  *This,
  OUT UINT32          *Type
  )
{
  ImageResourceImpl *Impl = (ImageResourceImpl *)This;

  if (Type == NULL) {
    return E_POINTER;
  }

  *Type = Impl->TypeCode;

  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
ImageResource_GetId (
  IN  IImageResource      *This,
  OUT IMGLOAD_RESOURCE_ID *ResourceId
  )
{
  ImageResourceImpl *Impl = (ImageResourceImpl *)This;

  if (ResourceId == NULL) {
    return E_POINTER;
  }

  *ResourceId = Impl->Id;

  return S_OK;
}

static CONST IImageResourceVtbl gImageResourceVtbl = {
  ImageResource_QueryInterface,
  ImageResource_AddRef,
  ImageResource_Release,
  ImageResource_GetData,
  ImageResource_GetType,
  ImageResource_GetId
};

/**
  Create IImageResource from resource fork entry.
**/
HRESULT
CreateImageResource (
  IN  CONST VOID       *ResourceFork,
  IN  UINT32           TypeCode,
  IN  UINT16           ResourceId,
  IN  CONST CHAR8      *Name,
  OUT IImageResource   **Resource
  )
{
  ImageResourceImpl  *Impl;
  VOID               *Data;
  UINT64             Size;
  HRESULT            Status;

  if (ResourceFork == NULL || Resource == NULL) {
    return E_POINTER;
  }

  // Find resource data
  if (Name != NULL) {
    Status = AnxResourceFindByName(ResourceFork, TypeCode, Name, &Data, &Size);
  } else {
    Status = AnxResourceFindById(ResourceFork, TypeCode, ResourceId, &Data, &Size);
  }

  if (FAILED(Status)) {
    return Status;
  }

  // Allocate implementation
  Impl = (ImageResourceImpl *)malloc(sizeof(ImageResourceImpl));
  if (Impl == NULL) {
    return E_OUTOFMEMORY;
  }

  memset(Impl, 0, sizeof(ImageResourceImpl));

  Impl->Vtbl = (IImageResourceVtbl *)&gImageResourceVtbl;
  Impl->RefCount = 1;
  Impl->ResourceFork = ResourceFork;
  Impl->Data = Data;
  Impl->Size = Size;
  Impl->TypeCode = TypeCode;
  Impl->Id.IsNumeric = (Name == NULL);
  if (Name != NULL) {
    Impl->Id.Name = Name;
  } else {
    Impl->Id.Id = ResourceId;
  }

  *Resource = (IImageResource *)Impl;

  return S_OK;
}

//
// IEnumImageResource Implementation
//

typedef struct _EnumImageResourceImpl {
  IEnumImageResourceVtbl  *Vtbl;
  UINT32                  RefCount;
  CONST VOID              *ResourceFork;
  UINT32                  TypeCode;
  UINT32                  Position;
  UINT32                  Count;
} EnumImageResourceImpl;

static
HRESULT
STDMETHODCALLTYPE
EnumImageResource_QueryInterface (
  IN  IEnumImageResource  *This,
  IN  CONST GUID          *Riid,
  OUT VOID                **Object
  )
{
  if (Object == NULL) {
    return E_POINTER;
  }

  if (GuidEquals(Riid, &IID_IUnknown) ||
      GuidEquals(Riid, &IID_IEnumImageResource)) {
    *Object = This;
    This->Vtbl->AddRef(This);
    return S_OK;
  }

  *Object = NULL;
  return E_NOINTERFACE;
}

static
UINT32
STDMETHODCALLTYPE
EnumImageResource_AddRef (
  IN IEnumImageResource  *This
  )
{
  EnumImageResourceImpl *Impl = (EnumImageResourceImpl *)This;
  return ++Impl->RefCount;
}

static
UINT32
STDMETHODCALLTYPE
EnumImageResource_Release (
  IN IEnumImageResource  *This
  )
{
  EnumImageResourceImpl *Impl = (EnumImageResourceImpl *)This;
  UINT32 RefCount = --Impl->RefCount;

  if (RefCount == 0) {
    free(Impl);
  }

  return RefCount;
}

static
HRESULT
STDMETHODCALLTYPE
EnumImageResource_Next (
  IN  IEnumImageResource  *This,
  IN  UINT32              Count,
  OUT IImageResource      **Resources,
  OUT UINT32              *Fetched
  )
{
  EnumImageResourceImpl  *Impl = (EnumImageResourceImpl *)This;
  UINT32                 i;
  UINT32                 Retrieved;
  HRESULT                Status;

  if (Resources == NULL) {
    return E_POINTER;
  }

  Retrieved = 0;

  for (i = 0; i < Count && Impl->Position < Impl->Count; i++) {
    UINT16       Id;
    CONST CHAR8  *Name;
    VOID         *Data;
    UINT64       Size;

    Status = AnxResourceGetByIndex(
               Impl->ResourceFork,
               Impl->TypeCode,
               Impl->Position,
               &Id,
               &Name,
               &Data,
               &Size
               );

    if (FAILED(Status)) {
      break;
    }

    Status = CreateImageResource(
               Impl->ResourceFork,
               Impl->TypeCode,
               Id,
               Name,
               &Resources[i]
               );

    if (FAILED(Status)) {
      break;
    }

    Impl->Position++;
    Retrieved++;
  }

  if (Fetched != NULL) {
    *Fetched = Retrieved;
  }

  return (Retrieved == Count) ? S_OK : S_FALSE;
}

static
HRESULT
STDMETHODCALLTYPE
EnumImageResource_Skip (
  IN IEnumImageResource  *This,
  IN UINT32              Count
  )
{
  EnumImageResourceImpl *Impl = (EnumImageResourceImpl *)This;

  if (Impl->Position + Count > Impl->Count) {
    Impl->Position = Impl->Count;
    return S_FALSE;
  }

  Impl->Position += Count;
  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
EnumImageResource_Reset (
  IN IEnumImageResource  *This
  )
{
  EnumImageResourceImpl *Impl = (EnumImageResourceImpl *)This;

  Impl->Position = 0;
  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
EnumImageResource_Clone (
  IN  IEnumImageResource  *This,
  OUT IEnumImageResource  **Clone
  )
{
  EnumImageResourceImpl  *Impl = (EnumImageResourceImpl *)This;
  EnumImageResourceImpl  *CloneImpl;

  if (Clone == NULL) {
    return E_POINTER;
  }

  CloneImpl = (EnumImageResourceImpl *)malloc(sizeof(EnumImageResourceImpl));
  if (CloneImpl == NULL) {
    return E_OUTOFMEMORY;
  }

  memcpy(CloneImpl, Impl, sizeof(EnumImageResourceImpl));
  CloneImpl->RefCount = 1;

  *Clone = (IEnumImageResource *)CloneImpl;

  return S_OK;
}

static CONST IEnumImageResourceVtbl gEnumImageResourceVtbl = {
  EnumImageResource_QueryInterface,
  EnumImageResource_AddRef,
  EnumImageResource_Release,
  EnumImageResource_Next,
  EnumImageResource_Skip,
  EnumImageResource_Reset,
  EnumImageResource_Clone
};

/**
  Create IEnumImageResource for resource fork.
**/
HRESULT
CreateImageResourceEnumerator (
  IN  CONST VOID          *ResourceFork,
  IN  UINT32              TypeCode,
  OUT IEnumImageResource  **Enumerator
  )
{
  EnumImageResourceImpl  *Impl;
  UINT32                 Count;
  HRESULT                Status;

  if (ResourceFork == NULL || Enumerator == NULL) {
    return E_POINTER;
  }

  // Get resource count
  Status = AnxResourceCount(ResourceFork, TypeCode, &Count);
  if (FAILED(Status)) {
    return Status;
  }

  if (Count == 0) {
    return S_FALSE;  // No resources
  }

  // Allocate implementation
  Impl = (EnumImageResourceImpl *)malloc(sizeof(EnumImageResourceImpl));
  if (Impl == NULL) {
    return E_OUTOFMEMORY;
  }

  memset(Impl, 0, sizeof(EnumImageResourceImpl));

  Impl->Vtbl = (IEnumImageResourceVtbl *)&gEnumImageResourceVtbl;
  Impl->RefCount = 1;
  Impl->ResourceFork = ResourceFork;
  Impl->TypeCode = TypeCode;
  Impl->Position = 0;
  Impl->Count = Count;

  *Enumerator = (IEnumImageResource *)Impl;

  return S_OK;
}
