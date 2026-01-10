/*
 * amx_serializer.c - YAML Serializer Implementation
 *
 * Universal YAML serializer for all ITuiSerializable objects.
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TYPE_FACTORIES 256
#define MAX_YAML_SIZE (1024 * 1024)  /* 1 MB */

typedef struct {
    CHAR8 TypeName[128];
    HRESULT (*FactoryFunc)(ITuiSerializable **OutObject);
} TypeFactory;

typedef struct {
    IAmxSerializer Interface;
    UINTN RefCount;

    /* Type factory registry */
    TypeFactory Factories[MAX_TYPE_FACTORIES];
    UINT32 FactoryCount;

} AmxSerializerImpl;

/* Helper: Escape string for YAML */
static VOID EscapeYamlString(CONST CHAR8 *Input, CHAR8 *Output, UINTN MaxLen)
{
    UINTN i = 0, j = 0;

    if (!Input || !Output || MaxLen == 0) return;

    Output[j++] = '"';

    while (Input[i] && j < MaxLen - 2) {
        switch (Input[i]) {
            case '"':
                if (j < MaxLen - 3) {
                    Output[j++] = '\\';
                    Output[j++] = '"';
                }
                break;
            case '\\':
                if (j < MaxLen - 3) {
                    Output[j++] = '\\';
                    Output[j++] = '\\';
                }
                break;
            case '\n':
                if (j < MaxLen - 3) {
                    Output[j++] = '\\';
                    Output[j++] = 'n';
                }
                break;
            case '\r':
                if (j < MaxLen - 3) {
                    Output[j++] = '\\';
                    Output[j++] = 'r';
                }
                break;
            case '\t':
                if (j < MaxLen - 3) {
                    Output[j++] = '\\';
                    Output[j++] = 't';
                }
                break;
            default:
                Output[j++] = Input[i];
                break;
        }
        i++;
    }

    Output[j++] = '"';
    Output[j] = '\0';
}

/* IUnknown methods */
static HRESULT ANXAPI Serializer_QueryInterface(
    IAmxSerializer *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_IAmxSerializer)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI Serializer_AddRef(IAmxSerializer *This)
{
    AmxSerializerImpl *impl = (AmxSerializerImpl *)This;
    return ++impl->RefCount;
}

static UINTN ANXAPI Serializer_Release(IAmxSerializer *This)
{
    AmxSerializerImpl *impl = (AmxSerializerImpl *)This;
    UINTN count = --impl->RefCount;

    if (count == 0) {
        free(impl);
    }

    return count;
}

/* Serialize object to YAML */
static HRESULT ANXAPI Serializer_Serialize(
    IAmxSerializer *This,
    ITuiSerializable *Object,
    CHAR8 **OutYaml,
    UINTN *OutLength
)
{
    AmxSerializerImpl *impl = (AmxSerializerImpl *)This;
    HRESULT hr;
    CHAR8 *yaml = NULL;
    UINTN length = 0;

    if (!Object || !OutYaml || !OutLength) {
        return E_INVALIDARG;
    }

    /* Call object's SerializeToYaml method */
    hr = Object->Vtbl->SerializeToYaml(Object, &yaml, &length);
    if (FAILED(hr)) {
        return hr;
    }

    *OutYaml = yaml;
    *OutLength = length;

    return S_OK;
}

/* Deserialize YAML to object */
static HRESULT ANXAPI Serializer_Deserialize(
    IAmxSerializer *This,
    CONST CHAR8 *Yaml,
    UINTN Length,
    ITuiSerializable **OutObject
)
{
    AmxSerializerImpl *impl = (AmxSerializerImpl *)This;
    HRESULT hr;
    CONST CHAR8 *typeName = NULL;
    ITuiSerializable *object = NULL;

    if (!Yaml || Length == 0 || !OutObject) {
        return E_INVALIDARG;
    }

    /* Parse type from YAML (simplified - looks for "type: TypeName") */
    CHAR8 typeBuffer[128] = {0};
    CONST CHAR8 *typeStart = strstr(Yaml, "type:");
    if (typeStart) {
        typeStart += 5;
        while (*typeStart == ' ' || *typeStart == '\t') typeStart++;

        UINTN i = 0;
        while (typeStart[i] && typeStart[i] != '\n' && typeStart[i] != '\r' && i < sizeof(typeBuffer) - 1) {
            typeBuffer[i] = typeStart[i];
            i++;
        }
        typeBuffer[i] = '\0';
        typeName = typeBuffer;
    }

    if (!typeName) {
        return E_FAIL;
    }

    /* Find factory for this type */
    for (UINT32 i = 0; i < impl->FactoryCount; i++) {
        if (strcmp(impl->Factories[i].TypeName, typeName) == 0) {
            /* Create object using factory */
            hr = impl->Factories[i].FactoryFunc(&object);
            if (FAILED(hr)) {
                return hr;
            }

            /* Deserialize into object */
            hr = object->Vtbl->DeserializeFromYaml(object, Yaml, Length);
            if (FAILED(hr)) {
                object->Vtbl->Release(object);
                return hr;
            }

            *OutObject = object;
            return S_OK;
        }
    }

    /* Type not registered */
    return E_FAIL;
}

/* Serialize to file */
static HRESULT ANXAPI Serializer_SerializeToFile(
    IAmxSerializer *This,
    ITuiSerializable *Object,
    CONST CHAR8 *FilePath
)
{
    HRESULT hr;
    CHAR8 *yaml = NULL;
    UINTN length = 0;
    FILE *file = NULL;

    if (!Object || !FilePath) {
        return E_INVALIDARG;
    }

    /* Serialize to string */
    hr = Serializer_Serialize(This, Object, &yaml, &length);
    if (FAILED(hr)) {
        return hr;
    }

    /* Write to file */
    file = fopen(FilePath, "w");
    if (!file) {
        free(yaml);
        return E_FAIL;
    }

    fwrite(yaml, 1, length, file);
    fclose(file);
    free(yaml);

    return S_OK;
}

/* Deserialize from file */
static HRESULT ANXAPI Serializer_DeserializeFromFile(
    IAmxSerializer *This,
    CONST CHAR8 *FilePath,
    ITuiSerializable **OutObject
)
{
    HRESULT hr;
    FILE *file = NULL;
    CHAR8 *yaml = NULL;
    UINTN length = 0;

    if (!FilePath || !OutObject) {
        return E_INVALIDARG;
    }

    /* Read file */
    file = fopen(FilePath, "r");
    if (!file) {
        return E_FAIL;
    }

    /* Get file size */
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (length == 0 || length > MAX_YAML_SIZE) {
        fclose(file);
        return E_FAIL;
    }

    /* Allocate buffer */
    yaml = (CHAR8 *)malloc(length + 1);
    if (!yaml) {
        fclose(file);
        return E_OUTOFMEMORY;
    }

    /* Read content */
    fread(yaml, 1, length, file);
    yaml[length] = '\0';
    fclose(file);

    /* Deserialize */
    hr = Serializer_Deserialize(This, yaml, length, OutObject);
    free(yaml);

    return hr;
}

/* Register type factory */
static HRESULT ANXAPI Serializer_RegisterTypeFactory(
    IAmxSerializer *This,
    CONST CHAR8 *TypeName,
    HRESULT (*FactoryFunc)(ITuiSerializable **OutObject)
)
{
    AmxSerializerImpl *impl = (AmxSerializerImpl *)This;

    if (!TypeName || !FactoryFunc) {
        return E_INVALIDARG;
    }

    if (impl->FactoryCount >= MAX_TYPE_FACTORIES) {
        return E_OUTOFMEMORY;
    }

    /* Add factory */
    TypeFactory *factory = &impl->Factories[impl->FactoryCount++];
    strncpy(factory->TypeName, TypeName, sizeof(factory->TypeName) - 1);
    factory->TypeName[sizeof(factory->TypeName) - 1] = '\0';
    factory->FactoryFunc = FactoryFunc;

    return S_OK;
}

/* Validate YAML */
static HRESULT ANXAPI Serializer_ValidateYaml(
    IAmxSerializer *This,
    CONST CHAR8 *Yaml,
    UINTN Length,
    BOOLEAN *IsValid,
    CHAR8 **ErrorMessage
)
{
    AmxSerializerImpl *impl = (AmxSerializerImpl *)This;

    if (!Yaml || Length == 0 || !IsValid) {
        return E_INVALIDARG;
    }

    /* Basic validation - check for proper YAML structure */
    /* This is simplified - real implementation would use a YAML parser */

    *IsValid = TRUE;

    /* Check for basic syntax errors */
    INT32 indentLevel = 0;
    CONST CHAR8 *ptr = Yaml;

    while (*ptr) {
        if (*ptr == '\n') {
            /* Check next line indentation */
            ptr++;
            INT32 spaces = 0;
            while (*ptr == ' ') {
                spaces++;
                ptr++;
            }

            /* Indent should be multiple of 2 */
            if (spaces % 2 != 0 && *ptr != '\n' && *ptr != '\0') {
                *IsValid = FALSE;
                if (ErrorMessage) {
                    *ErrorMessage = strdup("Invalid indentation (must be multiple of 2)");
                }
                return S_OK;
            }
        } else {
            ptr++;
        }
    }

    if (ErrorMessage) {
        *ErrorMessage = NULL;
    }

    return S_OK;
}

/* VTable */
static IAmxSerializer_Vtbl SerializerVtbl = {
    Serializer_QueryInterface,
    Serializer_AddRef,
    Serializer_Release,
    Serializer_Serialize,
    Serializer_Deserialize,
    Serializer_SerializeToFile,
    Serializer_DeserializeFromFile,
    Serializer_RegisterTypeFactory,
    Serializer_ValidateYaml
};

/* Factory function */
HRESULT AnxAmxCreateSerializer(IAmxSerializer **OutSerializer)
{
    AmxSerializerImpl *impl;

    if (!OutSerializer) return E_INVALIDARG;

    impl = (AmxSerializerImpl *)calloc(1, sizeof(AmxSerializerImpl));
    if (!impl) return E_OUTOFMEMORY;

    impl->Interface.Vtbl = &SerializerVtbl;
    impl->RefCount = 1;
    impl->FactoryCount = 0;

    *OutSerializer = &impl->Interface;
    return S_OK;
}
