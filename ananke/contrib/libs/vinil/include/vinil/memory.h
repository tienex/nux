/*
** ==========================================================================
**
** VINIL Memory Management
**
** Pool-based memory allocator for IL programs
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

#ifndef VINIL_MEMORY_H
#define VINIL_MEMORY_H 1

#include <vinil/vinil.h>
#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** --------------------------------------------------------------------------
** Constants
** --------------------------------------------------------------------------
*/

#define VINIL_DEFAULT_PAGE_SIZE     8192

/*
** --------------------------------------------------------------------------
** Types
** --------------------------------------------------------------------------
*/

typedef jmp_buf vinil_jump_buffer;

/*
** --------------------------------------------------------------------------
** Functions
** --------------------------------------------------------------------------
*/

/**
 * Create a new memory pool with the specified default page size
 *
 * @param default_page_size Default size for memory pages
 * @param handler Optional jump buffer for allocation failures (can be NULL)
 * @return New memory pool, or NULL on failure
 */
vinil_memory_pool* vinil_memory_pool_create(vinil_size default_page_size,
                                              vinil_jump_buffer* handler);

/**
 * Set the allocation error handler for the pool
 *
 * @param pool Memory pool
 * @param handler Jump buffer to use for allocation failures
 */
void vinil_memory_pool_set_handler(vinil_memory_pool* pool,
                                     vinil_jump_buffer* handler);

/**
 * Clear all allocations in the pool and reset to initial state
 *
 * @param pool Memory pool to clear
 */
void vinil_memory_pool_clear(vinil_memory_pool* pool);

/**
 * Destroy the memory pool and deallocate all associated memory
 *
 * @param pool Memory pool to destroy
 */
void vinil_memory_pool_destroy(vinil_memory_pool* pool);

/**
 * Allocate memory from the pool
 *
 * @param pool Memory pool
 * @param amount Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* vinil_memory_pool_allocate(vinil_memory_pool* pool, vinil_size amount);

#ifdef __cplusplus
}
#endif

#endif /* VINIL_MEMORY_H */
