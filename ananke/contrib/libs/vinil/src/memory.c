/*
** ==========================================================================
**
** VINIL Memory Management - Implementation
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

#include <vinil/memory.h>
#include <stdlib.h>
#include <string.h>

/*
** --------------------------------------------------------------------------
** Constants
** --------------------------------------------------------------------------
*/

#define HEAP_ALIGNMENT  8

/*
** --------------------------------------------------------------------------
** Internal Structures
** --------------------------------------------------------------------------
*/

typedef struct vinil_memory_page {
    struct vinil_memory_page*   next;       /* next page in pool      */
    uint8_t*                    base;       /* memory base address    */
    vinil_size                  total;      /* total memory in page   */
} vinil_memory_page;

struct vinil_memory_pool {
    vinil_jump_buffer*      handler;            /* exception handler      */
    vinil_memory_page*      pages;              /* list of pages          */
    vinil_size              default_page_size;  /* standard page size     */
    vinil_size              current;            /* used memory in page    */
};

/*
** --------------------------------------------------------------------------
** Internal Functions
** --------------------------------------------------------------------------
*/

static vinil_memory_page* create_page(vinil_size page_size) {
    vinil_memory_page* result = (vinil_memory_page*)malloc(sizeof(vinil_memory_page));

    if (result) {
        result->base = (uint8_t*)malloc(page_size);
        result->total = page_size;
        result->next = NULL;

        if (!result->base) {
            free(result);
            return NULL;
        }
    }

    return result;
}

/*
** --------------------------------------------------------------------------
** Public Functions
** --------------------------------------------------------------------------
*/

vinil_memory_pool* vinil_memory_pool_create(vinil_size default_page_size,
                                              vinil_jump_buffer* handler) {
    vinil_memory_pool* pool = (vinil_memory_pool*)malloc(sizeof(vinil_memory_pool));

    if (pool) {
        pool->handler = handler;
        pool->default_page_size = default_page_size;
        pool->current = 0;
        pool->pages = create_page(default_page_size);

        if (!pool->pages) {
            free(pool);
            return NULL;
        }
    }

    return pool;
}

void vinil_memory_pool_set_handler(vinil_memory_pool* pool,
                                     vinil_jump_buffer* handler) {
    if (pool) {
        pool->handler = handler;
    }
}

void vinil_memory_pool_destroy(vinil_memory_pool* pool) {
    if (!pool) {
        return;
    }

    vinil_memory_page* current;
    vinil_memory_page* next;

    for (current = pool->pages; current != NULL; current = next) {
        next = current->next;
        free(current->base);
        free(current);
    }

    free(pool);
}

void vinil_memory_pool_clear(vinil_memory_pool* pool) {
    if (!pool) {
        return;
    }

    vinil_memory_page* current;
    vinil_memory_page* next;

    for (current = pool->pages; current != NULL; current = next) {
        next = current->next;
        free(current->base);
        free(current);
    }

    pool->pages = NULL;
    pool->current = 0;
}

void* vinil_memory_pool_allocate(vinil_memory_pool* pool, vinil_size amount) {
    void* result;

    if (!pool) {
        return NULL;
    }

    /* Align to HEAP_ALIGNMENT bytes */
    amount = (amount + HEAP_ALIGNMENT - 1) & ~(HEAP_ALIGNMENT - 1);

    /* Check if current page has enough space */
    if (!pool->pages || pool->pages->total - pool->current < amount) {
        vinil_memory_page* new_page;

        /* Allocate larger page if requested amount exceeds default */
        if (amount > pool->default_page_size) {
            new_page = create_page(amount);
        } else {
            new_page = create_page(pool->default_page_size);
        }

        if (!new_page) {
            if (pool->handler) {
                longjmp(*pool->handler, 1);
            } else {
                return NULL;
            }
        }

        /* Insert new page at front of list */
        new_page->next = pool->pages;
        pool->pages = new_page;
        pool->current = 0;
    }

    /* Allocate from current page */
    result = pool->pages->base + pool->current;
    pool->current += amount;

    return result;
}
