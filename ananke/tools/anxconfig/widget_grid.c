/*
 * widget_grid.c - Grid Layout Container Widget
 *
 * Grid layout container that arranges children in rows and columns.
 * Supports spanning multiple rows/columns, padding, and alignment.
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_GRID_CHILDREN 256
#define MAX_GRID_ROWS 32
#define MAX_GRID_COLS 32

/* Grid child placement */
typedef struct {
    VOID *Widget;           /* Child widget */
    UINT32 Row;             /* Starting row */
    UINT32 Column;          /* Starting column */
    UINT32 RowSpan;         /* Number of rows to span */
    UINT32 ColumnSpan;      /* Number of columns to span */
    UINT32 Padding;         /* Padding around this child */

    /* Alignment within cell */
    UINT8 HAlign;           /* 0=left, 1=center, 2=right, 3=fill */
    UINT8 VAlign;           /* 0=top, 1=center, 2=bottom, 3=fill */
} GridChild;

typedef struct {
    ITuiGrid Interface;
    WIDGET_STATE State;

    /* Children */
    GridChild Children[MAX_GRID_CHILDREN];
    UINT32 ChildCount;

    /* Grid dimensions */
    UINT32 Rows;
    UINT32 Columns;

    /* Row/column sizes */
    UINT32 RowHeights[MAX_GRID_ROWS];      /* 0 = auto */
    UINT32 ColumnWidths[MAX_GRID_COLS];    /* 0 = auto */

    /* Spacing */
    UINT32 RowSpacing;
    UINT32 ColumnSpacing;

    /* Homogeneous */
    BOOLEAN HomogeneousRows;
    BOOLEAN HomogeneousColumns;

} TuiGridImpl;

/* IUnknown methods */
static HRESULT ANXAPI Grid_QueryInterface(
    ITuiGrid *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiGrid)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI Grid_AddRef(ITuiGrid *This)
{
    TuiGridImpl *impl = (TuiGridImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Grid_Release(ITuiGrid *This)
{
    TuiGridImpl *impl = (TuiGridImpl *)This;
    UINTN count = --impl->State.RefCount;

    if (count == 0) {
        /* Release all child widgets */
        for (UINT32 i = 0; i < impl->ChildCount; i++) {
            if (impl->Children[i].Widget) {
                IUnknown *unk = (IUnknown *)impl->Children[i].Widget;
                unk->Vtbl->Release(unk);
            }
        }
        free(impl);
    }

    return count;
}

/* Calculate grid layout */
static VOID CalculateGridLayout(
    TuiGridImpl *impl,
    UINT32 *rowHeights,
    UINT32 *columnWidths
)
{
    UINT32 availableWidth = impl->State.Bounds.Width;
    UINT32 availableHeight = impl->State.Bounds.Height;

    /* Subtract spacing */
    UINT32 totalColSpacing = (impl->Columns > 1) ? (impl->ColumnSpacing * (impl->Columns - 1)) : 0;
    UINT32 totalRowSpacing = (impl->Rows > 1) ? (impl->RowSpacing * (impl->Rows - 1)) : 0;

    availableWidth -= totalColSpacing;
    availableHeight -= totalRowSpacing;

    if (impl->HomogeneousColumns) {
        /* Equal width columns */
        UINT32 colWidth = availableWidth / impl->Columns;
        for (UINT32 i = 0; i < impl->Columns; i++) {
            columnWidths[i] = colWidth;
        }
    } else {
        /* Use specified widths or distribute auto columns */
        UINT32 fixedWidth = 0;
        UINT32 autoColumns = 0;

        for (UINT32 i = 0; i < impl->Columns; i++) {
            if (impl->ColumnWidths[i] > 0) {
                columnWidths[i] = impl->ColumnWidths[i];
                fixedWidth += impl->ColumnWidths[i];
            } else {
                autoColumns++;
            }
        }

        /* Distribute remaining width to auto columns */
        if (autoColumns > 0 && availableWidth > fixedWidth) {
            UINT32 autoWidth = (availableWidth - fixedWidth) / autoColumns;
            for (UINT32 i = 0; i < impl->Columns; i++) {
                if (impl->ColumnWidths[i] == 0) {
                    columnWidths[i] = autoWidth;
                }
            }
        }
    }

    if (impl->HomogeneousRows) {
        /* Equal height rows */
        UINT32 rowHeight = availableHeight / impl->Rows;
        for (UINT32 i = 0; i < impl->Rows; i++) {
            rowHeights[i] = rowHeight;
        }
    } else {
        /* Use specified heights or distribute auto rows */
        UINT32 fixedHeight = 0;
        UINT32 autoRows = 0;

        for (UINT32 i = 0; i < impl->Rows; i++) {
            if (impl->RowHeights[i] > 0) {
                rowHeights[i] = impl->RowHeights[i];
                fixedHeight += impl->RowHeights[i];
            } else {
                autoRows++;
            }
        }

        /* Distribute remaining height to auto rows */
        if (autoRows > 0 && availableHeight > fixedHeight) {
            UINT32 autoHeight = (availableHeight - fixedHeight) / autoRows;
            for (UINT32 i = 0; i < impl->Rows; i++) {
                if (impl->RowHeights[i] == 0) {
                    rowHeights[i] = autoHeight;
                }
            }
        }
    }
}

/* Render the grid */
static HRESULT ANXAPI Grid_Render(
    ITuiGrid *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiGridImpl *impl = (TuiGridImpl *)This;

    if (!impl->State.Visible) return S_OK;
    if (impl->ChildCount == 0) return S_OK;

    /* Calculate layout */
    UINT32 rowHeights[MAX_GRID_ROWS] = {0};
    UINT32 columnWidths[MAX_GRID_COLS] = {0};
    CalculateGridLayout(impl, rowHeights, columnWidths);

    /* Calculate row/column positions */
    UINT32 rowPositions[MAX_GRID_ROWS];
    UINT32 columnPositions[MAX_GRID_COLS];

    rowPositions[0] = impl->State.Bounds.Y;
    for (UINT32 i = 1; i < impl->Rows; i++) {
        rowPositions[i] = rowPositions[i - 1] + rowHeights[i - 1] + impl->RowSpacing;
    }

    columnPositions[0] = impl->State.Bounds.X;
    for (UINT32 i = 1; i < impl->Columns; i++) {
        columnPositions[i] = columnPositions[i - 1] + columnWidths[i - 1] + impl->ColumnSpacing;
    }

    /* Render children */
    for (UINT32 i = 0; i < impl->ChildCount; i++) {
        GridChild *child = &impl->Children[i];
        if (!child->Widget) continue;

        /* Calculate cell bounds */
        UINT32 cellX = columnPositions[child->Column];
        UINT32 cellY = rowPositions[child->Row];

        /* Calculate cell size including spans */
        UINT32 cellWidth = 0;
        for (UINT32 c = 0; c < child->ColumnSpan; c++) {
            if (child->Column + c < impl->Columns) {
                cellWidth += columnWidths[child->Column + c];
                if (c > 0) cellWidth += impl->ColumnSpacing;
            }
        }

        UINT32 cellHeight = 0;
        for (UINT32 r = 0; r < child->RowSpan; r++) {
            if (child->Row + r < impl->Rows) {
                cellHeight += rowHeights[child->Row + r];
                if (r > 0) cellHeight += impl->RowSpacing;
            }
        }

        /* Apply padding */
        TUI_RECT childBounds;
        childBounds.X = cellX + child->Padding;
        childBounds.Y = cellY + child->Padding;
        childBounds.Width = cellWidth - (2 * child->Padding);
        childBounds.Height = cellHeight - (2 * child->Padding);

        /* Apply alignment */
        if (child->HAlign != 3) {  /* Not fill */
            UINT32 prefWidth = childBounds.Width / 2;  /* Preferred width */
            switch (child->HAlign) {
                case 0:  /* Left */
                    childBounds.Width = prefWidth;
                    break;
                case 1:  /* Center */
                    childBounds.X += (childBounds.Width - prefWidth) / 2;
                    childBounds.Width = prefWidth;
                    break;
                case 2:  /* Right */
                    childBounds.X += childBounds.Width - prefWidth;
                    childBounds.Width = prefWidth;
                    break;
            }
        }

        if (child->VAlign != 3) {  /* Not fill */
            UINT32 prefHeight = childBounds.Height / 2;  /* Preferred height */
            switch (child->VAlign) {
                case 0:  /* Top */
                    childBounds.Height = prefHeight;
                    break;
                case 1:  /* Center */
                    childBounds.Y += (childBounds.Height - prefHeight) / 2;
                    childBounds.Height = prefHeight;
                    break;
                case 2:  /* Bottom */
                    childBounds.Y += childBounds.Height - prefHeight;
                    childBounds.Height = prefHeight;
                    break;
            }
        }

        /* Set bounds and render */
        ITuiButton *widget = (ITuiButton *)child->Widget;
        if (widget->Vtbl->SetBounds) {
            widget->Vtbl->SetBounds(widget, &childBounds);
        }
        if (widget->Vtbl->Render) {
            widget->Vtbl->Render(widget, Screen, childBounds.X, childBounds.Y, FALSE);
        }
    }

    return S_OK;
}

/* Attach a child widget to the grid */
static HRESULT ANXAPI Grid_Attach(
    ITuiGrid *This,
    VOID *Widget,
    UINT32 Column,
    UINT32 Row,
    UINT32 ColumnSpan,
    UINT32 RowSpan
)
{
    TuiGridImpl *impl = (TuiGridImpl *)This;

    if (impl->ChildCount >= MAX_GRID_CHILDREN) {
        return E_OUTOFMEMORY;
    }

    if (!Widget) return E_INVALIDARG;
    if (Column >= impl->Columns || Row >= impl->Rows) return E_INVALIDARG;

    GridChild *child = &impl->Children[impl->ChildCount++];
    child->Widget = Widget;
    child->Column = Column;
    child->Row = Row;
    child->ColumnSpan = (ColumnSpan == 0) ? 1 : ColumnSpan;
    child->RowSpan = (RowSpan == 0) ? 1 : RowSpan;
    child->Padding = 0;
    child->HAlign = 3;  /* Fill */
    child->VAlign = 3;  /* Fill */

    /* AddRef the widget */
    IUnknown *unk = (IUnknown *)Widget;
    unk->Vtbl->AddRef(unk);

    return S_OK;
}

/* Set row/column spacing */
static HRESULT ANXAPI Grid_SetSpacing(
    ITuiGrid *This,
    UINT32 RowSpacing,
    UINT32 ColumnSpacing
)
{
    TuiGridImpl *impl = (TuiGridImpl *)This;
    impl->RowSpacing = RowSpacing;
    impl->ColumnSpacing = ColumnSpacing;
    return S_OK;
}

/* Set row height */
static HRESULT ANXAPI Grid_SetRowHeight(
    ITuiGrid *This,
    UINT32 Row,
    UINT32 Height
)
{
    TuiGridImpl *impl = (TuiGridImpl *)This;
    if (Row >= impl->Rows) return E_INVALIDARG;

    impl->RowHeights[Row] = Height;
    return S_OK;
}

/* Set column width */
static HRESULT ANXAPI Grid_SetColumnWidth(
    ITuiGrid *This,
    UINT32 Column,
    UINT32 Width
)
{
    TuiGridImpl *impl = (TuiGridImpl *)This;
    if (Column >= impl->Columns) return E_INVALIDARG;

    impl->ColumnWidths[Column] = Width;
    return S_OK;
}

/* Widget common methods */
static HRESULT ANXAPI Grid_SetBounds(ITuiGrid *This, CONST TUI_RECT *Bounds)
{
    TuiGridImpl *impl = (TuiGridImpl *)This;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI Grid_GetBounds(ITuiGrid *This, TUI_RECT *Bounds)
{
    TuiGridImpl *impl = (TuiGridImpl *)This;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI Grid_SetVisible(ITuiGrid *This, BOOLEAN Visible)
{
    TuiGridImpl *impl = (TuiGridImpl *)This;
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI Grid_IsVisible(ITuiGrid *This)
{
    TuiGridImpl *impl = (TuiGridImpl *)This;
    return impl->State.Visible;
}

static HRESULT ANXAPI Grid_SetEnabled(ITuiGrid *This, BOOLEAN Enabled)
{
    TuiGridImpl *impl = (TuiGridImpl *)This;
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI Grid_IsEnabled(ITuiGrid *This)
{
    TuiGridImpl *impl = (TuiGridImpl *)This;
    return impl->State.Enabled;
}

/* VTable */
static ITuiGridVtbl GridVtbl = {
    Grid_QueryInterface,
    Grid_AddRef,
    Grid_Release,
    Grid_Render,
    Grid_SetBounds,
    Grid_GetBounds,
    Grid_SetVisible,
    Grid_IsVisible,
    Grid_SetEnabled,
    Grid_IsEnabled,
    Grid_Attach,
    Grid_SetSpacing,
    Grid_SetRowHeight,
    Grid_SetColumnWidth
};

/* Factory function */
HRESULT AnxTuiCreateGrid(
    UINT32 Rows,
    UINT32 Columns,
    ITuiGrid **OutGrid
)
{
    TuiGridImpl *impl;

    if (!OutGrid) return E_INVALIDARG;
    if (Rows == 0 || Rows > MAX_GRID_ROWS) return E_INVALIDARG;
    if (Columns == 0 || Columns > MAX_GRID_COLS) return E_INVALIDARG;

    impl = (TuiGridImpl *)malloc(sizeof(TuiGridImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiGridImpl));
    impl->Interface.Vtbl = &GridVtbl;
    InitWidgetState(&impl->State);

    impl->Rows = Rows;
    impl->Columns = Columns;
    impl->RowSpacing = 0;
    impl->ColumnSpacing = 0;
    impl->HomogeneousRows = FALSE;
    impl->HomogeneousColumns = FALSE;

    impl->State.Bounds.Width = 80;
    impl->State.Bounds.Height = 24;

    *OutGrid = &impl->Interface;
    return S_OK;
}
