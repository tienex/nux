/**
 * @file ioworkloop.h
 * @brief IOWorkLoop Interface - Event processing and synchronization
 *
 * IIOWorkLoop provides a single-threaded event processing mechanism for drivers.
 * It manages event sources (timers, interrupts, command gates) and ensures
 * serialized access to driver data structures.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOWORKLOOP_H
#define IOWORKLOOP_H

#include <iokit/iokit.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOWorkLoop interface GUID
 * {6D8E9F4C-7A5B-4E8D-9C6F-5E7D8A9B6C4E}
 */
DEFINE_GUID(IID_IIOWorkLoop,
    0x6D8E9F4C, 0x7A5B, 0x4E8D, 0x9C, 0x6F, 0x5E, 0x7D, 0x8A, 0x9B, 0x6C, 0x4E);

/**
 * @brief IIOEventSource interface GUID
 * {5C7D8E9F-6A4B-4D7E-8B5C-6D8E9A7B5C4D}
 */
DEFINE_GUID(IID_IIOEventSource,
    0x5C7D8E9F, 0x6A4B, 0x4D7E, 0x8B, 0x5C, 0x6D, 0x8E, 0x9A, 0x7B, 0x5C, 0x4D);

/**
 * @brief Action function type for work loop operations
 */
typedef IO_RETURN (*IO_ACTION_FUNCTION)(
    VOID *pOwner,
    VOID *pArg0,
    VOID *pArg1,
    VOID *pArg2,
    VOID *pArg3
);

/**
 * @brief Event source types
 */
typedef enum _IO_EVENT_SOURCE_TYPE {
    IO_EVENT_SOURCE_INTERRUPT       = 0x00000001,   /**< Interrupt event source */
    IO_EVENT_SOURCE_TIMER           = 0x00000002,   /**< Timer event source */
    IO_EVENT_SOURCE_COMMAND_GATE    = 0x00000003,   /**< Command gate (synchronization) */
    IO_EVENT_SOURCE_INTERRUPT_GATE  = 0x00000004,   /**< Interrupt event gate */
} IO_EVENT_SOURCE_TYPE;

/**
 * @brief Work loop options
 */
typedef enum _IO_WORKLOOP_OPTIONS {
    IO_WORKLOOP_OPTION_NONE         = 0x00000000,
    IO_WORKLOOP_OPTION_DISABLE_IRQ  = 0x00000001,   /**< Disable interrupts during execution */
} IO_WORKLOOP_OPTIONS;

/**
 * @brief Forward declaration for event source
 */
typedef struct IIOEventSource IIOEventSource;

/**
 * @brief IIOEventSource - Base interface for work loop event sources
 *
 * This interface represents an event source that can be attached to a work loop.
 * Event sources include interrupts, timers, and command gates.
 */
#undef INTERFACE
#define INTERFACE IIOEventSource

DECLARE_INTERFACE_(IIOEventSource, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)(THIS_
        REFIID riid,
        void **ppvObject
        ) PURE;

    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IIOEventSource methods

    /**
     * @brief Enable the event source
     *
     * Enables event delivery from this source to the work loop.
     *
     * @retval IO_SUCCESS           Event source enabled successfully
     * @retval IO_ERROR             Failed to enable event source
     */
    STDMETHOD_(IO_RETURN, Enable)(THIS) PURE;

    /**
     * @brief Disable the event source
     *
     * Disables event delivery from this source to the work loop.
     *
     * @retval IO_SUCCESS           Event source disabled successfully
     */
    STDMETHOD_(IO_RETURN, Disable)(THIS) PURE;

    /**
     * @brief Check if event source is enabled
     *
     * Returns the enabled state of the event source.
     *
     * @param pbEnabled     Receives enabled state (TRUE/FALSE)
     *
     * @retval IO_SUCCESS           State retrieved successfully
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, IsEnabled)(THIS_
        BOOLEAN *pbEnabled
        ) PURE;

    /**
     * @brief Get event source type
     *
     * Returns the type of this event source.
     *
     * @param puType        Receives event source type
     *
     * @retval IO_SUCCESS           Type retrieved successfully
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetType)(THIS_
        UINT32 *puType
        ) PURE;

    /**
     * @brief Get owning work loop
     *
     * Returns the work loop this event source is attached to.
     *
     * @param ppWorkLoop    Receives work loop interface
     *
     * @retval IO_SUCCESS           Work loop retrieved successfully
     * @retval IO_NOT_ATTACHED      Not attached to work loop
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetWorkLoop)(THIS_
        IIOWorkLoop **ppWorkLoop
        ) PURE;

    /**
     * @brief Set action function
     *
     * Sets the action function to be called when this event source triggers.
     *
     * @param pfnAction     Action function pointer
     * @param pOwner        Owner object passed to action function
     *
     * @retval IO_SUCCESS           Action set successfully
     * @retval IO_BAD_ARGUMENT      Invalid function pointer
     */
    STDMETHOD_(IO_RETURN, SetAction)(THIS_
        IO_ACTION_FUNCTION pfnAction,
        VOID *pOwner
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOWorkLoop - Work loop interface
 *
 * This interface provides a single-threaded event processing mechanism for drivers.
 * Drivers can add event sources (interrupts, timers, command gates) to the work loop
 * for serialized event handling.
 */
#undef INTERFACE
#define INTERFACE IIOWorkLoop

DECLARE_INTERFACE_(IIOWorkLoop, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)(THIS_
        REFIID riid,
        void **ppvObject
        ) PURE;

    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IIOWorkLoop methods

    /**
     * @brief Add an event source
     *
     * Attaches an event source to this work loop for event processing.
     *
     * @param pEventSource  Event source to add
     *
     * @retval IO_SUCCESS           Event source added successfully
     * @retval IO_BAD_ARGUMENT      Invalid event source
     * @retval IO_NO_RESOURCES      Too many event sources
     */
    STDMETHOD_(IO_RETURN, AddEventSource)(THIS_
        IIOEventSource *pEventSource
        ) PURE;

    /**
     * @brief Remove an event source
     *
     * Detaches an event source from this work loop.
     *
     * @param pEventSource  Event source to remove
     *
     * @retval IO_SUCCESS           Event source removed successfully
     * @retval IO_BAD_ARGUMENT      Invalid event source
     * @retval IO_NOT_ATTACHED      Event source not attached
     */
    STDMETHOD_(IO_RETURN, RemoveEventSource)(THIS_
        IIOEventSource *pEventSource
        ) PURE;

    /**
     * @brief Run action on work loop thread
     *
     * Executes an action function on the work loop's thread, ensuring serialized
     * access to driver data structures.
     *
     * @param pfnAction     Action function to execute
     * @param pOwner        Owner object passed to action function
     * @param pArg0         First argument
     * @param pArg1         Second argument
     * @param pArg2         Third argument
     * @param pArg3         Fourth argument
     *
     * @retval IO_SUCCESS           Action executed successfully
     * @retval IO_ERROR             Action execution failed
     */
    STDMETHOD_(IO_RETURN, RunAction)(THIS_
        IO_ACTION_FUNCTION pfnAction,
        VOID *pOwner,
        VOID *pArg0,
        VOID *pArg1,
        VOID *pArg2,
        VOID *pArg3
        ) PURE;

    /**
     * @brief Enable all event sources
     *
     * Enables all event sources attached to this work loop.
     *
     * @retval IO_SUCCESS           Event sources enabled successfully
     */
    STDMETHOD_(IO_RETURN, EnableAllEventSources)(THIS) PURE;

    /**
     * @brief Disable all event sources
     *
     * Disables all event sources attached to this work loop.
     *
     * @retval IO_SUCCESS           Event sources disabled successfully
     */
    STDMETHOD_(IO_RETURN, DisableAllEventSources)(THIS) PURE;

    /**
     * @brief Check if running on work loop thread
     *
     * Determines if the current execution context is the work loop's thread.
     *
     * @param pbOnThread    Receives TRUE if on work loop thread, FALSE otherwise
     *
     * @retval IO_SUCCESS           Status retrieved successfully
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, InGate)(THIS_
        BOOLEAN *pbOnThread
        ) PURE;

    /**
     * @brief Get work loop thread
     *
     * Returns the thread handle for this work loop.
     *
     * @param ppThread      Receives thread handle
     *
     * @retval IO_SUCCESS           Thread retrieved successfully
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetThread)(THIS_
        VOID **ppThread
        ) PURE;

    /**
     * @brief Get event source count
     *
     * Returns the number of event sources attached to this work loop.
     *
     * @param puCount       Receives event source count
     *
     * @retval IO_SUCCESS           Count retrieved successfully
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetEventSourceCount)(THIS_
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Get event source by index
     *
     * Retrieves an event source from the work loop by index.
     *
     * @param uIndex        Index of event source (zero-based)
     * @param ppEventSource Receives event source interface
     *
     * @retval IO_SUCCESS           Event source retrieved successfully
     * @retval IO_BAD_ARGUMENT      Invalid index
     */
    STDMETHOD_(IO_RETURN, GetEventSourceByIndex)(THIS_
        UINT32 uIndex,
        IIOEventSource **ppEventSource
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOEventSource methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOEventSource_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IIOEventSource_AddRef(p)                     (p)->lpVtbl->AddRef(p)
#define IIOEventSource_Release(p)                    (p)->lpVtbl->Release(p)
#define IIOEventSource_Enable(p)                     (p)->lpVtbl->Enable(p)
#define IIOEventSource_Disable(p)                    (p)->lpVtbl->Disable(p)
#define IIOEventSource_IsEnabled(p,a)                (p)->lpVtbl->IsEnabled(p,a)
#define IIOEventSource_GetType(p,a)                  (p)->lpVtbl->GetType(p,a)
#define IIOEventSource_GetWorkLoop(p,a)              (p)->lpVtbl->GetWorkLoop(p,a)
#define IIOEventSource_SetAction(p,a,b)              (p)->lpVtbl->SetAction(p,a,b)

#endif

/**
 * @brief Convenience macros for calling IIOWorkLoop methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOWorkLoop_QueryInterface(p,a,b)               (p)->lpVtbl->QueryInterface(p,a,b)
#define IIOWorkLoop_AddRef(p)                            (p)->lpVtbl->AddRef(p)
#define IIOWorkLoop_Release(p)                           (p)->lpVtbl->Release(p)
#define IIOWorkLoop_AddEventSource(p,a)                  (p)->lpVtbl->AddEventSource(p,a)
#define IIOWorkLoop_RemoveEventSource(p,a)               (p)->lpVtbl->RemoveEventSource(p,a)
#define IIOWorkLoop_RunAction(p,a,b,c,d,e,f)             (p)->lpVtbl->RunAction(p,a,b,c,d,e,f)
#define IIOWorkLoop_EnableAllEventSources(p)             (p)->lpVtbl->EnableAllEventSources(p)
#define IIOWorkLoop_DisableAllEventSources(p)            (p)->lpVtbl->DisableAllEventSources(p)
#define IIOWorkLoop_InGate(p,a)                          (p)->lpVtbl->InGate(p,a)
#define IIOWorkLoop_GetThread(p,a)                       (p)->lpVtbl->GetThread(p,a)
#define IIOWorkLoop_GetEventSourceCount(p,a)             (p)->lpVtbl->GetEventSourceCount(p,a)
#define IIOWorkLoop_GetEventSourceByIndex(p,a,b)         (p)->lpVtbl->GetEventSourceByIndex(p,a,b)

#endif

#ifdef __cplusplus
}
#endif

#endif /* IOWORKLOOP_H */
