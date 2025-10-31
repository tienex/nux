/*
 * widget_listview.c - Multi-Column List View Widget
 *
 * Advanced list control with:
 * - Resizable columns
 * - Multiple view modes (list, details, icons, column browsing)
 * - Alternating row colors
 * - Checkboxes
 * - Inline editing
 * - Sorting
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_COLUMNS 32
#define MAX_ITEMS 10000
#define MAX_CELL_TEXT 256

/* List view mode */
typedef enum {
    ListViewModeList,          /* Simple list */
    ListViewModeDetails,       /* Multi-column details */
    ListViewModeIcons,         /* Icon view */
    ListViewModeColumnBrowse   /* Miller columns */
} ListViewMode;

/* Column definition */
typedef struct {
    CHAR8 Header[128];
    UINT32 Width;
    BOOLEAN Resizable;
    BOOLEAN Sortable;
    INT32 SortOrder;       /* 0=none, 1=ascending, -1=descending */
} ListViewColumn;

/* List item */
typedef struct {
    CHAR8 Cells[MAX_COLUMNS][MAX_CELL_TEXT];
    BOOLEAN Checked;
    BOOLEAN Selected;
    VOID *UserData;

    /* Inline editing */
    BOOLEAN IsEditing;
    UINT32 EditColumn;
    CHAR8 EditBuffer[MAX_CELL_TEXT];

    /* Icon */
    UINT32 Icon;
} ListViewItem;

typedef struct {
    ITuiListView Interface;
    WIDGET_STATE State;

    /* Columns */
    ListViewColumn Columns[MAX_COLUMNS];
    UINT32 ColumnCount;

    /* Items */
    ListViewItem **Items;      /* Dynamic array */
    UINT32 ItemCount;
    UINT32 ItemCapacity;

    /* Display mode */
    ListViewMode Mode;

    /* Selection and scrolling */
    INT32 SelectedIndex;
    INT32 ScrollOffsetY;
    INT32 ScrollOffsetX;       /* Horizontal scroll for wide columns */

    /* Display options */
    BOOLEAN ShowHeaders;
    BOOLEAN ShowCheckboxes;
    BOOLEAN AllowEditing;
    BOOLEAN AlternatingColors;
    BOOLEAN FullRowSelect;

    /* Column resizing */
    INT32 ResizingColumn;      /* -1 if not resizing */
    INT32 ResizeStartX;

    /* Colors */
    TUI_COLOR HeaderBgColor;
    TUI_COLOR HeaderFgColor;
    TUI_COLOR SelectedBgColor;
    TUI_COLOR SelectedFgColor;
    TUI_COLOR AlternateBgColor;

    /* Callbacks */
    HRESULT (*OnItemSelected)(VOID *UserData, UINT32 Index);
    VOID *CallbackUserData;

} TuiListViewImpl;

/* Helper: Allocate item */
static ListViewItem *AllocateItem(VOID)
{
    ListViewItem *item = (ListViewItem *)calloc(1, sizeof(ListViewItem));
    return item;
}

/* Helper: Ensure capacity for items */
static HRESULT EnsureItemCapacity(TuiListViewImpl *impl, UINT32 MinCapacity)
{
    if (impl->ItemCapacity >= MinCapacity) {
        return S_OK;
    }

    UINT32 newCapacity = impl->ItemCapacity == 0 ? 16 : impl->ItemCapacity * 2;
    while (newCapacity < MinCapacity) {
        newCapacity *= 2;
    }

    ListViewItem **newItems = (ListViewItem **)realloc(impl->Items, newCapacity * sizeof(ListViewItem *));
    if (!newItems) {
        return E_OUTOFMEMORY;
    }

    impl->Items = newItems;
    impl->ItemCapacity = newCapacity;

    return S_OK;
}

/* IUnknown methods */
static HRESULT ANXAPI ListView_QueryInterface(
    ITuiListView *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiListView)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI ListView_AddRef(ITuiListView *This)
{
    TuiListViewImpl *impl = (TuiListViewImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ListView_Release(ITuiListView *This)
{
    TuiListViewImpl *impl = (TuiListViewImpl *)This;
    UINTN count = --impl->State.RefCount;

    if (count == 0) {
        for (UINT32 i = 0; i < impl->ItemCount; i++) {
            free(impl->Items[i]);
        }
        free(impl->Items);
        free(impl);
    }

    return count;
}

/* Render the list view */
static HRESULT ANXAPI ListView_Render(
    ITuiListView *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height
)
{
    TuiListViewImpl *impl = (TuiListViewImpl *)This;

    if (!impl->State.Visible) return S_OK;

    UINT32 currentY = Y;

    /* Render column headers */
    if (impl->ShowHeaders && (impl->Mode == ListViewModeDetails || impl->Mode == ListViewModeList)) {
        CHAR8 headerLine[1024];
        UINTN pos = 0;

        /* Checkbox column */
        if (impl->ShowCheckboxes) {
            headerLine[pos++] = ' ';
            headerLine[pos++] = ' ';
            headerLine[pos++] = ' ';
            headerLine[pos++] = ' ';
        }

        /* Column headers */
        for (UINT32 col = 0; col < impl->ColumnCount && pos < sizeof(headerLine) - 10; col++) {
            UINTN headerLen = strlen(impl->Columns[col].Header);
            UINT32 colWidth = impl->Columns[col].Width;

            /* Truncate if needed */
            if (headerLen > colWidth - 2) {
                headerLen = colWidth - 2;
            }

            memcpy(&headerLine[pos], impl->Columns[col].Header, headerLen);
            pos += headerLen;

            /* Sort indicator */
            if (impl->Columns[col].SortOrder == 1) {
                headerLine[pos++] = 0x25B2;  /* ▲ */
            } else if (impl->Columns[col].SortOrder == -1) {
                headerLine[pos++] = 0x25BC;  /* ▼ */
            }

            /* Padding */
            UINTN padding = colWidth - headerLen - (impl->Columns[col].SortOrder != 0 ? 1 : 0);
            for (UINTN p = 0; p < padding && pos < sizeof(headerLine) - 1; p++) {
                headerLine[pos++] = ' ';
            }

            /* Column separator */
            if (col < impl->ColumnCount - 1) {
                headerLine[pos++] = 0x2502;  /* │ */
            }
        }

        headerLine[pos] = '\0';
        Screen->Vtbl->WriteText(Screen, X, currentY, headerLine,
            impl->HeaderFgColor, impl->HeaderBgColor);
        currentY++;

        /* Header separator line */
        for (UINT32 x = 0; x < Width; x++) {
            Screen->Vtbl->WriteChar(Screen, X + x, currentY, 0x2500, /* ─ */
                impl->TreeLineColor, impl->State.BackgroundColor);
        }
        currentY++;
    }

    /* Render items */
    UINT32 visibleHeight = Height - (currentY - Y);
    for (UINT32 i = 0; i < visibleHeight && (impl->ScrollOffsetY + i) < impl->ItemCount; i++) {
        ListViewItem *item = impl->Items[impl->ScrollOffsetY + i];
        if (!item) continue;

        BOOLEAN isSelected = (impl->SelectedIndex == (INT32)(impl->ScrollOffsetY + i));
        BOOLEAN isAlternate = ((impl->ScrollOffsetY + i) % 2 == 1);

        TUI_COLOR fg, bg;
        if (isSelected && impl->FullRowSelect) {
            fg = impl->SelectedFgColor;
            bg = impl->SelectedBgColor;
        } else {
            fg = impl->State.ForegroundColor;
            bg = (impl->AlternatingColors && isAlternate) ? impl->AlternateBgColor : impl->State.BackgroundColor;
        }

        CHAR8 line[1024];
        UINTN pos = 0;

        /* Checkbox */
        if (impl->ShowCheckboxes) {
            line[pos++] = ' ';
            line[pos++] = '[';
            line[pos++] = item->Checked ? 'X' : ' ';
            line[pos++] = ']';
        }

        /* Cells */
        for (UINT32 col = 0; col < impl->ColumnCount && pos < sizeof(line) - 10; col++) {
            CONST CHAR8 *text = (item->IsEditing && item->EditColumn == col) ?
                item->EditBuffer : item->Cells[col];

            UINTN textLen = strlen(text);
            UINT32 colWidth = impl->Columns[col].Width;

            /* Truncate if needed */
            if (textLen > colWidth - 2) {
                textLen = colWidth - 2;
            }

            memcpy(&line[pos], text, textLen);
            pos += textLen;

            /* Editing cursor */
            if (item->IsEditing && item->EditColumn == col && isSelected) {
                line[pos++] = '_';
                colWidth--;
            }

            /* Padding */
            UINTN padding = colWidth - textLen;
            for (UINTN p = 0; p < padding && pos < sizeof(line) - 1; p++) {
                line[pos++] = ' ';
            }

            /* Column separator */
            if (col < impl->ColumnCount - 1) {
                line[pos++] = 0x2502;  /* │ */
            }
        }

        /* Fill rest of line */
        while (pos < Width && pos < sizeof(line) - 1) {
            line[pos++] = ' ';
        }
        line[pos] = '\0';

        Screen->Vtbl->WriteText(Screen, X, currentY + i, line, fg, bg);
    }

    /* Clear remaining lines */
    CHAR8 emptyLine[1024];
    memset(emptyLine, ' ', sizeof(emptyLine));
    emptyLine[Width < sizeof(emptyLine) ? Width : sizeof(emptyLine) - 1] = '\0';

    for (UINT32 i = impl->ItemCount - impl->ScrollOffsetY; i < visibleHeight; i++) {
        Screen->Vtbl->WriteText(Screen, X, currentY + i, emptyLine,
            impl->State.ForegroundColor, impl->State.BackgroundColor);
    }

    return S_OK;
}

/* Handle keyboard input */
static HRESULT ANXAPI ListView_HandleKey(
    ITuiListView *This,
    TUI_KEY Key
)
{
    TuiListViewImpl *impl = (TuiListViewImpl *)This;

    if (impl->ItemCount == 0) return S_OK;

    ListViewItem *selectedItem = (impl->SelectedIndex >= 0 && impl->SelectedIndex < (INT32)impl->ItemCount) ?
        impl->Items[impl->SelectedIndex] : NULL;

    /* Handle editing mode */
    if (selectedItem && selectedItem->IsEditing) {
        if (Key == TuiKeyEnter || Key == TuiKeyEscape) {
            if (Key == TuiKeyEnter) {
                /* Commit edit */
                strncpy(selectedItem->Cells[selectedItem->EditColumn], selectedItem->EditBuffer,
                       sizeof(selectedItem->Cells[selectedItem->EditColumn]) - 1);
                selectedItem->Cells[selectedItem->EditColumn][sizeof(selectedItem->Cells[selectedItem->EditColumn]) - 1] = '\0';
            }
            selectedItem->IsEditing = FALSE;
            return S_OK;
        } else if (Key == TuiKeyBackspace) {
            UINTN len = strlen(selectedItem->EditBuffer);
            if (len > 0) {
                selectedItem->EditBuffer[len - 1] = '\0';
            }
            return S_OK;
        } else if (Key >= 32 && Key < 127) {
            UINTN len = strlen(selectedItem->EditBuffer);
            if (len < sizeof(selectedItem->EditBuffer) - 1) {
                selectedItem->EditBuffer[len] = (CHAR8)Key;
                selectedItem->EditBuffer[len + 1] = '\0';
            }
            return S_OK;
        }
        return S_OK;
    }

    /* Navigation */
    if (Key == TuiKeyUp) {
        if (impl->SelectedIndex > 0) {
            impl->SelectedIndex--;
            if (impl->SelectedIndex < impl->ScrollOffsetY) {
                impl->ScrollOffsetY = impl->SelectedIndex;
            }

            if (impl->OnItemSelected) {
                impl->OnItemSelected(impl->CallbackUserData, impl->SelectedIndex);
            }
        }
        return S_OK;
    }

    if (Key == TuiKeyDown) {
        if (impl->SelectedIndex < (INT32)impl->ItemCount - 1) {
            impl->SelectedIndex++;
            UINT32 viewHeight = impl->State.Bounds.Height - (impl->ShowHeaders ? 2 : 0);
            if (impl->SelectedIndex >= impl->ScrollOffsetY + (INT32)viewHeight) {
                impl->ScrollOffsetY = impl->SelectedIndex - viewHeight + 1;
            }

            if (impl->OnItemSelected) {
                impl->OnItemSelected(impl->CallbackUserData, impl->SelectedIndex);
            }
        }
        return S_OK;
    }

    if (Key == TuiKeyPageUp) {
        UINT32 viewHeight = impl->State.Bounds.Height - (impl->ShowHeaders ? 2 : 0);
        impl->SelectedIndex -= viewHeight;
        if (impl->SelectedIndex < 0) impl->SelectedIndex = 0;
        impl->ScrollOffsetY = impl->SelectedIndex;

        if (impl->OnItemSelected) {
            impl->OnItemSelected(impl->CallbackUserData, impl->SelectedIndex);
        }
        return S_OK;
    }

    if (Key == TuiKeyPageDown) {
        UINT32 viewHeight = impl->State.Bounds.Height - (impl->ShowHeaders ? 2 : 0);
        impl->SelectedIndex += viewHeight;
        if (impl->SelectedIndex >= (INT32)impl->ItemCount) {
            impl->SelectedIndex = impl->ItemCount - 1;
        }
        if (impl->SelectedIndex >= impl->ScrollOffsetY + (INT32)viewHeight) {
            impl->ScrollOffsetY = impl->SelectedIndex - viewHeight + 1;
        }

        if (impl->OnItemSelected) {
            impl->OnItemSelected(impl->CallbackUserData, impl->SelectedIndex);
        }
        return S_OK;
    }

    if (selectedItem) {
        /* Toggle checkbox */
        if (Key == ' ' && impl->ShowCheckboxes) {
            selectedItem->Checked = !selectedItem->Checked;
            return S_OK;
        }

        /* Start editing */
        if (Key == TuiKeyF2 && impl->AllowEditing && impl->ColumnCount > 0) {
            selectedItem->IsEditing = TRUE;
            selectedItem->EditColumn = 0;  /* Edit first column by default */
            strncpy(selectedItem->EditBuffer, selectedItem->Cells[0], sizeof(selectedItem->EditBuffer) - 1);
            selectedItem->EditBuffer[sizeof(selectedItem->EditBuffer) - 1] = '\0';
            return S_OK;
        }
    }

    return S_OK;
}

/* Add column */
static HRESULT ANXAPI ListView_AddColumn(
    ITuiListView *This,
    CONST CHAR8 *Header,
    UINT32 Width
)
{
    TuiListViewImpl *impl = (TuiListViewImpl *)This;

    if (impl->ColumnCount >= MAX_COLUMNS) {
        return E_OUTOFMEMORY;
    }

    ListViewColumn *col = &impl->Columns[impl->ColumnCount++];
    strncpy(col->Header, Header ? Header : "", sizeof(col->Header) - 1);
    col->Header[sizeof(col->Header) - 1] = '\0';
    col->Width = Width > 0 ? Width : 20;
    col->Resizable = TRUE;
    col->Sortable = TRUE;
    col->SortOrder = 0;

    return S_OK;
}

/* Add item */
static HRESULT ANXAPI ListView_AddItem(
    ITuiListView *This,
    CONST CHAR8 **Cells,
    UINT32 CellCount,
    VOID *UserData,
    UINT32 *OutIndex
)
{
    TuiListViewImpl *impl = (TuiListViewImpl *)This;

    /* Ensure capacity */
    HRESULT hr = EnsureItemCapacity(impl, impl->ItemCount + 1);
    if (FAILED(hr)) return hr;

    ListViewItem *item = AllocateItem();
    if (!item) return E_OUTOFMEMORY;

    /* Copy cell data */
    for (UINT32 i = 0; i < CellCount && i < MAX_COLUMNS; i++) {
        if (Cells[i]) {
            strncpy(item->Cells[i], Cells[i], sizeof(item->Cells[i]) - 1);
            item->Cells[i][sizeof(item->Cells[i]) - 1] = '\0';
        }
    }

    item->UserData = UserData;

    impl->Items[impl->ItemCount] = item;

    if (OutIndex) {
        *OutIndex = impl->ItemCount;
    }

    impl->ItemCount++;

    return S_OK;
}

/* Clear all items */
static HRESULT ANXAPI ListView_Clear(ITuiListView *This)
{
    TuiListViewImpl *impl = (TuiListViewImpl *)This;

    for (UINT32 i = 0; i < impl->ItemCount; i++) {
        free(impl->Items[i]);
    }

    impl->ItemCount = 0;
    impl->SelectedIndex = 0;
    impl->ScrollOffsetY = 0;

    return S_OK;
}

/* Set view mode */
static HRESULT ANXAPI ListView_SetMode(
    ITuiListView *This,
    UINT32 Mode
)
{
    TuiListViewImpl *impl = (TuiListViewImpl *)This;

    if (Mode > ListViewModeColumnBrowse) {
        return E_INVALIDARG;
    }

    impl->Mode = (ListViewMode)Mode;
    return S_OK;
}

/* Set column width */
static HRESULT ANXAPI ListView_SetColumnWidth(
    ITuiListView *This,
    UINT32 ColumnIndex,
    UINT32 Width
)
{
    TuiListViewImpl *impl = (TuiListViewImpl *)This;

    if (ColumnIndex >= impl->ColumnCount) {
        return E_INVALIDARG;
    }

    impl->Columns[ColumnIndex].Width = Width;
    return S_OK;
}

/* Widget common methods */
static HRESULT ANXAPI ListView_SetBounds(ITuiListView *This, CONST TUI_RECT *Bounds)
{
    TuiListViewImpl *impl = (TuiListViewImpl *)This;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI ListView_GetBounds(ITuiListView *This, TUI_RECT *Bounds)
{
    TuiListViewImpl *impl = (TuiListViewImpl *)This;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI ListView_SetVisible(ITuiListView *This, BOOLEAN Visible)
{
    TuiListViewImpl *impl = (TuiListViewImpl *)This;
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI ListView_IsVisible(ITuiListView *This)
{
    TuiListViewImpl *impl = (TuiListViewImpl *)This;
    return impl->State.Visible;
}

static HRESULT ANXAPI ListView_SetEnabled(ITuiListView *This, BOOLEAN Enabled)
{
    TuiListViewImpl *impl = (TuiListViewImpl *)This;
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI ListView_IsEnabled(ITuiListView *This)
{
    TuiListViewImpl *impl = (TuiListViewImpl *)This;
    return impl->State.Enabled;
}

/* VTable */
static ITuiListViewVtbl ListViewVtbl = {
    ListView_QueryInterface,
    ListView_AddRef,
    ListView_Release,
    ListView_Render,
    ListView_HandleKey,
    ListView_SetBounds,
    ListView_GetBounds,
    ListView_SetVisible,
    ListView_IsVisible,
    ListView_SetEnabled,
    ListView_IsEnabled,
    ListView_AddColumn,
    ListView_AddItem,
    ListView_Clear,
    ListView_SetMode,
    ListView_SetColumnWidth
};

/* Factory function */
HRESULT AnxTuiCreateListView(ITuiListView **OutListView)
{
    TuiListViewImpl *impl;

    if (!OutListView) return E_INVALIDARG;

    impl = (TuiListViewImpl *)malloc(sizeof(TuiListViewImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiListViewImpl));
    impl->Interface.Vtbl = &ListViewVtbl;
    InitWidgetState(&impl->State);

    impl->Mode = ListViewModeDetails;
    impl->ShowHeaders = TRUE;
    impl->ShowCheckboxes = FALSE;
    impl->AllowEditing = TRUE;
    impl->AlternatingColors = TRUE;
    impl->FullRowSelect = TRUE;
    impl->ResizingColumn = -1;

    impl->HeaderBgColor = TuiColorBlue;
    impl->HeaderFgColor = TuiColorWhite;
    impl->SelectedBgColor = TuiColorCyan;
    impl->SelectedFgColor = TuiColorBlack;
    impl->AlternateBgColor = TuiColorBrightBlack;

    *OutListView = &impl->Interface;
    return S_OK;
}
