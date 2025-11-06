/*++
    Module Name:

        yaml_node.c

    Abstract:

        Implementation of IYamlNode interface.

    Environment:

        C89 compatible.
--*/

#include <ananke/yaml.h>
#include <ananke/atomics.h>
#include <string.h>
#include <stdlib.h>

/* --------------------------------------------------------------- */
/*  Internal structures                                             */
/* --------------------------------------------------------------- */

typedef struct _YAML_MAPPING_ENTRY {
    CHAR8 *Key;
    IYamlNode *Value;
    struct _YAML_MAPPING_ENTRY *Next;
} YAML_MAPPING_ENTRY;

typedef struct _YAML_SEQUENCE_ITEM {
    IYamlNode *Node;
    struct _YAML_SEQUENCE_ITEM *Next;
} YAML_SEQUENCE_ITEM;

typedef struct _YAML_NODE_IMPL {
    IYamlNode Base;
    VOLATILE INT32 RefCount;
    YAML_NODE_TYPE NodeType;

    /* Scalar data */
    CHAR8 *ScalarValue;
    UINTN ScalarLength;
    YAML_SCALAR_STYLE ScalarStyle;

    /* Sequence data */
    YAML_SEQUENCE_ITEM *SequenceHead;
    YAML_SEQUENCE_ITEM *SequenceTail;
    UINTN SequenceCount;

    /* Mapping data */
    YAML_MAPPING_ENTRY *MappingHead;
    UINTN MappingCount;
} YAML_NODE_IMPL;

/* --------------------------------------------------------------- */
/*  IUnknown implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
YamlNode_QueryInterface(
    IYamlNode *This,
    REFIID riid,
    VOID **ppvObject
)
{
    YAML_NODE_IMPL *impl = (YAML_NODE_IMPL *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (ANX_IS_EQUAL_GUID(riid, &IID_IUnknown) ||
        ANX_IS_EQUAL_GUID(riid, &IID_IYamlNode)) {
        *ppvObject = &impl->Base;
        ANX_INTERLOCKED_INCREMENT(&impl->RefCount);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
YamlNode_AddRef(IYamlNode *This)
{
    YAML_NODE_IMPL *impl = (YAML_NODE_IMPL *)This;
    return (UINT32)ANX_INTERLOCKED_INCREMENT(&impl->RefCount);
}

static UINT32 STDMETHODCALLTYPE
YamlNode_Release(IYamlNode *This)
{
    YAML_NODE_IMPL *impl = (YAML_NODE_IMPL *)This;
    UINT32 newRef = (UINT32)ANX_INTERLOCKED_DECREMENT(&impl->RefCount);

    if (newRef == 0) {
        /* Free scalar data */
        if (impl->ScalarValue != NULL) {
            free(impl->ScalarValue);
        }

        /* Free sequence data */
        if (impl->NodeType == YamlNodeTypeSequence) {
            YAML_SEQUENCE_ITEM *item = impl->SequenceHead;
            while (item != NULL) {
                YAML_SEQUENCE_ITEM *next = item->Next;
                if (item->Node != NULL) {
                    IYamlNode_Release(item->Node);
                }
                free(item);
                item = next;
            }
        }

        /* Free mapping data */
        if (impl->NodeType == YamlNodeTypeMapping) {
            YAML_MAPPING_ENTRY *entry = impl->MappingHead;
            while (entry != NULL) {
                YAML_MAPPING_ENTRY *next = entry->Next;
                if (entry->Key != NULL) {
                    free(entry->Key);
                }
                if (entry->Value != NULL) {
                    IYamlNode_Release(entry->Value);
                }
                free(entry);
                entry = next;
            }
        }

        free(impl);
    }

    return newRef;
}

/* --------------------------------------------------------------- */
/*  IYamlNode implementation                                        */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
YamlNode_GetType(
    IYamlNode *This,
    YAML_NODE_TYPE *NodeType
)
{
    YAML_NODE_IMPL *impl = (YAML_NODE_IMPL *)This;

    if (NodeType == NULL) {
        return E_POINTER;
    }

    *NodeType = impl->NodeType;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
YamlNode_GetScalarValue(
    IYamlNode *This,
    CONST CHAR8 **Value,
    UINTN *Length
)
{
    YAML_NODE_IMPL *impl = (YAML_NODE_IMPL *)This;

    if (impl->NodeType != YamlNodeTypeScalar) {
        return E_INVALIDARG;
    }

    if (Value != NULL) {
        *Value = impl->ScalarValue;
    }
    if (Length != NULL) {
        *Length = impl->ScalarLength;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
YamlNode_SetScalarValue(
    IYamlNode *This,
    CONST CHAR8 *Value,
    UINTN Length,
    YAML_SCALAR_STYLE Style
)
{
    YAML_NODE_IMPL *impl = (YAML_NODE_IMPL *)This;
    CHAR8 *newValue;

    if (impl->NodeType != YamlNodeTypeScalar) {
        return E_INVALIDARG;
    }

    if (Value == NULL) {
        return E_POINTER;
    }

    /* Allocate new buffer */
    newValue = (CHAR8 *)malloc(Length + 1);
    if (newValue == NULL) {
        return E_OUTOFMEMORY;
    }

    memcpy(newValue, Value, Length);
    newValue[Length] = '\0';

    /* Free old value */
    if (impl->ScalarValue != NULL) {
        free(impl->ScalarValue);
    }

    impl->ScalarValue = newValue;
    impl->ScalarLength = Length;
    impl->ScalarStyle = Style;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
YamlNode_GetSequenceCount(
    IYamlNode *This,
    UINTN *Count
)
{
    YAML_NODE_IMPL *impl = (YAML_NODE_IMPL *)This;

    if (impl->NodeType != YamlNodeTypeSequence) {
        return E_INVALIDARG;
    }

    if (Count == NULL) {
        return E_POINTER;
    }

    *Count = impl->SequenceCount;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
YamlNode_GetSequenceItem(
    IYamlNode *This,
    UINTN Index,
    IYamlNode **Node
)
{
    YAML_NODE_IMPL *impl = (YAML_NODE_IMPL *)This;
    YAML_SEQUENCE_ITEM *item;
    UINTN i;

    if (impl->NodeType != YamlNodeTypeSequence) {
        return E_INVALIDARG;
    }

    if (Node == NULL) {
        return E_POINTER;
    }

    if (Index >= impl->SequenceCount) {
        return E_INVALIDARG;
    }

    item = impl->SequenceHead;
    for (i = 0; i < Index; i++) {
        item = item->Next;
    }

    *Node = item->Node;
    if (*Node != NULL) {
        IYamlNode_AddRef(*Node);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
YamlNode_AppendSequenceItem(
    IYamlNode *This,
    IYamlNode *Node
)
{
    YAML_NODE_IMPL *impl = (YAML_NODE_IMPL *)This;
    YAML_SEQUENCE_ITEM *item;

    if (impl->NodeType != YamlNodeTypeSequence) {
        return E_INVALIDARG;
    }

    if (Node == NULL) {
        return E_POINTER;
    }

    item = (YAML_SEQUENCE_ITEM *)malloc(sizeof(YAML_SEQUENCE_ITEM));
    if (item == NULL) {
        return E_OUTOFMEMORY;
    }

    item->Node = Node;
    item->Next = NULL;
    IYamlNode_AddRef(Node);

    if (impl->SequenceTail != NULL) {
        impl->SequenceTail->Next = item;
    } else {
        impl->SequenceHead = item;
    }
    impl->SequenceTail = item;
    impl->SequenceCount++;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
YamlNode_GetMappingCount(
    IYamlNode *This,
    UINTN *Count
)
{
    YAML_NODE_IMPL *impl = (YAML_NODE_IMPL *)This;

    if (impl->NodeType != YamlNodeTypeMapping) {
        return E_INVALIDARG;
    }

    if (Count == NULL) {
        return E_POINTER;
    }

    *Count = impl->MappingCount;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
YamlNode_GetMappingKey(
    IYamlNode *This,
    UINTN Index,
    CONST CHAR8 **Key,
    UINTN *KeyLength
)
{
    YAML_NODE_IMPL *impl = (YAML_NODE_IMPL *)This;
    YAML_MAPPING_ENTRY *entry;
    UINTN i;

    if (impl->NodeType != YamlNodeTypeMapping) {
        return E_INVALIDARG;
    }

    if (Index >= impl->MappingCount) {
        return E_INVALIDARG;
    }

    entry = impl->MappingHead;
    for (i = 0; i < Index; i++) {
        entry = entry->Next;
    }

    if (Key != NULL) {
        *Key = entry->Key;
    }
    if (KeyLength != NULL) {
        *KeyLength = strlen(entry->Key);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
YamlNode_GetMappingValue(
    IYamlNode *This,
    CONST CHAR8 *Key,
    IYamlNode **Node
)
{
    YAML_NODE_IMPL *impl = (YAML_NODE_IMPL *)This;
    YAML_MAPPING_ENTRY *entry;

    if (impl->NodeType != YamlNodeTypeMapping) {
        return E_INVALIDARG;
    }

    if (Key == NULL || Node == NULL) {
        return E_POINTER;
    }

    entry = impl->MappingHead;
    while (entry != NULL) {
        if (strcmp(entry->Key, Key) == 0) {
            *Node = entry->Value;
            if (*Node != NULL) {
                IYamlNode_AddRef(*Node);
            }
            return S_OK;
        }
        entry = entry->Next;
    }

    return E_NOTFOUND;
}

static HRESULT STDMETHODCALLTYPE
YamlNode_SetMappingValue(
    IYamlNode *This,
    CONST CHAR8 *Key,
    IYamlNode *Node
)
{
    YAML_NODE_IMPL *impl = (YAML_NODE_IMPL *)This;
    YAML_MAPPING_ENTRY *entry;
    CHAR8 *keyCopy;

    if (impl->NodeType != YamlNodeTypeMapping) {
        return E_INVALIDARG;
    }

    if (Key == NULL || Node == NULL) {
        return E_POINTER;
    }

    /* Check if key already exists */
    entry = impl->MappingHead;
    while (entry != NULL) {
        if (strcmp(entry->Key, Key) == 0) {
            /* Replace existing value */
            if (entry->Value != NULL) {
                IYamlNode_Release(entry->Value);
            }
            entry->Value = Node;
            IYamlNode_AddRef(Node);
            return S_OK;
        }
        entry = entry->Next;
    }

    /* Create new entry */
    entry = (YAML_MAPPING_ENTRY *)malloc(sizeof(YAML_MAPPING_ENTRY));
    if (entry == NULL) {
        return E_OUTOFMEMORY;
    }

    keyCopy = (CHAR8 *)malloc(strlen(Key) + 1);
    if (keyCopy == NULL) {
        free(entry);
        return E_OUTOFMEMORY;
    }
    strcpy(keyCopy, Key);

    entry->Key = keyCopy;
    entry->Value = Node;
    entry->Next = impl->MappingHead;
    impl->MappingHead = entry;
    impl->MappingCount++;

    IYamlNode_AddRef(Node);

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Vtable                                                          */
/* --------------------------------------------------------------- */

static CONST IYamlNodeVtbl gYamlNodeVtbl = {
    YamlNode_QueryInterface,
    YamlNode_AddRef,
    YamlNode_Release,
    YamlNode_GetType,
    YamlNode_GetScalarValue,
    YamlNode_SetScalarValue,
    YamlNode_GetSequenceCount,
    YamlNode_GetSequenceItem,
    YamlNode_AppendSequenceItem,
    YamlNode_GetMappingCount,
    YamlNode_GetMappingKey,
    YamlNode_GetMappingValue,
    YamlNode_SetMappingValue,
};

/* --------------------------------------------------------------- */
/*  Factory function                                                */
/* --------------------------------------------------------------- */

HRESULT STDAPICALLTYPE
YamlCreateNode(
    YAML_NODE_TYPE NodeType,
    IYamlNode **ppNode
)
{
    YAML_NODE_IMPL *impl;

    if (ppNode == NULL) {
        return E_POINTER;
    }

    if (NodeType == YamlNodeTypeInvalid) {
        return E_INVALIDARG;
    }

    impl = (YAML_NODE_IMPL *)malloc(sizeof(YAML_NODE_IMPL));
    if (impl == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(impl, 0, sizeof(YAML_NODE_IMPL));
    impl->Base.lpVtbl = &gYamlNodeVtbl;
    impl->RefCount = 1;
    impl->NodeType = NodeType;

    *ppNode = &impl->Base;
    return S_OK;
}
