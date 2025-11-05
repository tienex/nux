/*++
    Module Name:

        yaml.h

    Abstract:

        YAML reader and writer library with COM-based interfaces.
        Provides simple YAML parsing and serialization capabilities
        with minimal dependencies.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>

/* --------------------------------------------------------------- */
/*  YAML Node Types                                                 */
/* --------------------------------------------------------------- */

typedef enum _YAML_NODE_TYPE {
    YamlNodeTypeInvalid    = 0,
    YamlNodeTypeScalar     = 1,  /* String value */
    YamlNodeTypeSequence   = 2,  /* Array/list */
    YamlNodeTypeMapping    = 3,  /* Dictionary/map */
} YAML_NODE_TYPE;

/* --------------------------------------------------------------- */
/*  YAML Scalar Styles                                              */
/* --------------------------------------------------------------- */

typedef enum _YAML_SCALAR_STYLE {
    YamlScalarStylePlain     = 0,  /* No quotes */
    YamlScalarStyleSingleQ   = 1,  /* Single quotes */
    YamlScalarStyleDoubleQ   = 2,  /* Double quotes */
    YamlScalarStyleLiteral   = 3,  /* | style */
    YamlScalarStyleFolded    = 4,  /* > style */
} YAML_SCALAR_STYLE;

/* --------------------------------------------------------------- */
/*  IYamlNode - Represents a YAML node (scalar, sequence, mapping) */
/* --------------------------------------------------------------- */

#define ANX_IID_IYamlNode "YAML0001-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IYamlNode,
    0xYAML0001, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IYamlNode, IUnknown,
    IID_IYamlNode, ANX_IID_IYamlNode)

    /* Get the type of this node */
    ANX_IFACE_METHOD(HRESULT, GetType, (
        OUT YAML_NODE_TYPE *NodeType))

    /* Scalar operations */
    ANX_IFACE_METHOD(HRESULT, GetScalarValue, (
        OUT CONST CHAR8 **Value,
        OUT UINTN *Length))

    ANX_IFACE_METHOD(HRESULT, SetScalarValue, (
        IN CONST CHAR8 *Value,
        IN UINTN Length,
        IN YAML_SCALAR_STYLE Style))

    /* Sequence operations */
    ANX_IFACE_METHOD(HRESULT, GetSequenceCount, (
        OUT UINTN *Count))

    ANX_IFACE_METHOD(HRESULT, GetSequenceItem, (
        IN UINTN Index,
        OUT IYamlNode **Node))

    ANX_IFACE_METHOD(HRESULT, AppendSequenceItem, (
        IN IYamlNode *Node))

    /* Mapping operations */
    ANX_IFACE_METHOD(HRESULT, GetMappingCount, (
        OUT UINTN *Count))

    ANX_IFACE_METHOD(HRESULT, GetMappingKey, (
        IN UINTN Index,
        OUT CONST CHAR8 **Key,
        OUT UINTN *KeyLength))

    ANX_IFACE_METHOD(HRESULT, GetMappingValue, (
        IN CONST CHAR8 *Key,
        OUT IYamlNode **Node))

    ANX_IFACE_METHOD(HRESULT, SetMappingValue, (
        IN CONST CHAR8 *Key,
        IN IYamlNode *Node))

ANX_END_INTERFACE(IYamlNode)

/* C helper macros for IYamlNode */
#define IYamlNode_GetType(This, pType) \
    ((This)->lpVtbl->GetType((This), (pType)))
#define IYamlNode_GetScalarValue(This, ppValue, pLength) \
    ((This)->lpVtbl->GetScalarValue((This), (ppValue), (pLength)))
#define IYamlNode_SetScalarValue(This, value, len, style) \
    ((This)->lpVtbl->SetScalarValue((This), (value), (len), (style)))
#define IYamlNode_GetSequenceCount(This, pCount) \
    ((This)->lpVtbl->GetSequenceCount((This), (pCount)))
#define IYamlNode_GetSequenceItem(This, idx, ppNode) \
    ((This)->lpVtbl->GetSequenceItem((This), (idx), (ppNode)))
#define IYamlNode_AppendSequenceItem(This, node) \
    ((This)->lpVtbl->AppendSequenceItem((This), (node)))
#define IYamlNode_GetMappingCount(This, pCount) \
    ((This)->lpVtbl->GetMappingCount((This), (pCount)))
#define IYamlNode_GetMappingKey(This, idx, ppKey, pLen) \
    ((This)->lpVtbl->GetMappingKey((This), (idx), (ppKey), (pLen)))
#define IYamlNode_GetMappingValue(This, key, ppNode) \
    ((This)->lpVtbl->GetMappingValue((This), (key), (ppNode)))
#define IYamlNode_SetMappingValue(This, key, node) \
    ((This)->lpVtbl->SetMappingValue((This), (key), (node)))

/* --------------------------------------------------------------- */
/*  IYamlReader - Parses YAML from string or buffer                */
/* --------------------------------------------------------------- */

#define ANX_IID_IYamlReader "YAML0002-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IYamlReader,
    0xYAML0002, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IYamlReader, IUnknown,
    IID_IYamlReader, ANX_IID_IYamlReader)

    /* Parse YAML from a null-terminated string */
    ANX_IFACE_METHOD(HRESULT, ParseString, (
        IN CONST CHAR8 *YamlString,
        OUT IYamlNode **RootNode))

    /* Parse YAML from a buffer with explicit length */
    ANX_IFACE_METHOD(HRESULT, ParseBuffer, (
        IN CONST UINT8 *Buffer,
        IN UINTN Length,
        OUT IYamlNode **RootNode))

    /* Get error information from last parse */
    ANX_IFACE_METHOD(HRESULT, GetLastError, (
        OUT CONST CHAR8 **ErrorMessage,
        OUT UINTN *Line,
        OUT UINTN *Column))

ANX_END_INTERFACE(IYamlReader)

/* C helper macros for IYamlReader */
#define IYamlReader_ParseString(This, yaml, ppRoot) \
    ((This)->lpVtbl->ParseString((This), (yaml), (ppRoot)))
#define IYamlReader_ParseBuffer(This, buf, len, ppRoot) \
    ((This)->lpVtbl->ParseBuffer((This), (buf), (len), (ppRoot)))
#define IYamlReader_GetLastError(This, ppMsg, pLine, pCol) \
    ((This)->lpVtbl->GetLastError((This), (ppMsg), (pLine), (pCol)))

/* --------------------------------------------------------------- */
/*  IYamlWriter - Serializes YAML to string                        */
/* --------------------------------------------------------------- */

#define ANX_IID_IYamlWriter "YAML0003-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IYamlWriter,
    0xYAML0003, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

typedef enum _YAML_WRITE_OPTIONS {
    YamlWriteOptionNone           = 0x00,
    YamlWriteOptionPrettyPrint    = 0x01,  /* Use indentation */
    YamlWriteOptionExplicitStart  = 0x02,  /* Add "---" */
    YamlWriteOptionExplicitEnd    = 0x04,  /* Add "..." */
} YAML_WRITE_OPTIONS;

ANX_BEGIN_INTERFACE(IYamlWriter, IUnknown,
    IID_IYamlWriter, ANX_IID_IYamlWriter)

    /* Set write options */
    ANX_IFACE_METHOD(HRESULT, SetOptions, (
        IN UINT32 Options))

    /* Serialize a YAML node to string */
    ANX_IFACE_METHOD(HRESULT, WriteToString, (
        IN IYamlNode *RootNode,
        OUT CHAR8 **OutputString,
        OUT UINTN *OutputLength))

    /* Free string allocated by WriteToString */
    ANX_IFACE_METHOD(HRESULT, FreeString, (
        IN CHAR8 *String))

ANX_END_INTERFACE(IYamlWriter)

/* C helper macros for IYamlWriter */
#define IYamlWriter_SetOptions(This, opts) \
    ((This)->lpVtbl->SetOptions((This), (opts)))
#define IYamlWriter_WriteToString(This, node, ppOut, pLen) \
    ((This)->lpVtbl->WriteToString((This), (node), (ppOut), (pLen)))
#define IYamlWriter_FreeString(This, str) \
    ((This)->lpVtbl->FreeString((This), (str)))

/* --------------------------------------------------------------- */
/*  Factory Functions                                               */
/* --------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/* Create a YAML node of specified type */
HRESULT STDAPICALLTYPE
YamlCreateNode(
    IN YAML_NODE_TYPE NodeType,
    OUT IYamlNode **ppNode
);

/* Create a YAML reader */
HRESULT STDAPICALLTYPE
YamlCreateReader(
    OUT IYamlReader **ppReader
);

/* Create a YAML writer */
HRESULT STDAPICALLTYPE
YamlCreateWriter(
    OUT IYamlWriter **ppWriter
);

#ifdef __cplusplus
}
#endif
