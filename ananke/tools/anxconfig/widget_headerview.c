/*
 * widget_headerview.c - Header View Widget
 *
 * Reusable column header control for list views, tree views, and spreadsheets.
 * Features:
 * - Multiple columns with titles
 * - Click to sort (with ascending/descending arrows)
 * - Drag to reorder columns
 * - Drag borders to resize columns
 * - Visual feedback (hover, pressed states)
 * - Section highlighting
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_HEADER_SECTIONS 64
#define MAX_SECTION_TITLE 128

/* Sort order */
typedef enum {
    SortNone,
    SortAscending,
    SortDescending
} SortOrder;

/* Header section (column) */
typedef struct {
    CHAR8 Title[MAX_SECTION_TITLE];
    UINT32 Width;
    UINT32 MinWidth;
    UINT32 MaxWidth;
    BOOLEAN Resizable;
    BOOLEAN Clickable;
    SortOrder SortIndicator;
    VOID *UserData;
} HeaderSection;

/* Drag operation */
typedef enum {
    DragNone,
    DragResize,      /* Resizing a column */
    DragReorder      /* Reordering columns */
} DragType;

typedef struct {
    ITuiHeaderView Interface;
    WIDGET_STATE State;

    /* Sections */
    HeaderSection Sections[MAX_HEADER_SECTIONS];
    UINT32 SectionCount;

    /* Visual state */
    INT32 HoverSection;      /* -1 = none */
    INT32 PressedSection;    /* -1 = none */

    /* Drag state */
    DragType DragOperation;
    INT32 DragSection;       /* Section being dragged/resized */
    INT32 DragStartX;        /* Mouse X when drag started */
    UINT32 DragStartWidth;   /* Original width when resizing */

    /* Callbacks */
    HRESULT (*OnSectionClicked)(VOID *UserData, UINT32 SectionIndex);
    HRESULT (*OnSectionResized)(VOID *UserData, UINT32 SectionIndex, UINT32 NewWidth);
    HRESULT (*OnSectionMoved)(VOID *UserData, UINT32 FromIndex, UINT32 ToIndex);
    VOID *UserData;

} TuiHeaderViewImpl;

/* Helper: Get section at X coordinate */
static INT32 GetSectionAtX(TuiHeaderViewImpl *impl, INT32 X)
{
    INT32 currentX = impl->State.Bounds.X;

    for (UINT32 i = 0; i < impl->SectionCount; i++) {
        if (X >= currentX && X < currentX + (INT32)impl->Sections[i].Width) {
            return i;
        }
        currentX += impl->Sections[i].Width;
    }

    return -1;
}

/* Helper: Check if X is on resize border */
static INT32 GetResizeBorderAtX(TuiHeaderViewImpl *impl, INT32 X)
{
    INT32 currentX = impl->State.Bounds.X;

    for (UINT32 i = 0; i < impl->SectionCount; i++) {
        currentX += impl->Sections[i].Width;

        /* Check if within 1 character of border */
        if (X >= currentX - 1 && X <= currentX + 1 && impl->Sections[i].Resizable) {
            return i;
        }
    }

    return -1;
}

/* IUnknown methods */
static HRESULT ANXAPI HeaderView_QueryInterface(
    ITuiHeaderView *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiHeaderView)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI HeaderView_AddRef(ITuiHeaderView *This)
{
    TuiHeaderViewImpl *impl = (TuiHeaderViewImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI HeaderView_Release(ITuiHeaderView *This)
{
    TuiHeaderViewImpl *impl = (TuiHeaderViewImpl *)This;
    UINTN count = --impl->State.RefCount;

    if (count == 0) {
        free(impl);
    }

    return count;
}

/* Render the header view */
static HRESULT ANXAPI HeaderView_Render(
    ITuiHeaderView *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiHeaderViewImpl *impl = (TuiHeaderViewImpl *)This;

    if (!impl->State.Visible) return S_OK;

    UINT32 height = impl->State.Bounds.Height;
    INT32 currentX = X;

    for (UINT32 i = 0; i < impl->SectionCount; i++) {
        HeaderSection *section = &impl->Sections[i];

        /* Determine colors based on state */
        TUI_COLOR bg = TuiColorBrightBlack;
        TUI_COLOR fg = TuiColorWhite;

        if ((INT32)i == impl->PressedSection) {
            bg = TuiColorBlue;
            fg = TuiColorWhite;
        } else if ((INT32)i == impl->HoverSection) {
            bg = TuiColorCyan;
            fg = TuiColorBlack;
        }

        /* Draw section background */
        for (UINT32 h = 0; h < height; h++) {
            for (UINT32 w = 0; w < section->Width; w++) {
                if (currentX + w < X + (INT32)impl->State.Bounds.Width) {
                    Screen->Vtbl->WriteChar(Screen, currentX + w, Y + h, ' ', fg, bg);
                }
            }
        }

        /* Draw section title */
        CHAR8 displayTitle[MAX_SECTION_TITLE];
        strncpy(displayTitle, section->Title, sizeof(displayTitle) - 4);
        displayTitle[sizeof(displayTitle) - 4] = '\0';

        /* Add sort indicator */
        if (section->SortIndicator == SortAscending) {
            strncat(displayTitle, " ", sizeof(displayTitle) - strlen(displayTitle) - 1);
            displayTitle[strlen(displayTitle)] = gBoxChars.ArrowUp;
            displayTitle[strlen(displayTitle) + 1] = '\0';
        } else if (section->SortIndicator == SortDescending) {
            strncat(displayTitle, " ", sizeof(displayTitle) - strlen(displayTitle) - 1);
            displayTitle[strlen(displayTitle)] = gBoxChars.ArrowDown;
            displayTitle[strlen(displayTitle) + 1] = '\0';
        }

        /* Truncate to section width */
        if (strlen(displayTitle) > section->Width - 2) {
            displayTitle[section->Width - 2] = '\0';
        }

        /* Center text */
        UINT32 textLen = strlen(displayTitle);
        UINT32 textX = (section->Width > textLen) ? ((section->Width - textLen) / 2) : 0;

        Screen->Vtbl->WriteText(Screen, currentX + textX, Y, displayTitle, fg, bg);

        /* Draw resize handle if resizable */
        if (section->Resizable && i < impl->SectionCount - 1) {
            Screen->Vtbl->WriteChar(Screen, currentX + section->Width - 1, Y,
                                   gBoxChars.SingleVertical,
                                   TuiColorBrightBlack, bg);
        }

        currentX += section->Width;
    }

    return S_OK;
}

/* Handle mouse input */
static HRESULT ANXAPI HeaderView_HandleMouse(
    ITuiHeaderView *This,
    CONST TUI_MOUSE_EVENT *Event
)
{
    TuiHeaderViewImpl *impl = (TuiHeaderViewImpl *)This;

    INT32 relativeX = Event->X - impl->State.Bounds.X;

    switch (Event->Type) {
        case TuiMouseMove:
            if (impl->DragOperation == DragResize && impl->DragSection >= 0) {
                /* Resizing column */
                INT32 delta = Event->X - impl->DragStartX;
                INT32 newWidth = (INT32)impl->DragStartWidth + delta;

                HeaderSection *section = &impl->Sections[impl->DragSection];

                if (newWidth < (INT32)section->MinWidth) {
                    newWidth = section->MinWidth;
                }
                if (section->MaxWidth > 0 && newWidth > (INT32)section->MaxWidth) {
                    newWidth = section->MaxWidth;
                }

                section->Width = newWidth;

                if (impl->OnSectionResized) {
                    impl->OnSectionResized(impl->UserData, impl->DragSection, newWidth);
                }
            } else if (impl->DragOperation == DragReorder && impl->DragSection >= 0) {
                /* Reordering column - show visual feedback */
                impl->HoverSection = GetSectionAtX(impl, Event->X);
            } else {
                /* Update hover state */
                INT32 resizeBorder = GetResizeBorderAtX(impl, Event->X);
                if (resizeBorder >= 0) {
                    impl->HoverSection = -1;  /* Show resize cursor */
                } else {
                    impl->HoverSection = GetSectionAtX(impl, Event->X);
                }
            }
            break;

        case TuiMousePress:
            if (Event->Button == TuiMouseLeft) {
                /* Check if clicking on resize border */
                INT32 resizeBorder = GetResizeBorderAtX(impl, Event->X);
                if (resizeBorder >= 0) {
                    impl->DragOperation = DragResize;
                    impl->DragSection = resizeBorder;
                    impl->DragStartX = Event->X;
                    impl->DragStartWidth = impl->Sections[resizeBorder].Width;
                } else {
                    /* Clicking on section */
                    impl->PressedSection = GetSectionAtX(impl, Event->X);
                }
            }
            break;

        case TuiMouseRelease:
            if (Event->Button == TuiMouseLeft) {
                if (impl->DragOperation == DragResize) {
                    /* End resize */
                    impl->DragOperation = DragNone;
                    impl->DragSection = -1;
                } else if (impl->DragOperation == DragReorder) {
                    /* End reorder - swap sections */
                    INT32 targetSection = GetSectionAtX(impl, Event->X);
                    if (targetSection >= 0 && targetSection != impl->DragSection) {
                        HeaderSection temp = impl->Sections[impl->DragSection];
                        impl->Sections[impl->DragSection] = impl->Sections[targetSection];
                        impl->Sections[targetSection] = temp;

                        if (impl->OnSectionMoved) {
                            impl->OnSectionMoved(impl->UserData, impl->DragSection, targetSection);
                        }
                    }
                    impl->DragOperation = DragNone;
                    impl->DragSection = -1;
                } else if (impl->PressedSection >= 0) {
                    /* Section click - toggle sort */
                    INT32 section = GetSectionAtX(impl, Event->X);
                    if (section == impl->PressedSection && section >= 0) {
                        HeaderSection *sec = &impl->Sections[section];
                        if (sec->Clickable) {
                            /* Cycle sort order */
                            if (sec->SortIndicator == SortNone) {
                                sec->SortIndicator = SortAscending;
                            } else if (sec->SortIndicator == SortAscending) {
                                sec->SortIndicator = SortDescending;
                            } else {
                                sec->SortIndicator = SortNone;
                            }

                            if (impl->OnSectionClicked) {
                                impl->OnSectionClicked(impl->UserData, section);
                            }
                        }
                    }
                }
                impl->PressedSection = -1;
            }
            break;

        case TuiMouseDrag:
            if (impl->PressedSection >= 0 && impl->DragOperation == DragNone) {
                /* Start reorder operation */
                impl->DragOperation = DragReorder;
                impl->DragSection = impl->PressedSection;
                impl->PressedSection = -1;
            }
            break;

        default:
            break;
    }

    return S_OK;
}

/* Add section */
static HRESULT ANXAPI HeaderView_AddSection(
    ITuiHeaderView *This,
    CONST CHAR8 *Title,
    UINT32 Width
)
{
    TuiHeaderViewImpl *impl = (TuiHeaderViewImpl *)This;

    if (impl->SectionCount >= MAX_HEADER_SECTIONS) {
        return E_OUTOFMEMORY;
    }

    HeaderSection *section = &impl->Sections[impl->SectionCount++];
    strncpy(section->Title, Title ? Title : "", sizeof(section->Title) - 1);
    section->Title[sizeof(section->Title) - 1] = '\0';
    section->Width = Width;
    section->MinWidth = 5;
    section->MaxWidth = 0;  /* No limit */
    section->Resizable = TRUE;
    section->Clickable = TRUE;
    section->SortIndicator = SortNone;
    section->UserData = NULL;

    return S_OK;
}

/* Set section sort indicator */
static HRESULT ANXAPI HeaderView_SetSortIndicator(
    ITuiHeaderView *This,
    UINT32 SectionIndex,
    UINT32 SortOrder
)
{
    TuiHeaderViewImpl *impl = (TuiHeaderViewImpl *)This;

    if (SectionIndex >= impl->SectionCount) {
        return E_INVALIDARG;
    }

    impl->Sections[SectionIndex].SortIndicator = (enum _SortOrder)SortOrder;
    return S_OK;
}

/* Get section width */
static HRESULT ANXAPI HeaderView_GetSectionWidth(
    ITuiHeaderView *This,
    UINT32 SectionIndex,
    UINT32 *OutWidth
)
{
    TuiHeaderViewImpl *impl = (TuiHeaderViewImpl *)This;

    if (SectionIndex >= impl->SectionCount || !OutWidth) {
        return E_INVALIDARG;
    }

    *OutWidth = impl->Sections[SectionIndex].Width;
    return S_OK;
}

/* Set callbacks */
static HRESULT ANXAPI HeaderView_SetCallbacks(
    ITuiHeaderView *This,
    HRESULT (*OnSectionClicked)(VOID*, UINT32),
    HRESULT (*OnSectionResized)(VOID*, UINT32, UINT32),
    HRESULT (*OnSectionMoved)(VOID*, UINT32, UINT32),
    VOID *UserData
)
{
    TuiHeaderViewImpl *impl = (TuiHeaderViewImpl *)This;

    impl->OnSectionClicked = OnSectionClicked;
    impl->OnSectionResized = OnSectionResized;
    impl->OnSectionMoved = OnSectionMoved;
    impl->UserData = UserData;

    return S_OK;
}

/* Widget common methods */
static HRESULT ANXAPI HeaderView_SetBounds(ITuiHeaderView *This, CONST TUI_RECT *Bounds)
{
    TuiHeaderViewImpl *impl = (TuiHeaderViewImpl *)This;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI HeaderView_GetBounds(ITuiHeaderView *This, TUI_RECT *Bounds)
{
    TuiHeaderViewImpl *impl = (TuiHeaderViewImpl *)This;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI HeaderView_SetVisible(ITuiHeaderView *This, BOOLEAN Visible)
{
    TuiHeaderViewImpl *impl = (TuiHeaderViewImpl *)This;
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI HeaderView_IsVisible(ITuiHeaderView *This)
{
    TuiHeaderViewImpl *impl = (TuiHeaderViewImpl *)This;
    return impl->State.Visible;
}

static HRESULT ANXAPI HeaderView_SetEnabled(ITuiHeaderView *This, BOOLEAN Enabled)
{
    TuiHeaderViewImpl *impl = (TuiHeaderViewImpl *)This;
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI HeaderView_IsEnabled(ITuiHeaderView *This)
{
    TuiHeaderViewImpl *impl = (TuiHeaderViewImpl *)This;
    return impl->State.Enabled;
}

/* VTable */
static ITuiHeaderViewVtbl HeaderViewVtbl = {
    HeaderView_QueryInterface,
    HeaderView_AddRef,
    HeaderView_Release,
    HeaderView_Render,
    HeaderView_HandleMouse,
    HeaderView_SetBounds,
    HeaderView_GetBounds,
    HeaderView_SetVisible,
    HeaderView_IsVisible,
    HeaderView_SetEnabled,
    HeaderView_IsEnabled,
    HeaderView_AddSection,
    HeaderView_SetSortIndicator,
    HeaderView_GetSectionWidth,
    HeaderView_SetCallbacks
};

/* Factory function */
HRESULT AnxTuiCreateHeaderView(ITuiHeaderView **OutHeaderView)
{
    TuiHeaderViewImpl *impl;

    if (!OutHeaderView) return E_INVALIDARG;

    impl = (TuiHeaderViewImpl *)malloc(sizeof(TuiHeaderViewImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiHeaderViewImpl));
    impl->Interface.Vtbl = &HeaderViewVtbl;
    InitWidgetState(&impl->State);

    impl->HoverSection = -1;
    impl->PressedSection = -1;
    impl->DragOperation = DragNone;
    impl->DragSection = -1;

    impl->State.Bounds.Width = 80;
    impl->State.Bounds.Height = 1;

    *OutHeaderView = &impl->Interface;
    return S_OK;
}
