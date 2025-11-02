/*
** ==========================================================================
**
** VINIL Type System - Implementation
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

#include <vinil/types.h>
#include <stdlib.h>
#include <string.h>

/*
** ==========================================================================
** INTERNAL STRUCTURES
** ==========================================================================
*/

/* Cache for basic types (shared instances) */
#define MAX_BASIC_TYPES 256

static struct {
    vinil_bool          initialized;
    vinil_type          basic_types[MAX_BASIC_TYPES];
    vinil_uint32        num_basic_types;
} type_cache = { VINIL_FALSE, };

/*
** ==========================================================================
** TYPE QUERIES
** ==========================================================================
*/

vinil_bool vinil_type_is_primitive(const vinil_type* type) {
    if (!type) return VINIL_FALSE;

    switch (type->base.kind) {
    case VINIL_TYPE_BOOL:
    case VINIL_TYPE_INT:
    case VINIL_TYPE_UINT:
    case VINIL_TYPE_FLOAT:
    case VINIL_TYPE_DOUBLE:
    case VINIL_TYPE_HALF:
    case VINIL_TYPE_CHAR:
    case VINIL_TYPE_UCHAR:
    case VINIL_TYPE_SHORT:
    case VINIL_TYPE_USHORT:
    case VINIL_TYPE_LONG:
    case VINIL_TYPE_ULONG:
    case VINIL_TYPE_BOOL_VEC2:
    case VINIL_TYPE_BOOL_VEC3:
    case VINIL_TYPE_BOOL_VEC4:
    case VINIL_TYPE_INT_VEC2:
    case VINIL_TYPE_INT_VEC3:
    case VINIL_TYPE_INT_VEC4:
    case VINIL_TYPE_UINT_VEC2:
    case VINIL_TYPE_UINT_VEC3:
    case VINIL_TYPE_UINT_VEC4:
    case VINIL_TYPE_FLOAT_VEC2:
    case VINIL_TYPE_FLOAT_VEC3:
    case VINIL_TYPE_FLOAT_VEC4:
    case VINIL_TYPE_DOUBLE_VEC2:
    case VINIL_TYPE_DOUBLE_VEC3:
    case VINIL_TYPE_DOUBLE_VEC4:
    case VINIL_TYPE_FLOAT_MAT2:
    case VINIL_TYPE_FLOAT_MAT3:
    case VINIL_TYPE_FLOAT_MAT4:
        return VINIL_TRUE;
    default:
        return VINIL_FALSE;
    }
}

vinil_bool vinil_type_is_scalar(const vinil_type* type) {
    if (!type) return VINIL_FALSE;

    switch (type->base.kind) {
    case VINIL_TYPE_BOOL:
    case VINIL_TYPE_INT:
    case VINIL_TYPE_UINT:
    case VINIL_TYPE_FLOAT:
    case VINIL_TYPE_DOUBLE:
    case VINIL_TYPE_HALF:
    case VINIL_TYPE_CHAR:
    case VINIL_TYPE_UCHAR:
    case VINIL_TYPE_SHORT:
    case VINIL_TYPE_USHORT:
    case VINIL_TYPE_LONG:
    case VINIL_TYPE_ULONG:
        return VINIL_TRUE;
    default:
        return VINIL_FALSE;
    }
}

vinil_bool vinil_type_is_vector(const vinil_type* type) {
    if (!type) return VINIL_FALSE;

    switch (type->base.kind) {
    case VINIL_TYPE_BOOL_VEC2:
    case VINIL_TYPE_BOOL_VEC3:
    case VINIL_TYPE_BOOL_VEC4:
    case VINIL_TYPE_INT_VEC2:
    case VINIL_TYPE_INT_VEC3:
    case VINIL_TYPE_INT_VEC4:
    case VINIL_TYPE_UINT_VEC2:
    case VINIL_TYPE_UINT_VEC3:
    case VINIL_TYPE_UINT_VEC4:
    case VINIL_TYPE_FLOAT_VEC2:
    case VINIL_TYPE_FLOAT_VEC3:
    case VINIL_TYPE_FLOAT_VEC4:
    case VINIL_TYPE_DOUBLE_VEC2:
    case VINIL_TYPE_DOUBLE_VEC3:
    case VINIL_TYPE_DOUBLE_VEC4:
        return VINIL_TRUE;
    default:
        return VINIL_FALSE;
    }
}

vinil_bool vinil_type_is_matrix(const vinil_type* type) {
    if (!type) return VINIL_FALSE;

    switch (type->base.kind) {
    case VINIL_TYPE_FLOAT_MAT2:
    case VINIL_TYPE_FLOAT_MAT3:
    case VINIL_TYPE_FLOAT_MAT4:
    case VINIL_TYPE_DOUBLE_MAT2:
    case VINIL_TYPE_DOUBLE_MAT3:
    case VINIL_TYPE_DOUBLE_MAT4:
        return VINIL_TRUE;
    default:
        return VINIL_FALSE;
    }
}

vinil_bool vinil_type_is_float(const vinil_type* type) {
    if (!type) return VINIL_FALSE;

    switch (type->base.kind) {
    case VINIL_TYPE_FLOAT:
    case VINIL_TYPE_DOUBLE:
    case VINIL_TYPE_HALF:
    case VINIL_TYPE_FLOAT_VEC2:
    case VINIL_TYPE_FLOAT_VEC3:
    case VINIL_TYPE_FLOAT_VEC4:
    case VINIL_TYPE_DOUBLE_VEC2:
    case VINIL_TYPE_DOUBLE_VEC3:
    case VINIL_TYPE_DOUBLE_VEC4:
    case VINIL_TYPE_FLOAT_MAT2:
    case VINIL_TYPE_FLOAT_MAT3:
    case VINIL_TYPE_FLOAT_MAT4:
    case VINIL_TYPE_DOUBLE_MAT2:
    case VINIL_TYPE_DOUBLE_MAT3:
    case VINIL_TYPE_DOUBLE_MAT4:
        return VINIL_TRUE;
    default:
        return VINIL_FALSE;
    }
}

vinil_bool vinil_type_is_integer(const vinil_type* type) {
    if (!type) return VINIL_FALSE;

    switch (type->base.kind) {
    case VINIL_TYPE_INT:
    case VINIL_TYPE_UINT:
    case VINIL_TYPE_CHAR:
    case VINIL_TYPE_UCHAR:
    case VINIL_TYPE_SHORT:
    case VINIL_TYPE_USHORT:
    case VINIL_TYPE_LONG:
    case VINIL_TYPE_ULONG:
    case VINIL_TYPE_INT_VEC2:
    case VINIL_TYPE_INT_VEC3:
    case VINIL_TYPE_INT_VEC4:
    case VINIL_TYPE_UINT_VEC2:
    case VINIL_TYPE_UINT_VEC3:
    case VINIL_TYPE_UINT_VEC4:
        return VINIL_TRUE;
    default:
        return VINIL_FALSE;
    }
}

vinil_bool vinil_type_is_bool(const vinil_type* type) {
    if (!type) return VINIL_FALSE;

    switch (type->base.kind) {
    case VINIL_TYPE_BOOL:
    case VINIL_TYPE_BOOL_VEC2:
    case VINIL_TYPE_BOOL_VEC3:
    case VINIL_TYPE_BOOL_VEC4:
        return VINIL_TRUE;
    default:
        return VINIL_FALSE;
    }
}

vinil_uint32 vinil_type_get_components(const vinil_type* type) {
    if (!type) return 0;

    /* Extract component count from type value */
    switch (type->base.kind) {
    /* Scalars */
    case VINIL_TYPE_BOOL:
    case VINIL_TYPE_INT:
    case VINIL_TYPE_UINT:
    case VINIL_TYPE_FLOAT:
    case VINIL_TYPE_DOUBLE:
    case VINIL_TYPE_HALF:
    case VINIL_TYPE_CHAR:
    case VINIL_TYPE_UCHAR:
    case VINIL_TYPE_SHORT:
    case VINIL_TYPE_USHORT:
    case VINIL_TYPE_LONG:
    case VINIL_TYPE_ULONG:
        return 1;

    /* Vec2 */
    case VINIL_TYPE_BOOL_VEC2:
    case VINIL_TYPE_INT_VEC2:
    case VINIL_TYPE_UINT_VEC2:
    case VINIL_TYPE_FLOAT_VEC2:
    case VINIL_TYPE_DOUBLE_VEC2:
        return 2;

    /* Vec3 */
    case VINIL_TYPE_BOOL_VEC3:
    case VINIL_TYPE_INT_VEC3:
    case VINIL_TYPE_UINT_VEC3:
    case VINIL_TYPE_FLOAT_VEC3:
    case VINIL_TYPE_DOUBLE_VEC3:
        return 3;

    /* Vec4 */
    case VINIL_TYPE_BOOL_VEC4:
    case VINIL_TYPE_INT_VEC4:
    case VINIL_TYPE_UINT_VEC4:
    case VINIL_TYPE_FLOAT_VEC4:
    case VINIL_TYPE_DOUBLE_VEC4:
        return 4;

    /* Matrices */
    case VINIL_TYPE_FLOAT_MAT2:
    case VINIL_TYPE_DOUBLE_MAT2:
        return 4;
    case VINIL_TYPE_FLOAT_MAT3:
    case VINIL_TYPE_DOUBLE_MAT3:
        return 9;
    case VINIL_TYPE_FLOAT_MAT4:
    case VINIL_TYPE_DOUBLE_MAT4:
        return 16;

    default:
        return 0;
    }
}

vinil_type_value vinil_type_get_scalar_type(const vinil_type* type) {
    if (!type) return VINIL_TYPE_INVALID;

    switch (type->base.kind) {
    /* Already scalar */
    case VINIL_TYPE_BOOL:
    case VINIL_TYPE_INT:
    case VINIL_TYPE_UINT:
    case VINIL_TYPE_FLOAT:
    case VINIL_TYPE_DOUBLE:
    case VINIL_TYPE_HALF:
    case VINIL_TYPE_CHAR:
    case VINIL_TYPE_UCHAR:
    case VINIL_TYPE_SHORT:
    case VINIL_TYPE_USHORT:
    case VINIL_TYPE_LONG:
    case VINIL_TYPE_ULONG:
        return type->base.kind;

    /* Boolean vectors */
    case VINIL_TYPE_BOOL_VEC2:
    case VINIL_TYPE_BOOL_VEC3:
    case VINIL_TYPE_BOOL_VEC4:
        return VINIL_TYPE_BOOL;

    /* Integer vectors */
    case VINIL_TYPE_INT_VEC2:
    case VINIL_TYPE_INT_VEC3:
    case VINIL_TYPE_INT_VEC4:
        return VINIL_TYPE_INT;

    /* Unsigned integer vectors */
    case VINIL_TYPE_UINT_VEC2:
    case VINIL_TYPE_UINT_VEC3:
    case VINIL_TYPE_UINT_VEC4:
        return VINIL_TYPE_UINT;

    /* Float vectors and matrices */
    case VINIL_TYPE_FLOAT_VEC2:
    case VINIL_TYPE_FLOAT_VEC3:
    case VINIL_TYPE_FLOAT_VEC4:
    case VINIL_TYPE_FLOAT_MAT2:
    case VINIL_TYPE_FLOAT_MAT3:
    case VINIL_TYPE_FLOAT_MAT4:
        return VINIL_TYPE_FLOAT;

    /* Double vectors and matrices */
    case VINIL_TYPE_DOUBLE_VEC2:
    case VINIL_TYPE_DOUBLE_VEC3:
    case VINIL_TYPE_DOUBLE_VEC4:
    case VINIL_TYPE_DOUBLE_MAT2:
    case VINIL_TYPE_DOUBLE_MAT3:
    case VINIL_TYPE_DOUBLE_MAT4:
        return VINIL_TYPE_DOUBLE;

    default:
        return VINIL_TYPE_INVALID;
    }
}

/*
** ==========================================================================
** TYPE CREATION
** ==========================================================================
*/

static vinil_type* create_basic_type(vinil_type_value kind,
                                      vinil_precision prec,
                                      vinil_uint32 components,
                                      vinil_size size_words) {
    if (type_cache.num_basic_types >= MAX_BASIC_TYPES) {
        return NULL;
    }

    vinil_type* type = &type_cache.basic_types[type_cache.num_basic_types++];
    memset(type, 0, sizeof(vinil_type));

    type->base.kind = kind;
    type->base.prec = prec;
    type->base.elements = components;
    type->base.size = size_words;

    return type;
}

vinil_type* vinil_type_get_basic(vinil_type_value type, vinil_precision prec) {
    /* Search cache for existing type */
    for (vinil_uint32 i = 0; i < type_cache.num_basic_types; i++) {
        vinil_type* cached = &type_cache.basic_types[i];
        if (cached->base.kind == type && cached->base.prec == prec) {
            return cached;
        }
    }

    /* Create new basic type */
    vinil_uint32 components = vinil_type_get_components(&(vinil_type){ .base = { .kind = type } });
    vinil_size size_words = (components + 3) / 4; /* Round up to vec4 */

    return create_basic_type(type, prec, components, size_words);
}

vinil_type* vinil_type_create_array(vinil_memory_pool* pool,
                                     vinil_type* element_type,
                                     vinil_size num_elements) {
    if (!pool || !element_type || num_elements == 0) {
        return NULL;
    }

    vinil_type* type = (vinil_type*)vinil_memory_pool_allocate(pool, sizeof(vinil_type));
    if (!type) {
        return NULL;
    }

    type->array.base.kind = VINIL_TYPE_ARRAY;
    type->array.base.size = element_type->base.size * num_elements;
    type->array.elements = num_elements;
    type->array.element_type = element_type;

    return type;
}

vinil_bool vinil_type_matches(const vinil_type* first, const vinil_type* second) {
    if (first == second) return VINIL_TRUE;
    if (!first || !second) return VINIL_FALSE;

    /* Basic type must match */
    if (first->base.kind != second->base.kind) {
        return VINIL_FALSE;
    }

    /* For arrays, check element type and size */
    if (first->base.kind == VINIL_TYPE_ARRAY) {
        return first->array.elements == second->array.elements &&
               vinil_type_matches(first->array.element_type, second->array.element_type);
    }

    /* For other composite types, do deep comparison */
    /* TODO: Implement struct and function type matching */

    return VINIL_TRUE;
}

/*
** ==========================================================================
** TYPE SYSTEM INITIALIZATION
** ==========================================================================
*/

void vinil_type_system_init(void) {
    if (type_cache.initialized) {
        return;
    }

    memset(&type_cache, 0, sizeof(type_cache));

    /* Pre-create common basic types */
    vinil_type_get_basic(VINIL_TYPE_VOID, VINIL_PRECISION_UNDEFINED);
    vinil_type_get_basic(VINIL_TYPE_BOOL, VINIL_PRECISION_UNDEFINED);
    vinil_type_get_basic(VINIL_TYPE_INT, VINIL_PRECISION_HIGH);
    vinil_type_get_basic(VINIL_TYPE_UINT, VINIL_PRECISION_HIGH);
    vinil_type_get_basic(VINIL_TYPE_FLOAT, VINIL_PRECISION_HIGH);
    vinil_type_get_basic(VINIL_TYPE_FLOAT_VEC2, VINIL_PRECISION_HIGH);
    vinil_type_get_basic(VINIL_TYPE_FLOAT_VEC3, VINIL_PRECISION_HIGH);
    vinil_type_get_basic(VINIL_TYPE_FLOAT_VEC4, VINIL_PRECISION_HIGH);
    vinil_type_get_basic(VINIL_TYPE_FLOAT_MAT2, VINIL_PRECISION_HIGH);
    vinil_type_get_basic(VINIL_TYPE_FLOAT_MAT3, VINIL_PRECISION_HIGH);
    vinil_type_get_basic(VINIL_TYPE_FLOAT_MAT4, VINIL_PRECISION_HIGH);

    type_cache.initialized = VINIL_TRUE;
}

void vinil_type_system_shutdown(void) {
    memset(&type_cache, 0, sizeof(type_cache));
}

const char* vinil_type_get_name(const vinil_type* type) {
    if (!type) return "null";

    switch (type->base.kind) {
    case VINIL_TYPE_VOID: return "void";
    case VINIL_TYPE_BOOL: return "bool";
    case VINIL_TYPE_INT: return "int";
    case VINIL_TYPE_UINT: return "uint";
    case VINIL_TYPE_FLOAT: return "float";
    case VINIL_TYPE_DOUBLE: return "double";
    case VINIL_TYPE_FLOAT_VEC2: return "vec2";
    case VINIL_TYPE_FLOAT_VEC3: return "vec3";
    case VINIL_TYPE_FLOAT_VEC4: return "vec4";
    case VINIL_TYPE_INT_VEC2: return "ivec2";
    case VINIL_TYPE_INT_VEC3: return "ivec3";
    case VINIL_TYPE_INT_VEC4: return "ivec4";
    case VINIL_TYPE_FLOAT_MAT2: return "mat2";
    case VINIL_TYPE_FLOAT_MAT3: return "mat3";
    case VINIL_TYPE_FLOAT_MAT4: return "mat4";
    case VINIL_TYPE_ARRAY: return "array";
    case VINIL_TYPE_STRUCT: return "struct";
    case VINIL_TYPE_FUNCTION: return "function";
    case VINIL_TYPE_POINTER: return "pointer";
    default: return "unknown";
    }
}
