/*
 * Focus Manager Implementation
 *
 * Manages keyboard focus and tab navigation between widgets.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

#define MAX_FOCUSABLE_WIDGETS 256

typedef struct {
    VOID *Widget;
    UINT32 TabOrder;
} FocusableWidget;

typedef struct {
    ITuiFocusManager Interface;
    UINTN RefCount;
    FocusableWidget Widgets[MAX_FOCUSABLE_WIDGETS];
    UINT32 WidgetCount;
    INT32 CurrentFocusIndex;
    BOOLEAN TabNavigationEnabled;
} TuiFocusManagerImpl;

/* IUnknown methods */
static HRESULT ANXAPI FocusManager_QueryInterface(
    ITuiFocusManager *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI FocusManager_AddRef(ITuiFocusManager *This)
{
    TuiFocusManagerImpl *impl = (TuiFocusManagerImpl *)This;
    return ++impl->RefCount;
}

static UINTN ANXAPI FocusManager_Release(ITuiFocusManager *This)
{
    TuiFocusManagerImpl *impl = (TuiFocusManagerImpl *)This;
    UINTN refCount = --impl->RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* Helper: Sort widgets by tab order */
static VOID SortWidgetsByTabOrder(TuiFocusManagerImpl *impl)
{
    UINT32 i, j;
    FocusableWidget temp;

    /* Simple bubble sort - good enough for small lists */
    for (i = 0; i < impl->WidgetCount - 1; i++) {
        for (j = 0; j < impl->WidgetCount - i - 1; j++) {
            if (impl->Widgets[j].TabOrder > impl->Widgets[j + 1].TabOrder) {
                temp = impl->Widgets[j];
                impl->Widgets[j] = impl->Widgets[j + 1];
                impl->Widgets[j + 1] = temp;
            }
        }
    }
}

/* ITuiFocusManager methods */
static HRESULT ANXAPI FocusManager_RegisterWidget(
    ITuiFocusManager *This,
    VOID *Widget,
    UINT32 TabOrder
)
{
    TuiFocusManagerImpl *impl = (TuiFocusManagerImpl *)This;

    if (Widget == NULL) return E_POINTER;
    if (impl->WidgetCount >= MAX_FOCUSABLE_WIDGETS) return E_OUTOFMEMORY;

    /* Check if widget already registered */
    UINT32 i;
    for (i = 0; i < impl->WidgetCount; i++) {
        if (impl->Widgets[i].Widget == Widget) {
            /* Update tab order */
            impl->Widgets[i].TabOrder = TabOrder;
            SortWidgetsByTabOrder(impl);
            return S_OK;
        }
    }

    /* Add new widget */
    impl->Widgets[impl->WidgetCount].Widget = Widget;
    impl->Widgets[impl->WidgetCount].TabOrder = TabOrder;
    impl->WidgetCount++;

    /* Sort by tab order */
    SortWidgetsByTabOrder(impl);

    return S_OK;
}

static HRESULT ANXAPI FocusManager_UnregisterWidget(
    ITuiFocusManager *This,
    VOID *Widget
)
{
    TuiFocusManagerImpl *impl = (TuiFocusManagerImpl *)This;
    UINT32 i, j;

    if (Widget == NULL) return E_POINTER;

    /* Find and remove widget */
    for (i = 0; i < impl->WidgetCount; i++) {
        if (impl->Widgets[i].Widget == Widget) {
            /* Shift remaining widgets */
            for (j = i; j < impl->WidgetCount - 1; j++) {
                impl->Widgets[j] = impl->Widgets[j + 1];
            }
            impl->WidgetCount--;

            /* Adjust focus index */
            if ((INT32)i == impl->CurrentFocusIndex) {
                impl->CurrentFocusIndex = -1;
            } else if ((INT32)i < impl->CurrentFocusIndex) {
                impl->CurrentFocusIndex--;
            }

            return S_OK;
        }
    }

    return E_INVALIDARG;
}

static HRESULT ANXAPI FocusManager_SetFocus(
    ITuiFocusManager *This,
    VOID *Widget
)
{
    TuiFocusManagerImpl *impl = (TuiFocusManagerImpl *)This;
    UINT32 i;

    if (Widget == NULL) {
        /* Clear focus */
        impl->CurrentFocusIndex = -1;
        return S_OK;
    }

    /* Find widget */
    for (i = 0; i < impl->WidgetCount; i++) {
        if (impl->Widgets[i].Widget == Widget) {
            impl->CurrentFocusIndex = i;
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

static HRESULT ANXAPI FocusManager_GetFocus(
    ITuiFocusManager *This,
    VOID **Widget
)
{
    TuiFocusManagerImpl *impl = (TuiFocusManagerImpl *)This;

    if (Widget == NULL) return E_POINTER;

    if (impl->CurrentFocusIndex >= 0 &&
        (UINT32)impl->CurrentFocusIndex < impl->WidgetCount) {
        *Widget = impl->Widgets[impl->CurrentFocusIndex].Widget;
    } else {
        *Widget = NULL;
    }

    return S_OK;
}

static HRESULT ANXAPI FocusManager_FocusNext(ITuiFocusManager *This)
{
    TuiFocusManagerImpl *impl = (TuiFocusManagerImpl *)This;

    if (!impl->TabNavigationEnabled) return S_OK;
    if (impl->WidgetCount == 0) return S_OK;

    if (impl->CurrentFocusIndex < 0) {
        /* No focus, set to first widget */
        impl->CurrentFocusIndex = 0;
    } else {
        /* Move to next widget, wrap around */
        impl->CurrentFocusIndex++;
        if ((UINT32)impl->CurrentFocusIndex >= impl->WidgetCount) {
            impl->CurrentFocusIndex = 0;
        }
    }

    return S_OK;
}

static HRESULT ANXAPI FocusManager_FocusPrevious(ITuiFocusManager *This)
{
    TuiFocusManagerImpl *impl = (TuiFocusManagerImpl *)This;

    if (!impl->TabNavigationEnabled) return S_OK;
    if (impl->WidgetCount == 0) return S_OK;

    if (impl->CurrentFocusIndex < 0) {
        /* No focus, set to last widget */
        impl->CurrentFocusIndex = impl->WidgetCount - 1;
    } else {
        /* Move to previous widget, wrap around */
        impl->CurrentFocusIndex--;
        if (impl->CurrentFocusIndex < 0) {
            impl->CurrentFocusIndex = impl->WidgetCount - 1;
        }
    }

    return S_OK;
}

static HRESULT ANXAPI FocusManager_SetTabOrder(
    ITuiFocusManager *This,
    VOID *Widget,
    UINT32 TabOrder
)
{
    TuiFocusManagerImpl *impl = (TuiFocusManagerImpl *)This;
    UINT32 i;

    if (Widget == NULL) return E_POINTER;

    /* Find widget and update tab order */
    for (i = 0; i < impl->WidgetCount; i++) {
        if (impl->Widgets[i].Widget == Widget) {
            impl->Widgets[i].TabOrder = TabOrder;
            SortWidgetsByTabOrder(impl);
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

static HRESULT ANXAPI FocusManager_GetTabOrder(
    ITuiFocusManager *This,
    VOID *Widget,
    UINT32 *TabOrder
)
{
    TuiFocusManagerImpl *impl = (TuiFocusManagerImpl *)This;
    UINT32 i;

    if (Widget == NULL || TabOrder == NULL) return E_POINTER;

    /* Find widget */
    for (i = 0; i < impl->WidgetCount; i++) {
        if (impl->Widgets[i].Widget == Widget) {
            *TabOrder = impl->Widgets[i].TabOrder;
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

static HRESULT ANXAPI FocusManager_SetTabNavigationEnabled(
    ITuiFocusManager *This,
    BOOLEAN Enabled
)
{
    TuiFocusManagerImpl *impl = (TuiFocusManagerImpl *)This;
    impl->TabNavigationEnabled = Enabled;
    return S_OK;
}

static HRESULT ANXAPI FocusManager_HandleKey(
    ITuiFocusManager *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiFocusManagerImpl *impl = (TuiFocusManagerImpl *)This;

    if (!impl->TabNavigationEnabled) {
        *Handled = FALSE;
        return S_OK;
    }

    if (Key == TuiKeyTab) {
        /* Tab key - focus next */
        FocusManager_FocusNext(This);
        *Handled = TRUE;
        return S_OK;
    }

    /* Note: Shift+Tab would need modifier key support in TUI_KEY_EVENT */
    /* For now, assume Shift+Tab is handled elsewhere or mapped to different key */

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiFocusManager_Vtbl FocusManagerVtbl = {
    FocusManager_QueryInterface,
    FocusManager_AddRef,
    FocusManager_Release,
    FocusManager_RegisterWidget,
    FocusManager_UnregisterWidget,
    FocusManager_SetFocus,
    FocusManager_GetFocus,
    FocusManager_FocusNext,
    FocusManager_FocusPrevious,
    FocusManager_SetTabOrder,
    FocusManager_GetTabOrder,
    FocusManager_SetTabNavigationEnabled,
    FocusManager_HandleKey
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateFocusManager(OUT ITuiFocusManager **FocusManager)
{
    TuiFocusManagerImpl *impl;

    if (FocusManager == NULL) return E_POINTER;

    impl = (TuiFocusManagerImpl *)calloc(1, sizeof(TuiFocusManagerImpl));
    if (impl == NULL) {
        *FocusManager = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &FocusManagerVtbl;
    impl->RefCount = 1;

    impl->WidgetCount = 0;
    impl->CurrentFocusIndex = -1;
    impl->TabNavigationEnabled = TRUE;

    *FocusManager = &impl->Interface;
    return S_OK;
}
