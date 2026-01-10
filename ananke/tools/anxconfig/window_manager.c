/*
 * window_manager.c - Window Manager
 *
 * Manages windows and routes events using QueryInterface to determine
 * which listener interfaces widgets support (Cocoa-style).
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_WINDOWS 32

typedef struct {
    ITuiWindow *Window;
    ITuiWidget *RootWidget;
    BOOLEAN Active;
} ManagedWindow;

typedef struct {
    ITuiWindowManager Interface;
    UINTN RefCount;

    /* Window management */
    ManagedWindow Windows[MAX_WINDOWS];
    UINT32 WindowCount;
    ITuiWindow *FocusedWindow;

    /* Responder chain */
    ITuiResponder *FirstResponder;

    /* Compositor */
    ITuiComposer *Composer;

} TuiWindowManagerImpl;

/* IUnknown methods */
static HRESULT ANXAPI WindowManager_QueryInterface(
    ITuiWindowManager *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiWindowManager)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI WindowManager_AddRef(ITuiWindowManager *This)
{
    TuiWindowManagerImpl *impl = (TuiWindowManagerImpl *)This;
    return ++impl->RefCount;
}

static UINTN ANXAPI WindowManager_Release(ITuiWindowManager *This)
{
    TuiWindowManagerImpl *impl = (TuiWindowManagerImpl *)This;
    UINTN count = --impl->RefCount;

    if (count == 0) {
        /* Release compositor if present */
        if (impl->Composer) {
            impl->Composer->Vtbl->Release(impl->Composer);
        }

        /* Release all windows */
        for (UINT32 i = 0; i < impl->WindowCount; i++) {
            if (impl->Windows[i].Window) {
                impl->Windows[i].Window->Vtbl->Release(impl->Windows[i].Window);
            }
            if (impl->Windows[i].RootWidget) {
                ITuiResponder *responder = (ITuiResponder *)impl->Windows[i].RootWidget;
                responder->Vtbl->Release(responder);
            }
        }

        free(impl);
    }

    return count;
}

/* Register a window */
static HRESULT ANXAPI WindowManager_RegisterWindow(
    ITuiWindowManager *This,
    ITuiWindow *Window,
    ITuiWidget *RootWidget
)
{
    TuiWindowManagerImpl *impl = (TuiWindowManagerImpl *)This;

    if (impl->WindowCount >= MAX_WINDOWS) {
        return E_OUTOFMEMORY;
    }

    ManagedWindow *mw = &impl->Windows[impl->WindowCount++];
    mw->Window = Window;
    mw->RootWidget = RootWidget;
    mw->Active = TRUE;

    /* Add references */
    Window->Vtbl->AddRef(Window);
    if (RootWidget) {
        ITuiResponder *responder = (ITuiResponder *)RootWidget;
        responder->Vtbl->AddRef(responder);
    }

    /* Set as focused if it's the first window */
    if (impl->WindowCount == 1) {
        impl->FocusedWindow = Window;
    }

    return S_OK;
}

/* Unregister a window */
static HRESULT ANXAPI WindowManager_UnregisterWindow(
    ITuiWindowManager *This,
    ITuiWindow *Window
)
{
    TuiWindowManagerImpl *impl = (TuiWindowManagerImpl *)This;

    for (UINT32 i = 0; i < impl->WindowCount; i++) {
        if (impl->Windows[i].Window == Window) {
            /* Release references */
            Window->Vtbl->Release(Window);
            if (impl->Windows[i].RootWidget) {
                ITuiResponder *responder = (ITuiResponder *)impl->Windows[i].RootWidget;
                responder->Vtbl->Release(responder);
            }

            /* Shift remaining windows */
            for (UINT32 j = i; j < impl->WindowCount - 1; j++) {
                impl->Windows[j] = impl->Windows[j + 1];
            }
            impl->WindowCount--;

            /* Update focused window if necessary */
            if (impl->FocusedWindow == Window) {
                impl->FocusedWindow = (impl->WindowCount > 0) ? impl->Windows[0].Window : NULL;
            }

            return S_OK;
        }
    }

    return E_INVALIDARG;
}

/* Get focused window */
static HRESULT ANXAPI WindowManager_GetFocusedWindow(
    ITuiWindowManager *This,
    ITuiWindow **Window
)
{
    TuiWindowManagerImpl *impl = (TuiWindowManagerImpl *)This;

    if (!Window) return E_INVALIDARG;

    *Window = impl->FocusedWindow;
    return S_OK;
}

/* Set focused window */
static HRESULT ANXAPI WindowManager_SetFocusedWindow(
    ITuiWindowManager *This,
    ITuiWindow *Window
)
{
    TuiWindowManagerImpl *impl = (TuiWindowManagerImpl *)This;

    /* Verify window is registered */
    for (UINT32 i = 0; i < impl->WindowCount; i++) {
        if (impl->Windows[i].Window == Window) {
            impl->FocusedWindow = Window;
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

/* Helper: Dispatch event to widget using responder chain */
static HRESULT DispatchEventToWidget(
    ITuiWidget *Widget,
    CONST VOID *Event,
    REFIID EventType
)
{
    HRESULT hr;
    VOID *listener = NULL;

    /* Try to get the appropriate listener interface using QueryInterface */
    ITuiResponder *responder = (ITuiResponder *)Widget;
    hr = responder->Vtbl->QueryInterface(responder, EventType, &listener);

    if (SUCCEEDED(hr) && listener) {
        /* Widget supports this event type */
        if (IsEqualGUID(EventType, &IID_ITuiKeyListener)) {
            /* This is a simplified example - in reality you'd need to parse the event */
            ITuiKeyListener *keyListener = (ITuiKeyListener *)listener;
            /* Would call keyListener->Vtbl->OnKeyDown, etc. */
            keyListener->Vtbl->Release(keyListener);
            return S_OK;
        } else if (IsEqualGUID(EventType, &IID_ITuiMouseListener)) {
            ITuiMouseListener *mouseListener = (ITuiMouseListener *)listener;
            /* Would call mouseListener->Vtbl->OnMouseDown, etc. */
            mouseListener->Vtbl->Release(mouseListener);
            return S_OK;
        } else if (IsEqualGUID(EventType, &IID_ITuiDrawListener)) {
            ITuiDrawListener *drawListener = (ITuiDrawListener *)listener;
            /* Would call drawListener->Vtbl->OnDraw */
            drawListener->Vtbl->Release(drawListener);
            return S_OK;
        }

        /* Release listener */
        ITuiResponder *listenerResp = (ITuiResponder *)listener;
        listenerResp->Vtbl->Release(listenerResp);
    }

    /* Try next responder in chain */
    ITuiResponder *nextResponder = NULL;
    hr = responder->Vtbl->GetNextResponder(responder, &nextResponder);
    if (SUCCEEDED(hr) && nextResponder) {
        /* Check if next responder is a widget */
        ITuiWidget *nextWidget = NULL;
        hr = nextResponder->Vtbl->QueryInterface(nextResponder, &IID_ITuiWidget, (VOID **)&nextWidget);
        if (SUCCEEDED(hr) && nextWidget) {
            hr = DispatchEventToWidget(nextWidget, Event, EventType);
            nextWidget->Vtbl->Release((ITuiWidget *)nextWidget);
            nextResponder->Vtbl->Release(nextResponder);
            return hr;
        }
        nextResponder->Vtbl->Release(nextResponder);
    }

    return S_FALSE; /* Not handled */
}

/* Dispatch event */
static HRESULT ANXAPI WindowManager_DispatchEvent(
    ITuiWindowManager *This,
    CONST VOID *Event,
    REFIID EventType
)
{
    TuiWindowManagerImpl *impl = (TuiWindowManagerImpl *)This;

    /* Start with first responder if set */
    if (impl->FirstResponder) {
        ITuiWidget *widget = NULL;
        HRESULT hr = impl->FirstResponder->Vtbl->QueryInterface(
            impl->FirstResponder,
            &IID_ITuiWidget,
            (VOID **)&widget
        );

        if (SUCCEEDED(hr) && widget) {
            hr = DispatchEventToWidget(widget, Event, EventType);
            widget->Vtbl->Release((ITuiWidget *)widget);
            if (SUCCEEDED(hr)) {
                return S_OK;
            }
        }
    }

    /* Otherwise, dispatch to focused window's root widget */
    if (impl->FocusedWindow) {
        for (UINT32 i = 0; i < impl->WindowCount; i++) {
            if (impl->Windows[i].Window == impl->FocusedWindow) {
                if (impl->Windows[i].RootWidget) {
                    return DispatchEventToWidget(impl->Windows[i].RootWidget, Event, EventType);
                }
                break;
            }
        }
    }

    return S_FALSE; /* Not handled */
}

/* Process events */
static HRESULT ANXAPI WindowManager_ProcessEvents(
    ITuiWindowManager *This
)
{
    TuiWindowManagerImpl *impl = (TuiWindowManagerImpl *)This;

    /* This would integrate with the event loop */
    /* For now, just a placeholder */

    return S_OK;
}

/* Get first responder */
static HRESULT ANXAPI WindowManager_GetFirstResponder(
    ITuiWindowManager *This,
    ITuiResponder **Responder
)
{
    TuiWindowManagerImpl *impl = (TuiWindowManagerImpl *)This;

    if (!Responder) return E_INVALIDARG;

    *Responder = impl->FirstResponder;
    if (impl->FirstResponder) {
        impl->FirstResponder->Vtbl->AddRef(impl->FirstResponder);
    }

    return S_OK;
}

/* Set first responder */
static HRESULT ANXAPI WindowManager_SetFirstResponder(
    ITuiWindowManager *This,
    ITuiResponder *Responder
)
{
    TuiWindowManagerImpl *impl = (TuiWindowManagerImpl *)This;

    /* Release old first responder */
    if (impl->FirstResponder) {
        impl->FirstResponder->Vtbl->ResignFirstResponder(impl->FirstResponder);
        impl->FirstResponder->Vtbl->Release(impl->FirstResponder);
    }

    /* Set new first responder */
    impl->FirstResponder = Responder;
    if (Responder) {
        Responder->Vtbl->AddRef(Responder);
        Responder->Vtbl->BecomeFirstResponder(Responder);
    }

    return S_OK;
}

/* VTable */
static ITuiWindowManager_Vtbl WindowManagerVtbl = {
    WindowManager_QueryInterface,
    WindowManager_AddRef,
    WindowManager_Release,
    WindowManager_RegisterWindow,
    WindowManager_UnregisterWindow,
    WindowManager_GetFocusedWindow,
    WindowManager_SetFocusedWindow,
    WindowManager_DispatchEvent,
    WindowManager_ProcessEvents,
    WindowManager_GetFirstResponder,
    WindowManager_SetFirstResponder
};

/* Factory function */
HRESULT AnxTuiCreateWindowManager(ITuiWindowManager **OutWindowManager)
{
    TuiWindowManagerImpl *impl;

    if (!OutWindowManager) return E_INVALIDARG;

    impl = (TuiWindowManagerImpl *)malloc(sizeof(TuiWindowManagerImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiWindowManagerImpl));
    impl->Interface.Vtbl = &WindowManagerVtbl;
    impl->RefCount = 1;

    *OutWindowManager = &impl->Interface;
    return S_OK;
}
