/*++
    Module Name:

        yaml_writer.c

    Abstract:

        Implementation of IYamlWriter interface - YAML serialization.

    Environment:

        C89 compatible.
--*/

#include <ananke/yaml.h>
#include <ananke/atomics.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* --------------------------------------------------------------- */
/*  Writer context                                                  */
/* --------------------------------------------------------------- */

typedef struct _YAML_WRITE_CTX {
    CHAR8 *Buffer;
    UINTN BufferSize;
    UINTN BufferCapacity;
    UINT32 Options;
    UINT32 IndentLevel;
} YAML_WRITE_CTX;

typedef struct _YAML_WRITER_IMPL {
    IYamlWriter Base;
    VOLATILE INT32 RefCount;
    UINT32 Options;
} YAML_WRITER_IMPL;

/* --------------------------------------------------------------- */
/*  Writer helpers                                                  */
/* --------------------------------------------------------------- */

static HRESULT
WriteBufferGrow(YAML_WRITE_CTX *ctx, UINTN additionalSize)
{
    UINTN newCapacity;
    CHAR8 *newBuffer;

    if (ctx->BufferSize + additionalSize <= ctx->BufferCapacity) {
        return S_OK;
    }

    newCapacity = ctx->BufferCapacity * 2;
    if (newCapacity < ctx->BufferSize + additionalSize + 256) {
        newCapacity = ctx->BufferSize + additionalSize + 256;
    }

    newBuffer = (CHAR8 *)realloc(ctx->Buffer, newCapacity);
    if (newBuffer == NULL) {
        return E_OUTOFMEMORY;
    }

    ctx->Buffer = newBuffer;
    ctx->BufferCapacity = newCapacity;
    return S_OK;
}

static HRESULT
WriteString(YAML_WRITE_CTX *ctx, CONST CHAR8 *str)
{
    UINTN len = strlen(str);
    HRESULT hr;

    hr = WriteBufferGrow(ctx, len);
    if (FAILED(hr)) {
        return hr;
    }

    memcpy(ctx->Buffer + ctx->BufferSize, str, len);
    ctx->BufferSize += len;

    return S_OK;
}

static HRESULT
WriteChar(YAML_WRITE_CTX *ctx, CHAR8 c)
{
    HRESULT hr;

    hr = WriteBufferGrow(ctx, 1);
    if (FAILED(hr)) {
        return hr;
    }

    ctx->Buffer[ctx->BufferSize++] = c;
    return S_OK;
}

static HRESULT
WriteIndent(YAML_WRITE_CTX *ctx)
{
    UINT32 i;
    HRESULT hr;

    if ((ctx->Options & YamlWriteOptionPrettyPrint) == 0) {
        return S_OK;
    }

    for (i = 0; i < ctx->IndentLevel * 2; i++) {
        hr = WriteChar(ctx, ' ');
        if (FAILED(hr)) {
            return hr;
        }
    }

    return S_OK;
}

static HRESULT WriteNode(YAML_WRITE_CTX *ctx, IYamlNode *node, BOOLEAN inlineMode);

static HRESULT
WriteScalar(YAML_WRITE_CTX *ctx, IYamlNode *node)
{
    CONST CHAR8 *value;
    UINTN length;
    YAML_SCALAR_STYLE style;
    HRESULT hr;
    UINTN i;
    BOOLEAN needsQuotes = FALSE;

    hr = IYamlNode_GetScalarValue(node, &value, &length);
    if (FAILED(hr)) {
        return hr;
    }

    /* Determine if we need quotes */
    if (length == 0) {
        needsQuotes = TRUE;
    } else {
        for (i = 0; i < length; i++) {
            CHAR8 c = value[i];
            if (c == ':' || c == ',' || c == '[' || c == ']' ||
                c == '{' || c == '}' || c == '#' || c == '\n') {
                needsQuotes = TRUE;
                break;
            }
        }
    }

    if (needsQuotes) {
        hr = WriteChar(ctx, '"');
        if (FAILED(hr)) {
            return hr;
        }
    }

    for (i = 0; i < length; i++) {
        hr = WriteChar(ctx, value[i]);
        if (FAILED(hr)) {
            return hr;
        }
    }

    if (needsQuotes) {
        hr = WriteChar(ctx, '"');
        if (FAILED(hr)) {
            return hr;
        }
    }

    return S_OK;
}

static HRESULT
WriteSequence(YAML_WRITE_CTX *ctx, IYamlNode *node, BOOLEAN inlineMode)
{
    UINTN count;
    UINTN i;
    HRESULT hr;

    hr = IYamlNode_GetSequenceCount(node, &count);
    if (FAILED(hr)) {
        return hr;
    }

    if (inlineMode || (ctx->Options & YamlWriteOptionPrettyPrint) == 0) {
        /* Inline flow style: [a, b, c] */
        hr = WriteChar(ctx, '[');
        if (FAILED(hr)) {
            return hr;
        }

        for (i = 0; i < count; i++) {
            IYamlNode *item;

            hr = IYamlNode_GetSequenceItem(node, i, &item);
            if (FAILED(hr)) {
                return hr;
            }

            hr = WriteNode(ctx, item, TRUE);
            IYamlNode_Release(item);
            if (FAILED(hr)) {
                return hr;
            }

            if (i + 1 < count) {
                hr = WriteString(ctx, ", ");
                if (FAILED(hr)) {
                    return hr;
                }
            }
        }

        hr = WriteChar(ctx, ']');
        return hr;
    } else {
        /* Block style with dashes */
        for (i = 0; i < count; i++) {
            IYamlNode *item;

            hr = WriteChar(ctx, '\n');
            if (FAILED(hr)) {
                return hr;
            }

            hr = WriteIndent(ctx);
            if (FAILED(hr)) {
                return hr;
            }

            hr = WriteString(ctx, "- ");
            if (FAILED(hr)) {
                return hr;
            }

            hr = IYamlNode_GetSequenceItem(node, i, &item);
            if (FAILED(hr)) {
                return hr;
            }

            ctx->IndentLevel++;
            hr = WriteNode(ctx, item, FALSE);
            ctx->IndentLevel--;

            IYamlNode_Release(item);
            if (FAILED(hr)) {
                return hr;
            }
        }

        return S_OK;
    }
}

static HRESULT
WriteMapping(YAML_WRITE_CTX *ctx, IYamlNode *node, BOOLEAN inlineMode)
{
    UINTN count;
    UINTN i;
    HRESULT hr;

    hr = IYamlNode_GetMappingCount(node, &count);
    if (FAILED(hr)) {
        return hr;
    }

    if (inlineMode || (ctx->Options & YamlWriteOptionPrettyPrint) == 0) {
        /* Inline flow style: {key: value} */
        hr = WriteChar(ctx, '{');
        if (FAILED(hr)) {
            return hr;
        }

        for (i = 0; i < count; i++) {
            CONST CHAR8 *key;
            UINTN keyLen;
            IYamlNode *value;

            hr = IYamlNode_GetMappingKey(node, i, &key, &keyLen);
            if (FAILED(hr)) {
                return hr;
            }

            hr = WriteString(ctx, key);
            if (FAILED(hr)) {
                return hr;
            }

            hr = WriteString(ctx, ": ");
            if (FAILED(hr)) {
                return hr;
            }

            hr = IYamlNode_GetMappingValue(node, key, &value);
            if (FAILED(hr)) {
                return hr;
            }

            hr = WriteNode(ctx, value, TRUE);
            IYamlNode_Release(value);
            if (FAILED(hr)) {
                return hr;
            }

            if (i + 1 < count) {
                hr = WriteString(ctx, ", ");
                if (FAILED(hr)) {
                    return hr;
                }
            }
        }

        hr = WriteChar(ctx, '}');
        return hr;
    } else {
        /* Block style */
        for (i = 0; i < count; i++) {
            CONST CHAR8 *key;
            UINTN keyLen;
            IYamlNode *value;

            if (i > 0) {
                hr = WriteChar(ctx, '\n');
                if (FAILED(hr)) {
                    return hr;
                }

                hr = WriteIndent(ctx);
                if (FAILED(hr)) {
                    return hr;
                }
            }

            hr = IYamlNode_GetMappingKey(node, i, &key, &keyLen);
            if (FAILED(hr)) {
                return hr;
            }

            hr = WriteString(ctx, key);
            if (FAILED(hr)) {
                return hr;
            }

            hr = WriteString(ctx, ": ");
            if (FAILED(hr)) {
                return hr;
            }

            hr = IYamlNode_GetMappingValue(node, key, &value);
            if (FAILED(hr)) {
                return hr;
            }

            ctx->IndentLevel++;
            hr = WriteNode(ctx, value, FALSE);
            ctx->IndentLevel--;

            IYamlNode_Release(value);
            if (FAILED(hr)) {
                return hr;
            }
        }

        return S_OK;
    }
}

static HRESULT
WriteNode(YAML_WRITE_CTX *ctx, IYamlNode *node, BOOLEAN inlineMode)
{
    YAML_NODE_TYPE type;
    HRESULT hr;

    hr = IYamlNode_GetType(node, &type);
    if (FAILED(hr)) {
        return hr;
    }

    switch (type) {
        case YamlNodeTypeScalar:
            return WriteScalar(ctx, node);

        case YamlNodeTypeSequence:
            return WriteSequence(ctx, node, inlineMode);

        case YamlNodeTypeMapping:
            return WriteMapping(ctx, node, inlineMode);

        default:
            return E_INVALIDARG;
    }
}

/* --------------------------------------------------------------- */
/*  IUnknown implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
YamlWriter_QueryInterface(
    IYamlWriter *This,
    REFIID riid,
    VOID **ppvObject
)
{
    YAML_WRITER_IMPL *impl = (YAML_WRITER_IMPL *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (ANX_IS_EQUAL_GUID(riid, &IID_IUnknown) ||
        ANX_IS_EQUAL_GUID(riid, &IID_IYamlWriter)) {
        *ppvObject = &impl->Base;
        ANX_INTERLOCKED_INCREMENT(&impl->RefCount);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
YamlWriter_AddRef(IYamlWriter *This)
{
    YAML_WRITER_IMPL *impl = (YAML_WRITER_IMPL *)This;
    return (UINT32)ANX_INTERLOCKED_INCREMENT(&impl->RefCount);
}

static UINT32 STDMETHODCALLTYPE
YamlWriter_Release(IYamlWriter *This)
{
    YAML_WRITER_IMPL *impl = (YAML_WRITER_IMPL *)This;
    UINT32 newRef = (UINT32)ANX_INTERLOCKED_DECREMENT(&impl->RefCount);

    if (newRef == 0) {
        free(impl);
    }

    return newRef;
}

/* --------------------------------------------------------------- */
/*  IYamlWriter implementation                                      */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
YamlWriter_SetOptions(
    IYamlWriter *This,
    UINT32 Options
)
{
    YAML_WRITER_IMPL *impl = (YAML_WRITER_IMPL *)This;
    impl->Options = Options;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
YamlWriter_WriteToString(
    IYamlWriter *This,
    IYamlNode *RootNode,
    CHAR8 **OutputString,
    UINTN *OutputLength
)
{
    YAML_WRITER_IMPL *impl = (YAML_WRITER_IMPL *)This;
    YAML_WRITE_CTX ctx;
    HRESULT hr;

    if (RootNode == NULL || OutputString == NULL) {
        return E_POINTER;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.Options = impl->Options;
    ctx.BufferCapacity = 1024;
    ctx.Buffer = (CHAR8 *)malloc(ctx.BufferCapacity);
    if (ctx.Buffer == NULL) {
        return E_OUTOFMEMORY;
    }

    /* Write document start marker */
    if (ctx.Options & YamlWriteOptionExplicitStart) {
        hr = WriteString(&ctx, "---\n");
        if (FAILED(hr)) {
            free(ctx.Buffer);
            return hr;
        }
    }

    /* Write the root node */
    hr = WriteNode(&ctx, RootNode, FALSE);
    if (FAILED(hr)) {
        free(ctx.Buffer);
        return hr;
    }

    /* Write document end marker */
    if (ctx.Options & YamlWriteOptionExplicitEnd) {
        hr = WriteString(&ctx, "\n...");
        if (FAILED(hr)) {
            free(ctx.Buffer);
            return hr;
        }
    }

    /* Add final newline */
    hr = WriteChar(&ctx, '\n');
    if (FAILED(hr)) {
        free(ctx.Buffer);
        return hr;
    }

    /* Null terminate */
    hr = WriteChar(&ctx, '\0');
    if (FAILED(hr)) {
        free(ctx.Buffer);
        return hr;
    }

    *OutputString = ctx.Buffer;
    if (OutputLength != NULL) {
        *OutputLength = ctx.BufferSize - 1; /* Exclude null terminator */
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
YamlWriter_FreeString(
    IYamlWriter *This,
    CHAR8 *String
)
{
    if (String != NULL) {
        free(String);
    }
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Vtable                                                          */
/* --------------------------------------------------------------- */

static CONST IYamlWriterVtbl gYamlWriterVtbl = {
    YamlWriter_QueryInterface,
    YamlWriter_AddRef,
    YamlWriter_Release,
    YamlWriter_SetOptions,
    YamlWriter_WriteToString,
    YamlWriter_FreeString,
};

/* --------------------------------------------------------------- */
/*  Factory function                                                */
/* --------------------------------------------------------------- */

HRESULT STDAPICALLTYPE
YamlCreateWriter(
    IYamlWriter **ppWriter
)
{
    YAML_WRITER_IMPL *impl;

    if (ppWriter == NULL) {
        return E_POINTER;
    }

    impl = (YAML_WRITER_IMPL *)malloc(sizeof(YAML_WRITER_IMPL));
    if (impl == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(impl, 0, sizeof(YAML_WRITER_IMPL));
    impl->Base.lpVtbl = &gYamlWriterVtbl;
    impl->RefCount = 1;
    impl->Options = YamlWriteOptionPrettyPrint;

    *ppWriter = &impl->Base;
    return S_OK;
}
