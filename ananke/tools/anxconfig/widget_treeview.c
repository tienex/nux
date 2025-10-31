/*
 * widget_treeview.c - Tree View Control
 *
 * Hierarchical tree control with expand/collapse, checkboxes,
 * inline editing, and keyboard navigation.
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_NODE_TEXT 256
#define MAX_CHILDREN 64
#define MAX_VISIBLE_NODES 1000

/* Tree node structure */
typedef struct _TreeNode {
    CHAR8 Text[MAX_NODE_TEXT];
    VOID *UserData;

    /* Tree structure */
    struct _TreeNode *Parent;
    struct _TreeNode *Children[MAX_CHILDREN];
    UINT32 ChildCount;

    /* Display state */
    BOOLEAN Expanded;
    BOOLEAN HasCheckbox;
    UINT8 CheckState;     /* 0=unchecked, 1=partial, 2=checked */
    BOOLEAN Selected;

    /* Inline editing */
    BOOLEAN IsEditing;
    CHAR8 EditBuffer[MAX_NODE_TEXT];

    /* Icon */
    UINT32 Icon;          /* Unicode codepoint for icon */

    /* Callbacks */
    HRESULT (*OnExpand)(struct _TreeNode *Node, VOID *UserData);
    HRESULT (*OnCollapse)(struct _TreeNode *Node, VOID *UserData);
    HRESULT (*OnCheckChanged)(struct _TreeNode *Node, UINT8 CheckState, VOID *UserData);
    HRESULT (*OnTextChanged)(struct _TreeNode *Node, CONST CHAR8 *NewText, VOID *UserData);

} TreeNode;

/* Virtual data callback structure */
typedef struct {
    CHAR8 Text[MAX_NODE_TEXT];
    UINT32 Depth;
    BOOLEAN HasChildren;
    BOOLEAN Expanded;
    BOOLEAN HasCheckbox;
    UINT8 CheckState;
    UINT32 Icon;
} VirtualTreeItemData;

typedef struct {
    ITuiTreeView Interface;
    WIDGET_STATE State;

    /* Root nodes */
    TreeNode *Root;       /* Invisible root node */

    /* Flattened visible nodes (for rendering and navigation) */
    TreeNode *VisibleNodes[MAX_VISIBLE_NODES];
    UINT32 VisibleCount;

    /* Selection and scrolling */
    INT32 SelectedIndex;
    INT32 ScrollOffset;

    /* Display options */
    BOOLEAN ShowRoot;
    BOOLEAN ShowLines;
    BOOLEAN ShowIcons;
    BOOLEAN AllowCheckboxes;
    BOOLEAN AllowEditing;

    /* Virtual mode */
    BOOLEAN VirtualMode;
    UINT32 VirtualItemCount;
    HRESULT (*OnGetVirtualItem)(VOID *UserData, UINT32 Index, VirtualTreeItemData *OutData);
    VOID *VirtualUserData;

    /* Colors */
    TUI_COLOR TreeLineColor;
    TUI_COLOR SelectedBgColor;
    TUI_COLOR SelectedFgColor;

} TuiTreeViewImpl;

/* Helper: Create tree node */
static TreeNode *CreateTreeNode(CONST CHAR8 *Text, VOID *UserData)
{
    TreeNode *node = (TreeNode *)calloc(1, sizeof(TreeNode));
    if (!node) return NULL;

    if (Text) {
        strncpy(node->Text, Text, sizeof(node->Text) - 1);
        node->Text[sizeof(node->Text) - 1] = '\0';
    }

    node->UserData = UserData;
    node->Expanded = FALSE;
    node->HasCheckbox = FALSE;
    node->CheckState = 0;
    node->Icon = 0;

    return node;
}

/* Helper: Free tree node and all children recursively */
static VOID FreeTreeNode(TreeNode *node)
{
    if (!node) return;

    for (UINT32 i = 0; i < node->ChildCount; i++) {
        FreeTreeNode(node->Children[i]);
    }

    free(node);
}

/* Helper: Add child to node */
static HRESULT AddChildNode(TreeNode *parent, TreeNode *child)
{
    if (!parent || !child) return E_INVALIDARG;
    if (parent->ChildCount >= MAX_CHILDREN) return E_OUTOFMEMORY;

    parent->Children[parent->ChildCount++] = child;
    child->Parent = parent;

    return S_OK;
}

/* Helper: Update check state of parent nodes (tristate) */
static VOID UpdateParentCheckState(TreeNode *node)
{
    if (!node || !node->Parent || !node->Parent->HasCheckbox) return;

    TreeNode *parent = node->Parent;
    BOOLEAN allChecked = TRUE;
    BOOLEAN allUnchecked = TRUE;

    for (UINT32 i = 0; i < parent->ChildCount; i++) {
        TreeNode *child = parent->Children[i];
        if (child->HasCheckbox) {
            if (child->CheckState != 2) allChecked = FALSE;
            if (child->CheckState != 0) allUnchecked = FALSE;
        }
    }

    if (allChecked) {
        parent->CheckState = 2;  /* Fully checked */
    } else if (allUnchecked) {
        parent->CheckState = 0;  /* Unchecked */
    } else {
        parent->CheckState = 1;  /* Partial/indeterminate */
    }

    /* Recursively update grandparents */
    UpdateParentCheckState(parent);
}

/* Helper: Recursively build list of visible nodes */
static VOID BuildVisibleList(TuiTreeViewImpl *impl, TreeNode *node, UINT32 *index)
{
    if (!node || *index >= MAX_VISIBLE_NODES) return;

    /* Add this node if it's not the root or if we're showing root */
    if (node != impl->Root || impl->ShowRoot) {
        impl->VisibleNodes[(*index)++] = node;
    }

    /* Add children if node is expanded */
    if (node->Expanded || node == impl->Root) {
        for (UINT32 i = 0; i < node->ChildCount; i++) {
            BuildVisibleList(impl, node->Children[i], index);
        }
    }
}

/* Helper: Rebuild visible nodes list */
static VOID RebuildVisibleList(TuiTreeViewImpl *impl)
{
    UINT32 index = 0;
    BuildVisibleList(impl, impl->Root, &index);
    impl->VisibleCount = index;
}

/* Helper: Get node depth (for indentation) */
static UINT32 GetNodeDepth(TreeNode *node, TreeNode *root)
{
    UINT32 depth = 0;
    while (node && node != root) {
        depth++;
        node = node->Parent;
    }
    return depth;
}

/* IUnknown methods */
static HRESULT ANXAPI TreeView_QueryInterface(
    ITuiTreeView *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiTreeView)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI TreeView_AddRef(ITuiTreeView *This)
{
    TuiTreeViewImpl *impl = (TuiTreeViewImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI TreeView_Release(ITuiTreeView *This)
{
    TuiTreeViewImpl *impl = (TuiTreeViewImpl *)This;
    UINTN count = --impl->State.RefCount;

    if (count == 0) {
        FreeTreeNode(impl->Root);
        free(impl);
    }

    return count;
}

/* Render the tree view */
static HRESULT ANXAPI TreeView_Render(
    ITuiTreeView *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height
)
{
    TuiTreeViewImpl *impl = (TuiTreeViewImpl *)This;

    if (!impl->State.Visible) return S_OK;

    /* Ensure visible list is up to date (non-virtual mode) */
    if (!impl->VirtualMode) {
        RebuildVisibleList(impl);
    }

    /* Render each visible node */
    for (UINT32 i = 0; i < Height && (impl->ScrollOffset + i) < impl->VisibleCount; i++) {
        CHAR8 line[512];
        UINTN pos = 0;
        UINT32 depth = 0;
        BOOLEAN hasChildren = FALSE;
        BOOLEAN expanded = FALSE;
        BOOLEAN hasCheckbox = FALSE;
        UINT8 checkState = 0;
        UINT32 icon = 0;
        CONST CHAR8 *text = NULL;
        BOOLEAN isEditing = FALSE;

        BOOLEAN isSelected = (impl->SelectedIndex == (INT32)(impl->ScrollOffset + i));
        TUI_COLOR fg = isSelected ? impl->SelectedFgColor : impl->State.ForegroundColor;
        TUI_COLOR bg = isSelected ? impl->SelectedBgColor : impl->State.BackgroundColor;

        /* Get data from virtual mode or normal mode */
        if (impl->VirtualMode) {
            if (impl->OnGetVirtualItem) {
                VirtualTreeItemData virtualData;
                if (SUCCEEDED(impl->OnGetVirtualItem(impl->VirtualUserData, impl->ScrollOffset + i, &virtualData))) {
                    text = virtualData.Text;
                    depth = virtualData.Depth;
                    hasChildren = virtualData.HasChildren;
                    expanded = virtualData.Expanded;
                    hasCheckbox = virtualData.HasCheckbox;
                    checkState = virtualData.CheckState;
                    icon = virtualData.Icon;
                } else {
                    continue;
                }
            } else {
                continue;
            }
        } else {
            TreeNode *node = impl->VisibleNodes[impl->ScrollOffset + i];
            if (!node) continue;

            text = node->IsEditing ? node->EditBuffer : node->Text;
            depth = GetNodeDepth(node, impl->Root);
            if (!impl->ShowRoot) depth--;
            hasChildren = (node->ChildCount > 0);
            expanded = node->Expanded;
            hasCheckbox = node->HasCheckbox;
            checkState = node->CheckState;
            icon = node->Icon;
            isEditing = node->IsEditing;
        }

        /* Indent */
        for (UINT32 d = 0; d < depth && pos < sizeof(line) - 1; d++) {
            if (impl->ShowLines) {
                line[pos++] = ' ';
                line[pos++] = ' ';
                line[pos++] = 0x2502;  /* │ */
                line[pos++] = ' ';
            } else {
                line[pos++] = ' ';
                line[pos++] = ' ';
            }
        }

        /* Expand/collapse indicator */
        if (hasChildren) {
            if (expanded) {
                line[pos++] = 0x25BC;  /* ▼ */
            } else {
                line[pos++] = 0x25B6;  /* ▶ */
            }
            line[pos++] = ' ';
        } else {
            line[pos++] = ' ';
            line[pos++] = ' ';
        }

        /* Checkbox */
        if (impl->AllowCheckboxes && hasCheckbox) {
            line[pos++] = '[';
            if (checkState == 0) {
                line[pos++] = ' ';  /* Unchecked */
            } else if (checkState == 1) {
                line[pos++] = '~';  /* Partial */
            } else {
                line[pos++] = 'X';  /* Checked */
            }
            line[pos++] = ']';
            line[pos++] = ' ';
        }

        /* Icon */
        if (impl->ShowIcons && icon) {
            line[pos++] = (CHAR8)icon;
            line[pos++] = ' ';
        }

        /* Node text */
        UINTN textLen = strlen(text);
        UINTN maxText = sizeof(line) - pos - 1;
        if (textLen > maxText) textLen = maxText;
        memcpy(&line[pos], text, textLen);
        pos += textLen;

        /* Editing cursor */
        if (node->IsEditing && isSelected) {
            line[pos++] = '_';
        }

        line[pos] = '\0';

        /* Clear rest of line */
        for (UINTN j = pos; j < Width; j++) {
            line[j] = ' ';
        }
        line[Width] = '\0';

        Screen->Vtbl->WriteText(Screen, X, Y + i, line, fg, bg);
    }

    /* Clear remaining lines */
    CHAR8 emptyLine[512];
    memset(emptyLine, ' ', sizeof(emptyLine));
    emptyLine[Width < sizeof(emptyLine) ? Width : sizeof(emptyLine) - 1] = '\0';

    for (UINT32 i = impl->VisibleCount - impl->ScrollOffset; i < Height; i++) {
        Screen->Vtbl->WriteText(Screen, X, Y + i, emptyLine,
            impl->State.ForegroundColor, impl->State.BackgroundColor);
    }

    return S_OK;
}

/* Handle keyboard input */
static HRESULT ANXAPI TreeView_HandleKey(
    ITuiTreeView *This,
    TUI_KEY Key
)
{
    TuiTreeViewImpl *impl = (TuiTreeViewImpl *)This;

    if (impl->VisibleCount == 0) return S_OK;

    TreeNode *selectedNode = (impl->SelectedIndex >= 0 && impl->SelectedIndex < (INT32)impl->VisibleCount) ?
        impl->VisibleNodes[impl->SelectedIndex] : NULL;

    /* Handle editing mode */
    if (selectedNode && selectedNode->IsEditing) {
        if (Key == TuiKeyEnter || Key == TuiKeyEscape) {
            if (Key == TuiKeyEnter) {
                /* Commit edit */
                strncpy(selectedNode->Text, selectedNode->EditBuffer, sizeof(selectedNode->Text) - 1);
                selectedNode->Text[sizeof(selectedNode->Text) - 1] = '\0';

                if (selectedNode->OnTextChanged) {
                    selectedNode->OnTextChanged(selectedNode, selectedNode->Text, selectedNode->UserData);
                }
            }
            selectedNode->IsEditing = FALSE;
            return S_OK;
        } else if (Key == TuiKeyBackspace) {
            UINTN len = strlen(selectedNode->EditBuffer);
            if (len > 0) {
                selectedNode->EditBuffer[len - 1] = '\0';
            }
            return S_OK;
        } else if (Key >= 32 && Key < 127) {
            UINTN len = strlen(selectedNode->EditBuffer);
            if (len < sizeof(selectedNode->EditBuffer) - 1) {
                selectedNode->EditBuffer[len] = (CHAR8)Key;
                selectedNode->EditBuffer[len + 1] = '\0';
            }
            return S_OK;
        }
        return S_OK;
    }

    /* Navigation */
    if (Key == TuiKeyUp) {
        if (impl->SelectedIndex > 0) {
            impl->SelectedIndex--;
            if (impl->SelectedIndex < impl->ScrollOffset) {
                impl->ScrollOffset = impl->SelectedIndex;
            }
        }
        return S_OK;
    }

    if (Key == TuiKeyDown) {
        if (impl->SelectedIndex < (INT32)impl->VisibleCount - 1) {
            impl->SelectedIndex++;
            UINT32 viewHeight = impl->State.Bounds.Height;
            if (impl->SelectedIndex >= impl->ScrollOffset + (INT32)viewHeight) {
                impl->ScrollOffset = impl->SelectedIndex - viewHeight + 1;
            }
        }
        return S_OK;
    }

    if (selectedNode) {
        /* Expand/collapse */
        if (Key == TuiKeyRight && selectedNode->ChildCount > 0) {
            selectedNode->Expanded = TRUE;
            if (selectedNode->OnExpand) {
                selectedNode->OnExpand(selectedNode, selectedNode->UserData);
            }
            return S_OK;
        }

        if (Key == TuiKeyLeft) {
            if (selectedNode->Expanded && selectedNode->ChildCount > 0) {
                selectedNode->Expanded = FALSE;
                if (selectedNode->OnCollapse) {
                    selectedNode->OnCollapse(selectedNode, selectedNode->UserData);
                }
            } else if (selectedNode->Parent && selectedNode->Parent != impl->Root) {
                /* Navigate to parent */
                for (UINT32 i = 0; i < impl->VisibleCount; i++) {
                    if (impl->VisibleNodes[i] == selectedNode->Parent) {
                        impl->SelectedIndex = i;
                        if (impl->SelectedIndex < impl->ScrollOffset) {
                            impl->ScrollOffset = impl->SelectedIndex;
                        }
                        break;
                    }
                }
            }
            return S_OK;
        }

        /* Toggle checkbox */
        if (Key == ' ' && impl->AllowCheckboxes && selectedNode->HasCheckbox) {
            selectedNode->CheckState = (selectedNode->CheckState + 1) % 3;

            /* Update children recursively */
            /* (Simplified - in full implementation would recursively set children) */

            UpdateParentCheckState(selectedNode);

            if (selectedNode->OnCheckChanged) {
                selectedNode->OnCheckChanged(selectedNode, selectedNode->CheckState, selectedNode->UserData);
            }
            return S_OK;
        }

        /* Start editing */
        if (Key == TuiKeyF2 && impl->AllowEditing) {
            selectedNode->IsEditing = TRUE;
            strncpy(selectedNode->EditBuffer, selectedNode->Text, sizeof(selectedNode->EditBuffer) - 1);
            selectedNode->EditBuffer[sizeof(selectedNode->EditBuffer) - 1] = '\0';
            return S_OK;
        }
    }

    return S_OK;
}

/* Add root node */
static HRESULT ANXAPI TreeView_AddNode(
    ITuiTreeView *This,
    CONST CHAR8 *Text,
    VOID *UserData,
    VOID **OutHandle
)
{
    TuiTreeViewImpl *impl = (TuiTreeViewImpl *)This;

    TreeNode *node = CreateTreeNode(Text, UserData);
    if (!node) return E_OUTOFMEMORY;

    HRESULT hr = AddChildNode(impl->Root, node);
    if (FAILED(hr)) {
        FreeTreeNode(node);
        return hr;
    }

    if (OutHandle) {
        *OutHandle = node;
    }

    return S_OK;
}

/* Add child node */
static HRESULT ANXAPI TreeView_AddChildNode(
    ITuiTreeView *This,
    VOID *ParentHandle,
    CONST CHAR8 *Text,
    VOID *UserData,
    VOID **OutHandle
)
{
    TreeNode *parent = (TreeNode *)ParentHandle;
    if (!parent) return E_INVALIDARG;

    TreeNode *node = CreateTreeNode(Text, UserData);
    if (!node) return E_OUTOFMEMORY;

    HRESULT hr = AddChildNode(parent, node);
    if (FAILED(hr)) {
        FreeTreeNode(node);
        return hr;
    }

    if (OutHandle) {
        *OutHandle = node;
    }

    return S_OK;
}

/* Set node checkbox */
static HRESULT ANXAPI TreeView_SetNodeCheckbox(
    ITuiTreeView *This,
    VOID *NodeHandle,
    BOOLEAN HasCheckbox,
    UINT8 CheckState
)
{
    TreeNode *node = (TreeNode *)NodeHandle;
    if (!node) return E_INVALIDARG;

    node->HasCheckbox = HasCheckbox;
    node->CheckState = CheckState;

    return S_OK;
}

/* Set node icon */
static HRESULT ANXAPI TreeView_SetNodeIcon(
    ITuiTreeView *This,
    VOID *NodeHandle,
    UINT32 Icon
)
{
    TreeNode *node = (TreeNode *)NodeHandle;
    if (!node) return E_INVALIDARG;

    node->Icon = Icon;
    return S_OK;
}

/* Expand/collapse node */
static HRESULT ANXAPI TreeView_ExpandNode(
    ITuiTreeView *This,
    VOID *NodeHandle,
    BOOLEAN Expand
)
{
    TreeNode *node = (TreeNode *)NodeHandle;
    if (!node) return E_INVALIDARG;

    node->Expanded = Expand;
    return S_OK;
}

/* Clear all nodes */
static HRESULT ANXAPI TreeView_Clear(ITuiTreeView *This)
{
    TuiTreeViewImpl *impl = (TuiTreeViewImpl *)This;

    /* Free all children of root */
    for (UINT32 i = 0; i < impl->Root->ChildCount; i++) {
        FreeTreeNode(impl->Root->Children[i]);
    }
    impl->Root->ChildCount = 0;

    impl->SelectedIndex = 0;
    impl->ScrollOffset = 0;
    impl->VisibleCount = 0;

    return S_OK;
}

/* Widget common methods */
static HRESULT ANXAPI TreeView_SetBounds(ITuiTreeView *This, CONST TUI_RECT *Bounds)
{
    TuiTreeViewImpl *impl = (TuiTreeViewImpl *)This;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI TreeView_GetBounds(ITuiTreeView *This, TUI_RECT *Bounds)
{
    TuiTreeViewImpl *impl = (TuiTreeViewImpl *)This;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI TreeView_SetVisible(ITuiTreeView *This, BOOLEAN Visible)
{
    TuiTreeViewImpl *impl = (TuiTreeViewImpl *)This;
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI TreeView_IsVisible(ITuiTreeView *This)
{
    TuiTreeViewImpl *impl = (TuiTreeViewImpl *)This;
    return impl->State.Visible;
}

static HRESULT ANXAPI TreeView_SetEnabled(ITuiTreeView *This, BOOLEAN Enabled)
{
    TuiTreeViewImpl *impl = (TuiTreeViewImpl *)This;
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI TreeView_IsEnabled(ITuiTreeView *This)
{
    TuiTreeViewImpl *impl = (TuiTreeViewImpl *)This;
    return impl->State.Enabled;
}

/* Enable virtual mode */
static HRESULT ANXAPI TreeView_SetVirtualMode(
    ITuiTreeView *This,
    BOOLEAN Enable,
    UINT32 ItemCount,
    HRESULT (*Callback)(VOID *UserData, UINT32 Index, VOID *OutData),
    VOID *UserData
)
{
    TuiTreeViewImpl *impl = (TuiTreeViewImpl *)This;

    impl->VirtualMode = Enable;
    impl->VirtualItemCount = ItemCount;
    impl->OnGetVirtualItem = (HRESULT (*)(VOID *, UINT32, VirtualTreeItemData *))Callback;
    impl->VirtualUserData = UserData;

    if (Enable) {
        impl->VisibleCount = ItemCount;
    }

    return S_OK;
}

/* VTable */
static ITuiTreeViewVtbl TreeViewVtbl = {
    TreeView_QueryInterface,
    TreeView_AddRef,
    TreeView_Release,
    TreeView_Render,
    TreeView_HandleKey,
    TreeView_SetBounds,
    TreeView_GetBounds,
    TreeView_SetVisible,
    TreeView_IsVisible,
    TreeView_SetEnabled,
    TreeView_IsEnabled,
    TreeView_AddNode,
    TreeView_AddChildNode,
    TreeView_SetNodeCheckbox,
    TreeView_SetNodeIcon,
    TreeView_ExpandNode,
    TreeView_Clear,
    TreeView_SetVirtualMode
};

/* Factory function */
HRESULT AnxTuiCreateTreeView(ITuiTreeView **OutTreeView)
{
    TuiTreeViewImpl *impl;

    if (!OutTreeView) return E_INVALIDARG;

    impl = (TuiTreeViewImpl *)malloc(sizeof(TuiTreeViewImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiTreeViewImpl));
    impl->Interface.Vtbl = &TreeViewVtbl;
    InitWidgetState(&impl->State);

    /* Create invisible root node */
    impl->Root = CreateTreeNode(NULL, NULL);
    if (!impl->Root) {
        free(impl);
        return E_OUTOFMEMORY;
    }
    impl->Root->Expanded = TRUE;  /* Root is always expanded */

    impl->ShowRoot = FALSE;
    impl->ShowLines = TRUE;
    impl->ShowIcons = TRUE;
    impl->AllowCheckboxes = TRUE;
    impl->AllowEditing = TRUE;

    impl->TreeLineColor = TuiColorBrightBlack;
    impl->SelectedBgColor = TuiColorCyan;
    impl->SelectedFgColor = TuiColorBlack;

    *OutTreeView = &impl->Interface;
    return S_OK;
}
