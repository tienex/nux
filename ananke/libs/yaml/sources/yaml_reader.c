/*++
    Module Name:

        yaml_reader.c

    Abstract:

        Implementation of IYamlReader interface - simple YAML parser.

    Environment:

        C89 compatible.
--*/

#include <ananke/yaml.h>
#include <ananke/atomics.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* --------------------------------------------------------------- */
/*  Parser context                                                  */
/* --------------------------------------------------------------- */

typedef struct _YAML_PARSER_CTX {
    CONST CHAR8 *Input;
    UINTN Position;
    UINTN Length;
    UINTN Line;
    UINTN Column;
    CHAR8 ErrorMessage[256];
} YAML_PARSER_CTX;

typedef struct _YAML_READER_IMPL {
    IYamlReader Base;
    VOLATILE INT32 RefCount;
    YAML_PARSER_CTX LastError;
} YAML_READER_IMPL;

/* --------------------------------------------------------------- */
/*  Parser helpers                                                  */
/* --------------------------------------------------------------- */

static BOOLEAN
IsWhitespace(CHAR8 c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static VOID
SkipWhitespace(YAML_PARSER_CTX *ctx)
{
    while (ctx->Position < ctx->Length) {
        CHAR8 c = ctx->Input[ctx->Position];
        if (c == '\n') {
            ctx->Line++;
            ctx->Column = 1;
            ctx->Position++;
        } else if (IsWhitespace(c)) {
            ctx->Column++;
            ctx->Position++;
        } else {
            break;
        }
    }
}

static VOID
SkipToEndOfLine(YAML_PARSER_CTX *ctx)
{
    while (ctx->Position < ctx->Length && ctx->Input[ctx->Position] != '\n') {
        ctx->Position++;
    }
}

static CHAR8
PeekChar(YAML_PARSER_CTX *ctx)
{
    if (ctx->Position >= ctx->Length) {
        return '\0';
    }
    return ctx->Input[ctx->Position];
}

static CHAR8
ConsumeChar(YAML_PARSER_CTX *ctx)
{
    CHAR8 c;
    if (ctx->Position >= ctx->Length) {
        return '\0';
    }
    c = ctx->Input[ctx->Position++];
    ctx->Column++;
    return c;
}

static HRESULT
ParseScalar(YAML_PARSER_CTX *ctx, IYamlNode **ppNode)
{
    UINTN start = ctx->Position;
    UINTN end;
    HRESULT hr;
    IYamlNode *node = NULL;

    /* Read until whitespace, newline, or special chars */
    while (ctx->Position < ctx->Length) {
        CHAR8 c = ctx->Input[ctx->Position];
        if (IsWhitespace(c) || c == ':' || c == ',' || c == '[' || c == ']' ||
            c == '{' || c == '}' || c == '#') {
            break;
        }
        ctx->Position++;
        ctx->Column++;
    }

    end = ctx->Position;

    /* Create scalar node */
    hr = YamlCreateNode(YamlNodeTypeScalar, &node);
    if (FAILED(hr)) {
        return hr;
    }

    hr = IYamlNode_SetScalarValue(node, &ctx->Input[start], end - start, YamlScalarStylePlain);
    if (FAILED(hr)) {
        IYamlNode_Release(node);
        return hr;
    }

    *ppNode = node;
    return S_OK;
}

static HRESULT
ParseQuotedString(YAML_PARSER_CTX *ctx, IYamlNode **ppNode)
{
    CHAR8 quote = ConsumeChar(ctx); /* Consume opening quote */
    UINTN start = ctx->Position;
    UINTN end;
    HRESULT hr;
    IYamlNode *node = NULL;
    YAML_SCALAR_STYLE style;

    /* Find closing quote */
    while (ctx->Position < ctx->Length) {
        CHAR8 c = ConsumeChar(ctx);
        if (c == quote) {
            break;
        }
    }

    end = ctx->Position - 1; /* Exclude closing quote */

    style = (quote == '\'') ? YamlScalarStyleSingleQ : YamlScalarStyleDoubleQ;

    /* Create scalar node */
    hr = YamlCreateNode(YamlNodeTypeScalar, &node);
    if (FAILED(hr)) {
        return hr;
    }

    hr = IYamlNode_SetScalarValue(node, &ctx->Input[start], end - start, style);
    if (FAILED(hr)) {
        IYamlNode_Release(node);
        return hr;
    }

    *ppNode = node;
    return S_OK;
}

static HRESULT ParseValue(YAML_PARSER_CTX *ctx, IYamlNode **ppNode);

static HRESULT
ParseSequence(YAML_PARSER_CTX *ctx, IYamlNode **ppNode)
{
    HRESULT hr;
    IYamlNode *sequence = NULL;

    hr = YamlCreateNode(YamlNodeTypeSequence, &sequence);
    if (FAILED(hr)) {
        return hr;
    }

    ConsumeChar(ctx); /* Consume '[' */
    SkipWhitespace(ctx);

    while (PeekChar(ctx) != ']' && PeekChar(ctx) != '\0') {
        IYamlNode *item = NULL;

        hr = ParseValue(ctx, &item);
        if (FAILED(hr)) {
            IYamlNode_Release(sequence);
            return hr;
        }

        hr = IYamlNode_AppendSequenceItem(sequence, item);
        IYamlNode_Release(item);
        if (FAILED(hr)) {
            IYamlNode_Release(sequence);
            return hr;
        }

        SkipWhitespace(ctx);
        if (PeekChar(ctx) == ',') {
            ConsumeChar(ctx);
            SkipWhitespace(ctx);
        }
    }

    if (PeekChar(ctx) == ']') {
        ConsumeChar(ctx);
    }

    *ppNode = sequence;
    return S_OK;
}

static HRESULT
ParseMapping(YAML_PARSER_CTX *ctx, IYamlNode **ppNode)
{
    HRESULT hr;
    IYamlNode *mapping = NULL;

    hr = YamlCreateNode(YamlNodeTypeMapping, &mapping);
    if (FAILED(hr)) {
        return hr;
    }

    ConsumeChar(ctx); /* Consume '{' */
    SkipWhitespace(ctx);

    while (PeekChar(ctx) != '}' && PeekChar(ctx) != '\0') {
        IYamlNode *keyNode = NULL;
        IYamlNode *valueNode = NULL;
        CONST CHAR8 *key;
        UINTN keyLen;

        /* Parse key */
        hr = ParseValue(ctx, &keyNode);
        if (FAILED(hr)) {
            IYamlNode_Release(mapping);
            return hr;
        }

        hr = IYamlNode_GetScalarValue(keyNode, &key, &keyLen);
        if (FAILED(hr)) {
            IYamlNode_Release(keyNode);
            IYamlNode_Release(mapping);
            return hr;
        }

        SkipWhitespace(ctx);
        if (PeekChar(ctx) == ':') {
            ConsumeChar(ctx);
        }
        SkipWhitespace(ctx);

        /* Parse value */
        hr = ParseValue(ctx, &valueNode);
        if (FAILED(hr)) {
            IYamlNode_Release(keyNode);
            IYamlNode_Release(mapping);
            return hr;
        }

        /* Add to mapping */
        hr = IYamlNode_SetMappingValue(mapping, key, valueNode);
        IYamlNode_Release(keyNode);
        IYamlNode_Release(valueNode);
        if (FAILED(hr)) {
            IYamlNode_Release(mapping);
            return hr;
        }

        SkipWhitespace(ctx);
        if (PeekChar(ctx) == ',') {
            ConsumeChar(ctx);
            SkipWhitespace(ctx);
        }
    }

    if (PeekChar(ctx) == '}') {
        ConsumeChar(ctx);
    }

    *ppNode = mapping;
    return S_OK;
}

static HRESULT
ParseValue(YAML_PARSER_CTX *ctx, IYamlNode **ppNode)
{
    CHAR8 c;

    SkipWhitespace(ctx);

    /* Skip comments */
    if (PeekChar(ctx) == '#') {
        SkipToEndOfLine(ctx);
        SkipWhitespace(ctx);
    }

    c = PeekChar(ctx);

    if (c == '\0') {
        return E_FAIL;
    }

    if (c == '[') {
        return ParseSequence(ctx, ppNode);
    }

    if (c == '{') {
        return ParseMapping(ctx, ppNode);
    }

    if (c == '\'' || c == '"') {
        return ParseQuotedString(ctx, ppNode);
    }

    return ParseScalar(ctx, ppNode);
}

/* --------------------------------------------------------------- */
/*  IUnknown implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
YamlReader_QueryInterface(
    IYamlReader *This,
    REFIID riid,
    VOID **ppvObject
)
{
    YAML_READER_IMPL *impl = (YAML_READER_IMPL *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (ANX_IS_EQUAL_GUID(riid, &IID_IUnknown) ||
        ANX_IS_EQUAL_GUID(riid, &IID_IYamlReader)) {
        *ppvObject = &impl->Base;
        ANX_INTERLOCKED_INCREMENT(&impl->RefCount);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
YamlReader_AddRef(IYamlReader *This)
{
    YAML_READER_IMPL *impl = (YAML_READER_IMPL *)This;
    return (UINT32)ANX_INTERLOCKED_INCREMENT(&impl->RefCount);
}

static UINT32 STDMETHODCALLTYPE
YamlReader_Release(IYamlReader *This)
{
    YAML_READER_IMPL *impl = (YAML_READER_IMPL *)This;
    UINT32 newRef = (UINT32)ANX_INTERLOCKED_DECREMENT(&impl->RefCount);

    if (newRef == 0) {
        free(impl);
    }

    return newRef;
}

/* --------------------------------------------------------------- */
/*  IYamlReader implementation                                      */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
YamlReader_ParseString(
    IYamlReader *This,
    CONST CHAR8 *YamlString,
    IYamlNode **RootNode
)
{
    YAML_READER_IMPL *impl = (YAML_READER_IMPL *)This;

    if (YamlString == NULL || RootNode == NULL) {
        return E_POINTER;
    }

    return IYamlReader_ParseBuffer(This, (CONST UINT8 *)YamlString, strlen(YamlString), RootNode);
}

static HRESULT STDMETHODCALLTYPE
YamlReader_ParseBuffer(
    IYamlReader *This,
    CONST UINT8 *Buffer,
    UINTN Length,
    IYamlNode **RootNode
)
{
    YAML_READER_IMPL *impl = (YAML_READER_IMPL *)This;
    YAML_PARSER_CTX ctx;
    HRESULT hr;

    if (Buffer == NULL || RootNode == NULL) {
        return E_POINTER;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.Input = (CONST CHAR8 *)Buffer;
    ctx.Length = Length;
    ctx.Line = 1;
    ctx.Column = 1;

    hr = ParseValue(&ctx, RootNode);

    /* Save error context */
    impl->LastError = ctx;

    return hr;
}

static HRESULT STDMETHODCALLTYPE
YamlReader_GetLastError(
    IYamlReader *This,
    CONST CHAR8 **ErrorMessage,
    UINTN *Line,
    UINTN *Column
)
{
    YAML_READER_IMPL *impl = (YAML_READER_IMPL *)This;

    if (ErrorMessage != NULL) {
        *ErrorMessage = impl->LastError.ErrorMessage;
    }
    if (Line != NULL) {
        *Line = impl->LastError.Line;
    }
    if (Column != NULL) {
        *Column = impl->LastError.Column;
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Vtable                                                          */
/* --------------------------------------------------------------- */

static CONST IYamlReaderVtbl gYamlReaderVtbl = {
    YamlReader_QueryInterface,
    YamlReader_AddRef,
    YamlReader_Release,
    YamlReader_ParseString,
    YamlReader_ParseBuffer,
    YamlReader_GetLastError,
};

/* --------------------------------------------------------------- */
/*  Factory function                                                */
/* --------------------------------------------------------------- */

HRESULT STDAPICALLTYPE
YamlCreateReader(
    IYamlReader **ppReader
)
{
    YAML_READER_IMPL *impl;

    if (ppReader == NULL) {
        return E_POINTER;
    }

    impl = (YAML_READER_IMPL *)malloc(sizeof(YAML_READER_IMPL));
    if (impl == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(impl, 0, sizeof(YAML_READER_IMPL));
    impl->Base.lpVtbl = &gYamlReaderVtbl;
    impl->RefCount = 1;

    *ppReader = &impl->Base;
    return S_OK;
}
