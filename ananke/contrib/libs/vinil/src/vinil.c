/*
** ==========================================================================
**
** VINIL - Vincent Intermediate Language Unified Library
** Main API Implementation
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

#include <vinil/vinil.h>
#include <vinil/memory.h>
#include <stdlib.h>
#include <stdio.h>

/*
** --------------------------------------------------------------------------
** Internal Structures
** --------------------------------------------------------------------------
*/

struct vinil_context {
    vinil_memory_pool*  memory_pool;
    vinil_error         last_error;
};

struct vinil_program {
    vinil_context*      context;
    vinil_memory_pool*  memory_pool;
    /* IL program data will go here */
};

struct vinil_executable {
    vinil_context*      context;
    void*               code;           /* JIT code or interpreter data */
    vinil_bool          is_jit;         /* true if JIT compiled */
};

/*
** --------------------------------------------------------------------------
** Context Management
** --------------------------------------------------------------------------
*/

vinil_context* vinil_context_create(void) {
    vinil_context* ctx = (vinil_context*)malloc(sizeof(vinil_context));
    if (!ctx) {
        return NULL;
    }

    ctx->memory_pool = vinil_memory_pool_create(VINIL_DEFAULT_PAGE_SIZE, NULL);
    if (!ctx->memory_pool) {
        free(ctx);
        return NULL;
    }

    ctx->last_error = VINIL_SUCCESS;
    return ctx;
}

void vinil_context_destroy(vinil_context* ctx) {
    if (!ctx) {
        return;
    }

    if (ctx->memory_pool) {
        vinil_memory_pool_destroy(ctx->memory_pool);
    }

    free(ctx);
}

/*
** --------------------------------------------------------------------------
** Program Management
** --------------------------------------------------------------------------
*/

vinil_program* vinil_program_create(vinil_context* ctx) {
    if (!ctx) {
        return NULL;
    }

    vinil_program* program = (vinil_program*)malloc(sizeof(vinil_program));
    if (!program) {
        ctx->last_error = VINIL_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    program->context = ctx;
    program->memory_pool = vinil_memory_pool_create(VINIL_DEFAULT_PAGE_SIZE, NULL);

    if (!program->memory_pool) {
        free(program);
        ctx->last_error = VINIL_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    return program;
}

void vinil_program_destroy(vinil_program* program) {
    if (!program) {
        return;
    }

    if (program->memory_pool) {
        vinil_memory_pool_destroy(program->memory_pool);
    }

    free(program);
}

vinil_executable* vinil_program_compile(vinil_context* ctx,
                                         vinil_program* program,
                                         vinil_bool use_jit) {
    if (!ctx || !program) {
        if (ctx) {
            ctx->last_error = VINIL_ERROR_INVALID_ARGUMENT;
        }
        return NULL;
    }

    vinil_executable* executable = (vinil_executable*)malloc(sizeof(vinil_executable));
    if (!executable) {
        ctx->last_error = VINIL_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    executable->context = ctx;
    executable->code = NULL;
    executable->is_jit = use_jit;

    /* TODO: Actual compilation happens here */
    /* For now, just a stub */

    return executable;
}

void vinil_executable_destroy(vinil_executable* executable) {
    if (!executable) {
        return;
    }

    /* TODO: Free JIT code or interpreter data */

    free(executable);
}

/*
** --------------------------------------------------------------------------
** Execution
** --------------------------------------------------------------------------
*/

vinil_error vinil_execute(vinil_context* ctx,
                           vinil_executable* executable,
                           void* user_data) {
    if (!ctx || !executable) {
        return VINIL_ERROR_INVALID_ARGUMENT;
    }

    /* TODO: Actual execution happens here */
    /* Either call JIT code or run interpreter */

    (void)user_data; /* Suppress unused warning for now */

    return VINIL_ERROR_NOT_IMPLEMENTED;
}

/*
** --------------------------------------------------------------------------
** Utility Functions
** --------------------------------------------------------------------------
*/

const char* vinil_error_string(vinil_error error) {
    switch (error) {
        case VINIL_SUCCESS:
            return "Success";
        case VINIL_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case VINIL_ERROR_INVALID_ARGUMENT:
            return "Invalid argument";
        case VINIL_ERROR_INVALID_PROGRAM:
            return "Invalid program";
        case VINIL_ERROR_COMPILATION_FAILED:
            return "Compilation failed";
        case VINIL_ERROR_LINKING_FAILED:
            return "Linking failed";
        case VINIL_ERROR_EXECUTION_FAILED:
            return "Execution failed";
        case VINIL_ERROR_NOT_IMPLEMENTED:
            return "Not implemented";
        default:
            return "Unknown error";
    }
}

const char* vinil_version_string(void) {
    static char version[32];
    snprintf(version, sizeof(version), "%d.%d.%d",
             VINIL_VERSION_MAJOR,
             VINIL_VERSION_MINOR,
             VINIL_VERSION_PATCH);
    return version;
}
