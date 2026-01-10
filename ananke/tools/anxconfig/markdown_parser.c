/*
 * Markdown Parser Implementation (COM Component)
 *
 * Parses Markdown syntax and produces a structured document tree.
 * Supports CommonMark specification with extensions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "widgets_common.h"

#define MAX_NODES 1024
#define MAX_TEXT_LENGTH 4096

typedef enum {
    MdNodeDocument,
    MdNodeHeading,
    MdNodeParagraph,
    MdNodeBlockquote,
    MdNodeCodeBlock,
    MdNodeList,
    MdNodeListItem,
    MdNodeHorizontalRule,
    MdNodeText,
    MdNodeEmphasis,
    MdNodeStrong,
    MdNodeCode,
    MdNodeLink,
    MdNodeImage,
    MdNodeLineBreak,
    MdNodeTable,
    MdNodeTableRow,
    MdNodeTableCell
} MarkdownNodeType;

typedef struct _MarkdownNode {
    MarkdownNodeType Type;
    CHAR8 Text[MAX_TEXT_LENGTH];
    CHAR8 Url[512];           /* For links/images */
    CHAR8 Title[256];         /* For links/images */
    CHAR8 Language[64];       /* For code blocks */
    UINT32 Level;             /* For headings (1-6) */
    BOOLEAN Ordered;          /* For lists */
    struct _MarkdownNode *Parent;
    struct _MarkdownNode *Children[32];
    UINT32 ChildCount;
    struct _MarkdownNode *Next;  /* Sibling */
} MarkdownNode;

typedef struct {
    IMarkdownParser Interface;
    UINTN RefCount;
    MarkdownNode *Root;
    MarkdownNode Nodes[MAX_NODES];
    UINT32 NodeCount;
    CHAR8 *SourceText;
} MarkdownParserImpl;

/* IUnknown methods */
static HRESULT ANXAPI MarkdownParser_QueryInterface(
    IMarkdownParser *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI MarkdownParser_AddRef(IMarkdownParser *This)
{
    MarkdownParserImpl *impl = (MarkdownParserImpl *)This;
    return ++impl->RefCount;
}

static UINTN ANXAPI MarkdownParser_Release(IMarkdownParser *This)
{
    MarkdownParserImpl *impl = (MarkdownParserImpl *)This;
    UINTN refCount = --impl->RefCount;
    if (refCount == 0) {
        if (impl->SourceText) free(impl->SourceText);
        free(impl);
    }
    return refCount;
}

/* Helper: Create new node */
static MarkdownNode *CreateNode(
    MarkdownParserImpl *impl,
    MarkdownNodeType Type
)
{
    if (impl->NodeCount >= MAX_NODES) return NULL;

    MarkdownNode *node = &impl->Nodes[impl->NodeCount++];
    memset(node, 0, sizeof(MarkdownNode));
    node->Type = Type;
    return node;
}

/* Helper: Add child to node */
static VOID AddChild(MarkdownNode *parent, MarkdownNode *child)
{
    if (parent->ChildCount < 32) {
        parent->Children[parent->ChildCount++] = child;
        child->Parent = parent;
    }
}

/* Helper: Check if line is heading */
static INT32 IsHeading(CONST CHAR8 *line)
{
    INT32 level = 0;
    while (*line == '#' && level < 6) {
        level++;
        line++;
    }
    if (level > 0 && *line == ' ') {
        return level;
    }
    return 0;
}

/* Helper: Check if line is horizontal rule */
static BOOLEAN IsHorizontalRule(CONST CHAR8 *line)
{
    INT32 count = 0;
    CHAR8 ch = *line;

    if (ch != '-' && ch != '_' && ch != '*') return FALSE;

    while (*line != '\0') {
        if (*line == ch) {
            count++;
        } else if (*line != ' ') {
            return FALSE;
        }
        line++;
    }

    return count >= 3;
}

/* Helper: Check if line starts code block */
static BOOLEAN IsCodeBlockStart(CONST CHAR8 *line, CHAR8 *language)
{
    if (strncmp(line, "```", 3) == 0 || strncmp(line, "~~~", 3) == 0) {
        /* Extract language if present */
        line += 3;
        INT32 i = 0;
        while (*line && !isspace((unsigned char)*line) && i < 63) {
            language[i++] = *line++;
        }
        language[i] = '\0';
        return TRUE;
    }
    return FALSE;
}

/* Helper: Check if line is list item */
static BOOLEAN IsListItem(CONST CHAR8 *line, BOOLEAN *ordered, INT32 *indent)
{
    *indent = 0;
    while (*line == ' ') {
        (*indent)++;
        line++;
    }

    /* Unordered list */
    if (*line == '-' || *line == '*' || *line == '+') {
        if (line[1] == ' ') {
            *ordered = FALSE;
            return TRUE;
        }
    }

    /* Ordered list */
    if (isdigit((unsigned char)*line)) {
        while (isdigit((unsigned char)*line)) line++;
        if (*line == '.' && line[1] == ' ') {
            *ordered = TRUE;
            return TRUE;
        }
    }

    return FALSE;
}

/* Helper: Parse inline formatting */
static MarkdownNode *ParseInline(
    MarkdownParserImpl *impl,
    CONST CHAR8 *text,
    UINT32 length
)
{
    MarkdownNode *node = CreateNode(impl, MdNodeText);
    if (node == NULL) return NULL;

    strncpy(node->Text, text, length);
    node->Text[length] = '\0';

    /* TODO: Parse inline formatting (bold, italic, code, links) */
    /* This is a simplified implementation */

    return node;
}

/* IMarkdownParser methods */
static HRESULT ANXAPI MarkdownParser_Parse(
    IMarkdownParser *This,
    CONST CHAR8 *MarkdownText
)
{
    MarkdownParserImpl *impl = (MarkdownParserImpl *)This;
    CHAR8 *text;
    CHAR8 *line;
    CHAR8 *saveptr;
    MarkdownNode *currentNode;
    MarkdownNode *listNode = NULL;
    BOOLEAN inCodeBlock = FALSE;
    MarkdownNode *codeBlockNode = NULL;

    if (MarkdownText == NULL) return E_POINTER;

    /* Reset parser state */
    impl->NodeCount = 0;
    if (impl->SourceText) free(impl->SourceText);
    impl->SourceText = strdup(MarkdownText);

    /* Create root document node */
    impl->Root = CreateNode(impl, MdNodeDocument);
    if (impl->Root == NULL) return E_OUTOFMEMORY;

    currentNode = impl->Root;

    /* Make a working copy for strtok */
    text = strdup(MarkdownText);
    if (text == NULL) return E_OUTOFMEMORY;

    /* Parse line by line */
    line = strtok_r(text, "\n", &saveptr);
    while (line != NULL) {
        /* Skip leading spaces */
        while (*line == ' ') line++;

        /* Check for code block delimiters */
        CHAR8 language[64];
        if (IsCodeBlockStart(line, language)) {
            if (!inCodeBlock) {
                /* Start code block */
                codeBlockNode = CreateNode(impl, MdNodeCodeBlock);
                if (codeBlockNode) {
                    strcpy(codeBlockNode->Language, language);
                    AddChild(impl->Root, codeBlockNode);
                    inCodeBlock = TRUE;
                }
            } else {
                /* End code block */
                inCodeBlock = FALSE;
                codeBlockNode = NULL;
            }
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        /* Inside code block */
        if (inCodeBlock && codeBlockNode) {
            strcat(codeBlockNode->Text, line);
            strcat(codeBlockNode->Text, "\n");
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        /* Empty line */
        if (*line == '\0') {
            listNode = NULL;  /* Reset list context */
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        /* Heading */
        INT32 level = IsHeading(line);
        if (level > 0) {
            MarkdownNode *heading = CreateNode(impl, MdNodeHeading);
            if (heading) {
                heading->Level = level;
                line += level + 1;  /* Skip # and space */
                strncpy(heading->Text, line, sizeof(heading->Text) - 1);
                AddChild(impl->Root, heading);
            }
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        /* Horizontal rule */
        if (IsHorizontalRule(line)) {
            MarkdownNode *hr = CreateNode(impl, MdNodeHorizontalRule);
            if (hr) {
                AddChild(impl->Root, hr);
            }
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        /* List item */
        BOOLEAN ordered;
        INT32 indent;
        if (IsListItem(line, &ordered, &indent)) {
            /* Create list if needed */
            if (listNode == NULL || listNode->Ordered != ordered) {
                listNode = CreateNode(impl, MdNodeList);
                if (listNode) {
                    listNode->Ordered = ordered;
                    AddChild(impl->Root, listNode);
                }
            }

            if (listNode) {
                MarkdownNode *item = CreateNode(impl, MdNodeListItem);
                if (item) {
                    /* Skip list marker */
                    while (*line && (*line == ' ' || *line == '-' || *line == '*' ||
                                     *line == '+' || isdigit((unsigned char)*line) ||
                                     *line == '.')) {
                        line++;
                    }
                    while (*line == ' ') line++;

                    strncpy(item->Text, line, sizeof(item->Text) - 1);
                    AddChild(listNode, item);
                }
            }

            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        /* Blockquote */
        if (*line == '>') {
            MarkdownNode *quote = CreateNode(impl, MdNodeBlockquote);
            if (quote) {
                line++;  /* Skip > */
                while (*line == ' ') line++;
                strncpy(quote->Text, line, sizeof(quote->Text) - 1);
                AddChild(impl->Root, quote);
            }
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        /* Paragraph */
        MarkdownNode *para = CreateNode(impl, MdNodeParagraph);
        if (para) {
            strncpy(para->Text, line, sizeof(para->Text) - 1);
            AddChild(impl->Root, para);
        }

        line = strtok_r(NULL, "\n", &saveptr);
    }

    free(text);
    return S_OK;
}

static HRESULT ANXAPI MarkdownParser_GetRoot(
    IMarkdownParser *This,
    MarkdownNode **Root
)
{
    MarkdownParserImpl *impl = (MarkdownParserImpl *)This;
    if (Root == NULL) return E_POINTER;
    *Root = impl->Root;
    return S_OK;
}

static HRESULT ANXAPI MarkdownParser_RenderToText(
    IMarkdownParser *This,
    CHAR8 *Buffer,
    UINTN BufferSize
)
{
    MarkdownParserImpl *impl = (MarkdownParserImpl *)This;
    UINT32 i;

    if (Buffer == NULL) return E_POINTER;
    if (impl->Root == NULL) return E_FAIL;

    Buffer[0] = '\0';
    UINTN offset = 0;

    /* Simple traversal and rendering */
    for (i = 0; i < impl->Root->ChildCount; i++) {
        MarkdownNode *node = impl->Root->Children[i];
        CHAR8 temp[MAX_TEXT_LENGTH];

        switch (node->Type) {
            case MdNodeHeading:
                snprintf(temp, sizeof(temp), "\n%s\n", node->Text);
                break;
            case MdNodeParagraph:
                snprintf(temp, sizeof(temp), "%s\n\n", node->Text);
                break;
            case MdNodeCodeBlock:
                snprintf(temp, sizeof(temp), "\n    %s\n", node->Text);
                break;
            case MdNodeBlockquote:
                snprintf(temp, sizeof(temp), "  > %s\n", node->Text);
                break;
            case MdNodeHorizontalRule:
                snprintf(temp, sizeof(temp), "\n---\n\n");
                break;
            case MdNodeList:
                snprintf(temp, sizeof(temp), "\n");
                break;
            default:
                temp[0] = '\0';
                break;
        }

        if (offset + strlen(temp) < BufferSize) {
            strcat(Buffer, temp);
            offset += strlen(temp);
        }

        /* Handle list items */
        if (node->Type == MdNodeList) {
            UINT32 j;
            for (j = 0; j < node->ChildCount; j++) {
                MarkdownNode *item = node->Children[j];
                if (node->Ordered) {
                    snprintf(temp, sizeof(temp), "  %d. %s\n", j + 1, item->Text);
                } else {
                    snprintf(temp, sizeof(temp), "  • %s\n", item->Text);
                }
                if (offset + strlen(temp) < BufferSize) {
                    strcat(Buffer, temp);
                    offset += strlen(temp);
                }
            }
        }
    }

    return S_OK;
}

static HRESULT ANXAPI MarkdownParser_GetNodeCount(
    IMarkdownParser *This,
    UINT32 *Count
)
{
    MarkdownParserImpl *impl = (MarkdownParserImpl *)This;
    if (Count == NULL) return E_POINTER;
    *Count = impl->NodeCount;
    return S_OK;
}

/* Vtable */
static CONST IMarkdownParser_Vtbl MarkdownParserVtbl = {
    MarkdownParser_QueryInterface,
    MarkdownParser_AddRef,
    MarkdownParser_Release,
    MarkdownParser_Parse,
    MarkdownParser_GetRoot,
    MarkdownParser_RenderToText,
    MarkdownParser_GetNodeCount
};

/* Factory function */
HRESULT ANXAPI AnxCreateMarkdownParser(OUT IMarkdownParser **Parser)
{
    MarkdownParserImpl *impl;

    if (Parser == NULL) return E_POINTER;

    impl = (MarkdownParserImpl *)calloc(1, sizeof(MarkdownParserImpl));
    if (impl == NULL) {
        *Parser = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &MarkdownParserVtbl;
    impl->RefCount = 1;
    impl->Root = NULL;
    impl->NodeCount = 0;
    impl->SourceText = NULL;

    *Parser = &impl->Interface;
    return S_OK;
}
