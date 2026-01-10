/*
 * amx_resource_manager.c - Universal Resource Manager
 *
 * Manages resources with URI-based storage and YAML persistence.
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_RESOURCES 1024
#define MAX_URI_LENGTH 512
#define MAX_METADATA_ENTRIES 32

typedef struct {
    CHAR8 Key[128];
    CHAR8 Value[512];
} MetadataEntry;

/* Resource implementation */
typedef struct {
    IAmxResource Interface;
    UINTN RefCount;

    CHAR8 Uri[MAX_URI_LENGTH];
    CHAR8 Type[128];
    ITuiSerializable *Object;

    MetadataEntry Metadata[MAX_METADATA_ENTRIES];
    UINT32 MetadataCount;

} AmxResourceImpl;

/* Resource Manager implementation */
typedef struct {
    IAmxResourceManager Interface;
    UINTN RefCount;

    /* Resources storage */
    AmxResourceImpl *Resources[MAX_RESOURCES];
    UINT32 ResourceCount;

    /* Serializer */
    IAmxSerializer *Serializer;

} AmxResourceManagerImpl;

/* Resource IUnknown methods */
static HRESULT ANXAPI Resource_QueryInterface(
    IAmxResource *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_IAmxResource)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI Resource_AddRef(IAmxResource *This)
{
    AmxResourceImpl *impl = (AmxResourceImpl *)This;
    return ++impl->RefCount;
}

static UINTN ANXAPI Resource_Release(IAmxResource *This)
{
    AmxResourceImpl *impl = (AmxResourceImpl *)This;
    UINTN count = --impl->RefCount;

    if (count == 0) {
        if (impl->Object) {
            impl->Object->Vtbl->Release(impl->Object);
        }
        free(impl);
    }

    return count;
}

/* Resource methods */
static HRESULT ANXAPI Resource_GetId(
    IAmxResource *This,
    CONST CHAR8 **OutId
)
{
    AmxResourceImpl *impl = (AmxResourceImpl *)This;

    if (!OutId) return E_INVALIDARG;

    *OutId = impl->Uri;
    return S_OK;
}

static HRESULT ANXAPI Resource_GetType(
    IAmxResource *This,
    CONST CHAR8 **OutType
)
{
    AmxResourceImpl *impl = (AmxResourceImpl *)This;

    if (!OutType) return E_INVALIDARG;

    *OutType = impl->Type;
    return S_OK;
}

static HRESULT ANXAPI Resource_GetObject(
    IAmxResource *This,
    ITuiSerializable **OutObject
)
{
    AmxResourceImpl *impl = (AmxResourceImpl *)This;

    if (!OutObject) return E_INVALIDARG;

    *OutObject = impl->Object;
    if (impl->Object) {
        impl->Object->Vtbl->AddRef(impl->Object);
    }

    return S_OK;
}

static HRESULT ANXAPI Resource_GetMetadata(
    IAmxResource *This,
    CONST CHAR8 *Key,
    CONST CHAR8 **OutValue
)
{
    AmxResourceImpl *impl = (AmxResourceImpl *)This;

    if (!Key || !OutValue) return E_INVALIDARG;

    for (UINT32 i = 0; i < impl->MetadataCount; i++) {
        if (strcmp(impl->Metadata[i].Key, Key) == 0) {
            *OutValue = impl->Metadata[i].Value;
            return S_OK;
        }
    }

    return E_FAIL;
}

static HRESULT ANXAPI Resource_SetMetadata(
    IAmxResource *This,
    CONST CHAR8 *Key,
    CONST CHAR8 *Value
)
{
    AmxResourceImpl *impl = (AmxResourceImpl *)This;

    if (!Key || !Value) return E_INVALIDARG;

    /* Check if key exists */
    for (UINT32 i = 0; i < impl->MetadataCount; i++) {
        if (strcmp(impl->Metadata[i].Key, Key) == 0) {
            strncpy(impl->Metadata[i].Value, Value, sizeof(impl->Metadata[i].Value) - 1);
            impl->Metadata[i].Value[sizeof(impl->Metadata[i].Value) - 1] = '\0';
            return S_OK;
        }
    }

    /* Add new entry */
    if (impl->MetadataCount >= MAX_METADATA_ENTRIES) {
        return E_OUTOFMEMORY;
    }

    MetadataEntry *entry = &impl->Metadata[impl->MetadataCount++];
    strncpy(entry->Key, Key, sizeof(entry->Key) - 1);
    entry->Key[sizeof(entry->Key) - 1] = '\0';
    strncpy(entry->Value, Value, sizeof(entry->Value) - 1);
    entry->Value[sizeof(entry->Value) - 1] = '\0';

    return S_OK;
}

/* Resource VTable */
static IAmxResource_Vtbl ResourceVtbl = {
    Resource_QueryInterface,
    Resource_AddRef,
    Resource_Release,
    Resource_GetId,
    Resource_GetType,
    Resource_GetObject,
    Resource_GetMetadata,
    Resource_SetMetadata
};

/* Resource Manager IUnknown methods */
static HRESULT ANXAPI ResourceManager_QueryInterface(
    IAmxResourceManager *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_IAmxResourceManager)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI ResourceManager_AddRef(IAmxResourceManager *This)
{
    AmxResourceManagerImpl *impl = (AmxResourceManagerImpl *)This;
    return ++impl->RefCount;
}

static UINTN ANXAPI ResourceManager_Release(IAmxResourceManager *This)
{
    AmxResourceManagerImpl *impl = (AmxResourceManagerImpl *)This;
    UINTN count = --impl->RefCount;

    if (count == 0) {
        /* Release all resources */
        for (UINT32 i = 0; i < impl->ResourceCount; i++) {
            if (impl->Resources[i]) {
                impl->Resources[i]->Interface.Vtbl->Release(&impl->Resources[i]->Interface);
            }
        }

        /* Release serializer */
        if (impl->Serializer) {
            impl->Serializer->Vtbl->Release(impl->Serializer);
        }

        free(impl);
    }

    return count;
}

/* Register resource */
static HRESULT ANXAPI ResourceManager_RegisterResource(
    IAmxResourceManager *This,
    CONST CHAR8 *Uri,
    ITuiSerializable *Object,
    IAmxResource **OutResource
)
{
    AmxResourceManagerImpl *impl = (AmxResourceManagerImpl *)This;
    AmxResourceImpl *resource = NULL;
    CONST CHAR8 *typeName = NULL;
    HRESULT hr;

    if (!Uri || !Object) {
        return E_INVALIDARG;
    }

    if (impl->ResourceCount >= MAX_RESOURCES) {
        return E_OUTOFMEMORY;
    }

    /* Get object type */
    hr = Object->Vtbl->GetTypeName(Object, &typeName);
    if (FAILED(hr)) {
        return hr;
    }

    /* Create resource */
    resource = (AmxResourceImpl *)calloc(1, sizeof(AmxResourceImpl));
    if (!resource) {
        return E_OUTOFMEMORY;
    }

    resource->Interface.Vtbl = &ResourceVtbl;
    resource->RefCount = 1;

    strncpy(resource->Uri, Uri, sizeof(resource->Uri) - 1);
    resource->Uri[sizeof(resource->Uri) - 1] = '\0';

    strncpy(resource->Type, typeName, sizeof(resource->Type) - 1);
    resource->Type[sizeof(resource->Type) - 1] = '\0';

    resource->Object = Object;
    Object->Vtbl->AddRef(Object);

    resource->MetadataCount = 0;

    /* Store resource */
    impl->Resources[impl->ResourceCount++] = resource;

    if (OutResource) {
        *OutResource = &resource->Interface;
        resource->Interface.Vtbl->AddRef(&resource->Interface);
    }

    return S_OK;
}

/* Get resource */
static HRESULT ANXAPI ResourceManager_GetResource(
    IAmxResourceManager *This,
    CONST CHAR8 *Uri,
    IAmxResource **OutResource
)
{
    AmxResourceManagerImpl *impl = (AmxResourceManagerImpl *)This;

    if (!Uri || !OutResource) {
        return E_INVALIDARG;
    }

    for (UINT32 i = 0; i < impl->ResourceCount; i++) {
        if (strcmp(impl->Resources[i]->Uri, Uri) == 0) {
            *OutResource = &impl->Resources[i]->Interface;
            impl->Resources[i]->Interface.Vtbl->AddRef(&impl->Resources[i]->Interface);
            return S_OK;
        }
    }

    return E_FAIL;
}

/* Remove resource */
static HRESULT ANXAPI ResourceManager_RemoveResource(
    IAmxResourceManager *This,
    CONST CHAR8 *Uri
)
{
    AmxResourceManagerImpl *impl = (AmxResourceManagerImpl *)This;

    if (!Uri) {
        return E_INVALIDARG;
    }

    for (UINT32 i = 0; i < impl->ResourceCount; i++) {
        if (strcmp(impl->Resources[i]->Uri, Uri) == 0) {
            /* Release resource */
            impl->Resources[i]->Interface.Vtbl->Release(&impl->Resources[i]->Interface);

            /* Shift remaining resources */
            for (UINT32 j = i; j < impl->ResourceCount - 1; j++) {
                impl->Resources[j] = impl->Resources[j + 1];
            }
            impl->ResourceCount--;

            return S_OK;
        }
    }

    return E_FAIL;
}

/* Load from file */
static HRESULT ANXAPI ResourceManager_LoadFromFile(
    IAmxResourceManager *This,
    CONST CHAR8 *FilePath
)
{
    AmxResourceManagerImpl *impl = (AmxResourceManagerImpl *)This;
    FILE *file = NULL;
    CHAR8 *yaml = NULL;
    UINTN length = 0;

    if (!FilePath) {
        return E_INVALIDARG;
    }

    if (!impl->Serializer) {
        return E_FAIL;
    }

    /* Read file */
    file = fopen(FilePath, "r");
    if (!file) {
        return E_FAIL;
    }

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

    yaml = (CHAR8 *)malloc(length + 1);
    if (!yaml) {
        fclose(file);
        return E_OUTOFMEMORY;
    }

    fread(yaml, 1, length, file);
    yaml[length] = '\0';
    fclose(file);

    /* Parse YAML and load resources */
    /* Simplified implementation - just deserialize single object */
    ITuiSerializable *object = NULL;
    HRESULT hr = impl->Serializer->Vtbl->Deserialize(impl->Serializer, yaml, length, &object);
    free(yaml);

    if (FAILED(hr)) {
        return hr;
    }

    /* Register as resource with filename as URI */
    hr = ResourceManager_RegisterResource(This, FilePath, object, NULL);
    object->Vtbl->Release(object);

    return hr;
}

/* Save to file */
static HRESULT ANXAPI ResourceManager_SaveToFile(
    IAmxResourceManager *This,
    CONST CHAR8 *FilePath
)
{
    AmxResourceManagerImpl *impl = (AmxResourceManagerImpl *)This;
    FILE *file = NULL;

    if (!FilePath) {
        return E_INVALIDARG;
    }

    if (!impl->Serializer) {
        return E_FAIL;
    }

    /* Open file */
    file = fopen(FilePath, "w");
    if (!file) {
        return E_FAIL;
    }

    /* Write header */
    fprintf(file, "# ANXCONFIG Universal Resource File\n");
    fprintf(file, "# Generated by IAmxResourceManager\n\n");
    fprintf(file, "resources:\n");

    /* Serialize each resource */
    for (UINT32 i = 0; i < impl->ResourceCount; i++) {
        AmxResourceImpl *res = impl->Resources[i];

        fprintf(file, "  - uri: \"%s\"\n", res->Uri);
        fprintf(file, "    type: \"%s\"\n", res->Type);

        /* Serialize object */
        if (res->Object) {
            CHAR8 *yaml = NULL;
            UINTN length = 0;

            HRESULT hr = impl->Serializer->Vtbl->Serialize(impl->Serializer, res->Object, &yaml, &length);
            if (SUCCEEDED(hr) && yaml) {
                /* Indent object YAML */
                CHAR8 *line = strtok(yaml, "\n");
                while (line) {
                    fprintf(file, "    %s\n", line);
                    line = strtok(NULL, "\n");
                }
                free(yaml);
            }
        }

        /* Write metadata */
        if (res->MetadataCount > 0) {
            fprintf(file, "    metadata:\n");
            for (UINT32 j = 0; j < res->MetadataCount; j++) {
                fprintf(file, "      %s: \"%s\"\n",
                    res->Metadata[j].Key,
                    res->Metadata[j].Value);
            }
        }

        fprintf(file, "\n");
    }

    fclose(file);
    return S_OK;
}

/* Enumerate resources */
static HRESULT ANXAPI ResourceManager_EnumerateResources(
    IAmxResourceManager *This,
    HRESULT (*Callback)(CONST CHAR8 *Uri, IAmxResource *Resource, VOID *UserData),
    VOID *UserData
)
{
    AmxResourceManagerImpl *impl = (AmxResourceManagerImpl *)This;

    if (!Callback) {
        return E_INVALIDARG;
    }

    for (UINT32 i = 0; i < impl->ResourceCount; i++) {
        HRESULT hr = Callback(impl->Resources[i]->Uri, &impl->Resources[i]->Interface, UserData);
        if (FAILED(hr)) {
            return hr;
        }
    }

    return S_OK;
}

/* Get serializer */
static HRESULT ANXAPI ResourceManager_GetSerializer(
    IAmxResourceManager *This,
    IAmxSerializer **OutSerializer
)
{
    AmxResourceManagerImpl *impl = (AmxResourceManagerImpl *)This;

    if (!OutSerializer) {
        return E_INVALIDARG;
    }

    *OutSerializer = impl->Serializer;
    if (impl->Serializer) {
        impl->Serializer->Vtbl->AddRef(impl->Serializer);
    }

    return S_OK;
}

/* VTable */
static IAmxResourceManager_Vtbl ResourceManagerVtbl = {
    ResourceManager_QueryInterface,
    ResourceManager_AddRef,
    ResourceManager_Release,
    ResourceManager_RegisterResource,
    ResourceManager_GetResource,
    ResourceManager_RemoveResource,
    ResourceManager_LoadFromFile,
    ResourceManager_SaveToFile,
    ResourceManager_EnumerateResources,
    ResourceManager_GetSerializer
};

/* Factory function */
HRESULT AnxAmxCreateResourceManager(IAmxResourceManager **OutManager)
{
    AmxResourceManagerImpl *impl;
    IAmxSerializer *serializer = NULL;
    HRESULT hr;

    if (!OutManager) return E_INVALIDARG;

    /* Create serializer */
    hr = AnxAmxCreateSerializer(&serializer);
    if (FAILED(hr)) {
        return hr;
    }

    impl = (AmxResourceManagerImpl *)calloc(1, sizeof(AmxResourceManagerImpl));
    if (!impl) {
        serializer->Vtbl->Release(serializer);
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &ResourceManagerVtbl;
    impl->RefCount = 1;
    impl->ResourceCount = 0;
    impl->Serializer = serializer;

    *OutManager = &impl->Interface;
    return S_OK;
}
