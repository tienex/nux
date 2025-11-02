/*
** ==========================================================================
**
** VINIL - Vincent Intermediate Language Unified Library
**
** Unified execution engine for graphics and compute workloads
**
** --------------------------------------------------------------------------
**
** Vincent 3D Rendering Library, Programmable Pipeline Edition
**
** Copyright (C) 2003-2007 Hans-Martin Will.
** Copyright (C) 2025 NUX Project
**
** @CDDL_HEADER_START@
**
** The contents of this file are subject to the terms of the
** Common Development and Distribution License, Version 1.0 only
** (the "License").  You may not use this file except in compliance
** with the License.
**
** You can obtain a copy of the license at
** http://www.vincent3d.com/software/ogles2/license/license.html
** See the License for the specific language governing permissions
** and limitations under the License.
**
** When distributing Covered Code, include this CDDL_HEADER in each
** file and include the License file named LICENSE.TXT in the root folder
** of your distribution.
** If applicable, add the following below this CDDL_HEADER, with the
** fields enclosed by brackets "[]" replaced with your own identifying
** information: Portions Copyright [yyyy] [name of copyright owner]
**
** @CDDL_HEADER_END@
**
** ==========================================================================
*/

#ifndef VINIL_H
#define VINIL_H 1

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** --------------------------------------------------------------------------
** Version Information
** --------------------------------------------------------------------------
*/

#define VINIL_VERSION_MAJOR     0
#define VINIL_VERSION_MINOR     1
#define VINIL_VERSION_PATCH     0

/*
** --------------------------------------------------------------------------
** Basic Types
** --------------------------------------------------------------------------
*/

typedef int32_t         vinil_int32;
typedef uint32_t        vinil_uint32;
typedef int64_t         vinil_int64;
typedef uint64_t        vinil_uint64;
typedef float           vinil_float;
typedef double          vinil_double;
typedef size_t          vinil_size;
typedef ptrdiff_t       vinil_ssize;
typedef bool            vinil_bool;

#define VINIL_TRUE      true
#define VINIL_FALSE     false

/*
** --------------------------------------------------------------------------
** Error Codes
** --------------------------------------------------------------------------
*/

typedef enum vinil_error {
    VINIL_SUCCESS = 0,
    VINIL_ERROR_OUT_OF_MEMORY,
    VINIL_ERROR_INVALID_ARGUMENT,
    VINIL_ERROR_INVALID_PROGRAM,
    VINIL_ERROR_COMPILATION_FAILED,
    VINIL_ERROR_LINKING_FAILED,
    VINIL_ERROR_EXECUTION_FAILED,
    VINIL_ERROR_NOT_IMPLEMENTED
} vinil_error;

/*
** --------------------------------------------------------------------------
** Forward Declarations
** --------------------------------------------------------------------------
*/

typedef struct vinil_context vinil_context;
typedef struct vinil_program vinil_program;
typedef struct vinil_executable vinil_executable;
typedef struct vinil_memory_pool vinil_memory_pool;

/*
** --------------------------------------------------------------------------
** Context Management
** --------------------------------------------------------------------------
*/

/**
 * Create a new VINIL execution context
 *
 * @return New context, or NULL on failure
 */
vinil_context* vinil_context_create(void);

/**
 * Destroy a VINIL execution context
 *
 * @param ctx Context to destroy
 */
void vinil_context_destroy(vinil_context* ctx);

/*
** --------------------------------------------------------------------------
** Program Management
** --------------------------------------------------------------------------
*/

/**
 * Create a new empty program
 *
 * @param ctx Execution context
 * @return New program, or NULL on failure
 */
vinil_program* vinil_program_create(vinil_context* ctx);

/**
 * Destroy a program
 *
 * @param program Program to destroy
 */
void vinil_program_destroy(vinil_program* program);

/**
 * Compile and link a program into an executable
 *
 * @param ctx Execution context
 * @param program Program to compile
 * @param use_jit If true, use JIT compilation; otherwise use interpreter
 * @return Executable code, or NULL on failure
 */
vinil_executable* vinil_program_compile(vinil_context* ctx,
                                         vinil_program* program,
                                         vinil_bool use_jit);

/**
 * Destroy an executable
 *
 * @param executable Executable to destroy
 */
void vinil_executable_destroy(vinil_executable* executable);

/*
** --------------------------------------------------------------------------
** Execution
** --------------------------------------------------------------------------
*/

/**
 * Execute a compiled program
 *
 * @param ctx Execution context
 * @param executable Executable to run
 * @param user_data User-provided execution context data
 * @return Error code
 */
vinil_error vinil_execute(vinil_context* ctx,
                           vinil_executable* executable,
                           void* user_data);

/*
** --------------------------------------------------------------------------
** Utility Functions
** --------------------------------------------------------------------------
*/

/**
 * Get error message for an error code
 *
 * @param error Error code
 * @return Human-readable error string
 */
const char* vinil_error_string(vinil_error error);

/**
 * Get VINIL version string
 *
 * @return Version string in format "major.minor.patch"
 */
const char* vinil_version_string(void);

#ifdef __cplusplus
}
#endif

#endif /* VINIL_H */
