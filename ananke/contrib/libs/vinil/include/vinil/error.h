/** @file
  VINIL Error Handling COM Interfaces

  COM interface sinks for error notifications and diagnostics.
  Proper callback mechanism using COM instead of function pointers.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#pragma once

#include <vinil/base.h>
#include <ananke/com.h>

//
// GUIDs
//

ANX_DEFINE_GUID(IID_IVinilErrorSink, 0x90123456, 0x9012, 0x9012, 0x90, 0x12, 0x34, 0x56, 0xAB, 0xCD, 0xEF, 0x78);
ANX_DEFINE_GUID(IID_IVinilDiagnosticSink, 0xA0123456, 0xA012, 0xA012, 0xA0, 0x12, 0x34, 0x56, 0xAB, 0xCD, 0xEF, 0x89);

//
// Error Severity Levels
//

typedef enum _VINIL_ERROR_SEVERITY {
    VinilErrorSeverityInfo     = 0,
    VinilErrorSeverityWarning  = 1,
    VinilErrorSeverityError    = 2,
    VinilErrorSeverityFatal    = 3,
} VINIL_ERROR_SEVERITY;

//
// Error Categories
//

typedef enum _VINIL_ERROR_CATEGORY {
    VinilErrorCategoryMemory       = 0,
    VinilErrorCategoryCompilation  = 1,
    VinilErrorCategoryExecution    = 2,
    VinilErrorCategoryValidation   = 3,
    VinilErrorCategoryInternal     = 4,
} VINIL_ERROR_CATEGORY;

//
// Error Information Structure
//

typedef struct _VINIL_ERROR_INFO {
    VINIL_ERROR_SEVERITY  Severity;
    VINIL_ERROR_CATEGORY  Category;
    UINT32                Code;
    CONST CHAR8           *Message;
    CONST CHAR8           *File;
    UINT32                Line;
    VOID                  *Context;
} VINIL_ERROR_INFO;

//
// IVinilErrorSink Interface
//
// Callback interface for error notifications.
// Clients implement this interface to receive error reports.
//

ANX_BEGIN_INTERFACE(IVinilErrorSink, IUnknown, IID_IVinilErrorSink, "90123456-9012-9012-9012-3456ABCDEF78")
    /**
      Report an error.

      @param[in]  ErrorInfo  Detailed error information.

      @retval  S_OK       Error handled successfully.
      @retval  E_ABORT    Abort current operation.
    **/
    ANX_IFACE_METHOD(HRESULT, OnError, (CONST VINIL_ERROR_INFO *ErrorInfo))

    /**
      Query if specific error category should be reported.

      @param[in]  Category  Error category.
      @param[out] Enable    TRUE if category should be reported.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, IsEnabled, (VINIL_ERROR_CATEGORY Category, BOOLEAN *Enable))
ANX_END_INTERFACE(IVinilErrorSink, IID_IVinilErrorSink)

//
// IVinilDiagnosticSink Interface
//
// Extended diagnostic interface for performance monitoring and profiling.
//

typedef enum _VINIL_DIAGNOSTIC_EVENT {
    VinilDiagnosticEventCompilationStart   = 0,
    VinilDiagnosticEventCompilationEnd     = 1,
    VinilDiagnosticEventExecutionStart     = 2,
    VinilDiagnosticEventExecutionEnd       = 3,
    VinilDiagnosticEventJitStart           = 4,
    VinilDiagnosticEventJitEnd             = 5,
    VinilDiagnosticEventMemoryAllocation   = 6,
    VinilDiagnosticEventMemoryRelease      = 7,
} VINIL_DIAGNOSTIC_EVENT;

typedef struct _VINIL_DIAGNOSTIC_INFO {
    VINIL_DIAGNOSTIC_EVENT  Event;
    UINT64                  Timestamp;
    UINT64                  Duration;
    UINTN                   MemoryUsed;
    UINT32                  InstructionCount;
    VOID                    *Context;
} VINIL_DIAGNOSTIC_INFO;

ANX_BEGIN_INTERFACE(IVinilDiagnosticSink, IUnknown, IID_IVinilDiagnosticSink, "A0123456-A012-A012-A012-3456ABCDEF89")
    /**
      Report a diagnostic event.

      @param[in]  DiagInfo  Diagnostic information.

      @retval  S_OK  Event logged successfully.
    **/
    ANX_IFACE_METHOD(HRESULT, OnDiagnosticEvent, (CONST VINIL_DIAGNOSTIC_INFO *DiagInfo))

    /**
      Query performance statistics.

      @param[out]  TotalExecutions      Total number of executions.
      @param[out]  TotalInstructions    Total instructions executed.
      @param[out]  AverageTimeNs        Average execution time in nanoseconds.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, GetStatistics, (UINT64 *TotalExecutions, UINT64 *TotalInstructions, UINT64 *AverageTimeNs))
ANX_END_INTERFACE(IVinilDiagnosticSink, IID_IVinilDiagnosticSink)

//
// COBJMACROS
//

#ifdef COBJMACROS

/* IVinilErrorSink */
#define IVinilErrorSink_QueryInterface(This, riid, ppv) \
    (This)->lpVtbl->QueryInterface(This, riid, ppv)
#define IVinilErrorSink_AddRef(This) \
    (This)->lpVtbl->AddRef(This)
#define IVinilErrorSink_Release(This) \
    (This)->lpVtbl->Release(This)
#define IVinilErrorSink_OnError(This, ErrorInfo) \
    (This)->lpVtbl->OnError(This, ErrorInfo)
#define IVinilErrorSink_IsEnabled(This, Category, Enable) \
    (This)->lpVtbl->IsEnabled(This, Category, Enable)

/* IVinilDiagnosticSink */
#define IVinilDiagnosticSink_QueryInterface(This, riid, ppv) \
    (This)->lpVtbl->QueryInterface(This, riid, ppv)
#define IVinilDiagnosticSink_AddRef(This) \
    (This)->lpVtbl->AddRef(This)
#define IVinilDiagnosticSink_Release(This) \
    (This)->lpVtbl->Release(This)
#define IVinilDiagnosticSink_OnDiagnosticEvent(This, DiagInfo) \
    (This)->lpVtbl->OnDiagnosticEvent(This, DiagInfo)
#define IVinilDiagnosticSink_GetStatistics(This, TotalExecutions, TotalInstructions, AverageTimeNs) \
    (This)->lpVtbl->GetStatistics(This, TotalExecutions, TotalInstructions, AverageTimeNs)

#endif /* COBJMACROS */

//
// Error Reporting Helper Functions
//

/**
  Report an error through the error sink.

  @param[in]  ErrorSink  Error sink interface (can be NULL).
  @param[in]  Severity   Error severity level.
  @param[in]  Category   Error category.
  @param[in]  Code       Error code.
  @param[in]  Message    Error message.
  @param[in]  File       Source file name.
  @param[in]  Line       Source line number.

  @retval  S_OK     Error reported successfully.
  @retval  E_ABORT  Error sink requested abort.
**/
HRESULT
VinilReportError (
    IVinilErrorSink       *ErrorSink,
    VINIL_ERROR_SEVERITY  Severity,
    VINIL_ERROR_CATEGORY  Category,
    UINT32                Code,
    CONST CHAR8           *Message,
    CONST CHAR8           *File,
    UINT32                Line
    );

//
// Diagnostic Helper Macros
//

#define VINIL_ERROR(Sink, Category, Code, Message) \
    VinilReportError((Sink), VinilErrorSeverityError, (Category), (Code), (Message), __FILE__, __LINE__)

#define VINIL_WARNING(Sink, Category, Code, Message) \
    VinilReportError((Sink), VinilErrorSeverityWarning, (Category), (Code), (Message), __FILE__, __LINE__)

#define VINIL_FATAL(Sink, Category, Code, Message) \
    VinilReportError((Sink), VinilErrorSeverityFatal, (Category), (Code), (Message), __FILE__, __LINE__)
