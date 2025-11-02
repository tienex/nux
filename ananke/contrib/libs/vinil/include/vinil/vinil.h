/** @file
  VINIL - Vincent Intermediate Language Unified Library

  Public API for unified execution engine supporting graphics and compute
  workloads across OpenGL ES, OpenCL, CUDA, HIP, and SYCL.

  Copyright (C) 2003-2007 Hans-Martin Will
  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#ifndef __vinil_vinil_h__
#define __vinil_vinil_h__ 1

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// Version Information
//

#define VINIL_VERSION_MAJOR     0
#define VINIL_VERSION_MINOR     2
#define VINIL_VERSION_PATCH     0

//
// Basic Types
//

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

//
// Error Codes
//

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

//
// Opaque Handle Types
//

typedef struct vinil_context vinil_context;
typedef struct vinil_program vinil_program;
typedef struct vinil_executable vinil_executable;
typedef struct vinil_memory_pool vinil_memory_pool;

//
// Context Management
//

/**
  Create a new VINIL execution context.

  The context manages all resources for IL compilation and execution.

  @return  Pointer to new context, or NULL on failure.
**/
vinil_context *
vinil_context_create (
  void
  );

/**
  Destroy a VINIL execution context.

  Frees all resources associated with the context. Any executables or
  programs created from this context become invalid.

  @param[in]  ctx  Context to destroy.
**/
void
vinil_context_destroy (
  vinil_context  *ctx
  );

//
// Program Management
//

/**
  Create a new IL program.

  The program is initially empty. Use IL construction APIs to build
  the intermediate representation.

  @param[in]  ctx  Execution context.

  @return  Pointer to new program, or NULL on failure.
**/
vinil_program *
vinil_program_create (
  vinil_context  *ctx
  );

/**
  Destroy an IL program.

  Frees all IL structures. Any executables compiled from this program
  remain valid.

  @param[in]  program  Program to destroy.
**/
void
vinil_program_destroy (
  vinil_program  *program
  );

/**
  Compile an IL program to executable code.

  @param[in]  ctx      Execution context.
  @param[in]  program  IL program to compile.
  @param[in]  use_jit  TRUE to use JIT compiler, FALSE for interpreter.

  @return  Executable code, or NULL on compilation failure.
**/
vinil_executable *
vinil_program_compile (
  vinil_context  *ctx,
  vinil_program  *program,
  vinil_bool     use_jit
  );

/**
  Destroy an executable.

  @param[in]  executable  Executable to destroy.
**/
void
vinil_executable_destroy (
  vinil_executable  *executable
  );

//
// Execution
//

/**
  Execute compiled code.

  For graphics shaders, this executes the shader pipeline. For compute
  kernels, use vinil_launch_kernel() instead.

  @param[in]  ctx         Execution context.
  @param[in]  executable  Compiled executable.
  @param[in]  user_data   User data pointer passed to kernel.

  @return  VINIL_SUCCESS or error code.
**/
vinil_error
vinil_execute (
  vinil_context      *ctx,
  vinil_executable   *executable,
  void               *user_data
  );

//
// Utility Functions
//

/**
  Get error message for an error code.

  @param[in]  error  Error code.

  @return  Human-readable error string.
**/
const char *
vinil_error_string (
  vinil_error  error
  );

/**
  Get VINIL version string.

  @return  Version string in format "major.minor.patch".
**/
const char *
vinil_version_string (
  void
  );

#ifdef __cplusplus
}
#endif

#endif // __vinil_vinil_h__
