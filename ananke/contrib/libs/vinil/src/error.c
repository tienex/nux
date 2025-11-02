/** @file
  VINIL Error Handling Implementation

  Error reporting and diagnostic helpers.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#define COBJMACROS
#include <vinil/error.h>
#include <string.h>

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
    )
{
    VINIL_ERROR_INFO  ErrorInfo;
    HRESULT           Result;

    if (ErrorSink == NULL) {
        /* No error sink - silently continue */
        return S_OK;
    }

    /* Fill error info structure */
    memset(&ErrorInfo, 0, sizeof(ErrorInfo));
    ErrorInfo.Severity = Severity;
    ErrorInfo.Category = Category;
    ErrorInfo.Code = Code;
    ErrorInfo.Message = Message;
    ErrorInfo.File = File;
    ErrorInfo.Line = Line;
    ErrorInfo.Context = NULL;

    /* Call error sink */
    Result = ErrorSink->lpVtbl->OnError(ErrorSink, &ErrorInfo);

    return Result;
}
