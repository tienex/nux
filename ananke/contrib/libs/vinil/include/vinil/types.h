/*
** ==========================================================================
**
** VINIL Type System
**
** Unified type system for graphics and compute workloads
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

#ifndef VINIL_TYPES_H
#define VINIL_TYPES_H 1

#include <vinil/vinil.h>
#include <vinil/memory.h>
#include <vinil/il.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** ==========================================================================
** TYPE VALUES
** ==========================================================================
*/

typedef enum vinil_type_value {
    /* Invalid/special types */
    VINIL_TYPE_INVALID = 0,
    VINIL_TYPE_VOID,

    /* Scalar types - Graphics + Compute */
    VINIL_TYPE_BOOL,
    VINIL_TYPE_INT,
    VINIL_TYPE_UINT,        /* Compute extension */
    VINIL_TYPE_FLOAT,
    VINIL_TYPE_DOUBLE,      /* Compute extension */
    VINIL_TYPE_HALF,        /* Compute extension (fp16) */

    /* Additional scalar types for compute */
    VINIL_TYPE_CHAR,        /* 8-bit signed */
    VINIL_TYPE_UCHAR,       /* 8-bit unsigned */
    VINIL_TYPE_SHORT,       /* 16-bit signed */
    VINIL_TYPE_USHORT,      /* 16-bit unsigned */
    VINIL_TYPE_LONG,        /* 64-bit signed */
    VINIL_TYPE_ULONG,       /* 64-bit unsigned */

    /* Boolean vectors - Graphics + Compute */
    VINIL_TYPE_BOOL_VEC2,
    VINIL_TYPE_BOOL_VEC3,
    VINIL_TYPE_BOOL_VEC4,
    VINIL_TYPE_BOOL_VEC8,   /* Compute extension */
    VINIL_TYPE_BOOL_VEC16,  /* Compute extension */

    /* Integer vectors - Graphics + Compute */
    VINIL_TYPE_INT_VEC2,
    VINIL_TYPE_INT_VEC3,
    VINIL_TYPE_INT_VEC4,
    VINIL_TYPE_INT_VEC8,    /* Compute extension */
    VINIL_TYPE_INT_VEC16,   /* Compute extension */

    /* Unsigned integer vectors - Compute */
    VINIL_TYPE_UINT_VEC2,
    VINIL_TYPE_UINT_VEC3,
    VINIL_TYPE_UINT_VEC4,
    VINIL_TYPE_UINT_VEC8,
    VINIL_TYPE_UINT_VEC16,

    /* Float vectors - Graphics + Compute */
    VINIL_TYPE_FLOAT_VEC2,
    VINIL_TYPE_FLOAT_VEC3,
    VINIL_TYPE_FLOAT_VEC4,
    VINIL_TYPE_FLOAT_VEC8,  /* Compute extension */
    VINIL_TYPE_FLOAT_VEC16, /* Compute extension */

    /* Double vectors - Compute */
    VINIL_TYPE_DOUBLE_VEC2,
    VINIL_TYPE_DOUBLE_VEC3,
    VINIL_TYPE_DOUBLE_VEC4,
    VINIL_TYPE_DOUBLE_VEC8,
    VINIL_TYPE_DOUBLE_VEC16,

    /* Half vectors - Compute (fp16) */
    VINIL_TYPE_HALF_VEC2,
    VINIL_TYPE_HALF_VEC3,
    VINIL_TYPE_HALF_VEC4,
    VINIL_TYPE_HALF_VEC8,
    VINIL_TYPE_HALF_VEC16,

    /* Char vectors - Compute */
    VINIL_TYPE_CHAR_VEC2,
    VINIL_TYPE_CHAR_VEC3,
    VINIL_TYPE_CHAR_VEC4,
    VINIL_TYPE_CHAR_VEC8,
    VINIL_TYPE_CHAR_VEC16,

    /* Unsigned char vectors - Compute */
    VINIL_TYPE_UCHAR_VEC2,
    VINIL_TYPE_UCHAR_VEC3,
    VINIL_TYPE_UCHAR_VEC4,
    VINIL_TYPE_UCHAR_VEC8,
    VINIL_TYPE_UCHAR_VEC16,

    /* Short vectors - Compute */
    VINIL_TYPE_SHORT_VEC2,
    VINIL_TYPE_SHORT_VEC3,
    VINIL_TYPE_SHORT_VEC4,
    VINIL_TYPE_SHORT_VEC8,
    VINIL_TYPE_SHORT_VEC16,

    /* Unsigned short vectors - Compute */
    VINIL_TYPE_USHORT_VEC2,
    VINIL_TYPE_USHORT_VEC3,
    VINIL_TYPE_USHORT_VEC4,
    VINIL_TYPE_USHORT_VEC8,
    VINIL_TYPE_USHORT_VEC16,

    /* Long vectors - Compute */
    VINIL_TYPE_LONG_VEC2,
    VINIL_TYPE_LONG_VEC3,
    VINIL_TYPE_LONG_VEC4,
    VINIL_TYPE_LONG_VEC8,
    VINIL_TYPE_LONG_VEC16,

    /* Unsigned long vectors - Compute */
    VINIL_TYPE_ULONG_VEC2,
    VINIL_TYPE_ULONG_VEC3,
    VINIL_TYPE_ULONG_VEC4,
    VINIL_TYPE_ULONG_VEC8,
    VINIL_TYPE_ULONG_VEC16,

    /* Matrices - Graphics */
    VINIL_TYPE_FLOAT_MAT2,
    VINIL_TYPE_FLOAT_MAT3,
    VINIL_TYPE_FLOAT_MAT4,
    VINIL_TYPE_FLOAT_MAT2x3,    /* OpenGL extension */
    VINIL_TYPE_FLOAT_MAT2x4,
    VINIL_TYPE_FLOAT_MAT3x2,
    VINIL_TYPE_FLOAT_MAT3x4,
    VINIL_TYPE_FLOAT_MAT4x2,
    VINIL_TYPE_FLOAT_MAT4x3,

    /* Double matrices - Compute */
    VINIL_TYPE_DOUBLE_MAT2,
    VINIL_TYPE_DOUBLE_MAT3,
    VINIL_TYPE_DOUBLE_MAT4,

    /* Samplers - Graphics */
    VINIL_TYPE_SAMPLER_2D,
    VINIL_TYPE_SAMPLER_3D,
    VINIL_TYPE_SAMPLER_CUBE,
    VINIL_TYPE_SAMPLER_2D_SHADOW,
    VINIL_TYPE_SAMPLER_2D_ARRAY,

    /* Images - Compute (read/write access) */
    VINIL_TYPE_IMAGE_1D,
    VINIL_TYPE_IMAGE_2D,
    VINIL_TYPE_IMAGE_3D,
    VINIL_TYPE_IMAGE_1D_ARRAY,
    VINIL_TYPE_IMAGE_2D_ARRAY,

    /* Pointers - Compute */
    VINIL_TYPE_POINTER,

    /* Composite types */
    VINIL_TYPE_ARRAY,
    VINIL_TYPE_STRUCT,
    VINIL_TYPE_FUNCTION,
} vinil_type_value;

/* Note: vinil_precision and vinil_addr_space are defined in il.h */

/*
** ==========================================================================
** PARAMETER DIRECTION (for functions)
** ==========================================================================
*/

typedef enum vinil_param_dir {
    VINIL_PARAM_IN      = 0x01,
    VINIL_PARAM_OUT     = 0x02,
    VINIL_PARAM_INOUT   = 0x03,
} vinil_param_dir;

/*
** ==========================================================================
** TYPE STRUCTURES
** ==========================================================================
*/

typedef union vinil_type vinil_type;
typedef struct vinil_parameter vinil_parameter;
typedef struct vinil_field vinil_field;

/* Base type structure (common to all types) */
typedef struct vinil_type_base {
    vinil_type_value    kind;           /* Type discriminator */
    vinil_size          size;           /* Size in vec4 words */
    vinil_uint32        elements : 8;   /* For basic types */
    vinil_precision     prec : 2;       /* Precision qualifier */
    vinil_address_space addr_space : 2; /* For pointers (compute) */
    vinil_uint32        is_const : 1;   /* Const qualifier */
    vinil_uint32        is_volatile : 1;/* Volatile qualifier */
} vinil_type_base;

/* Array type */
typedef struct vinil_type_array {
    vinil_type_base     base;
    vinil_size          elements;       /* Number of elements */
    vinil_type*         element_type;   /* Element type */
} vinil_type_array;

/* Struct field */
struct vinil_field {
    vinil_type*         type;           /* Field type */
    vinil_size          offset;         /* Offset in struct */
    const char*         name;           /* Field name */
    vinil_size          name_length;    /* Name length */
};

/* Struct type */
typedef struct vinil_type_struct {
    vinil_type_base     base;
    vinil_size          num_fields;     /* Number of fields */
    vinil_field*        fields;         /* Array of fields */
    const char*         name;           /* Struct name (optional) */
    vinil_size          name_length;    /* Name length */
} vinil_type_struct;

/* Function parameter */
struct vinil_parameter {
    vinil_type*         type;           /* Parameter type */
    vinil_param_dir     direction;      /* In/out/inout */
    const char*         name;           /* Parameter name (optional) */
    vinil_size          name_length;    /* Name length */
};

/* Function type */
typedef struct vinil_type_function {
    vinil_type_base     base;
    vinil_size          num_params;     /* Number of parameters */
    vinil_type*         return_type;    /* Return type */
    vinil_parameter     parameters[];   /* Flexible array */
} vinil_type_function;

/* Pointer type (compute) */
typedef struct vinil_type_pointer {
    vinil_type_base     base;
    vinil_type*         pointee_type;   /* Type being pointed to */
} vinil_type_pointer;

/* Union of all type variants */
union vinil_type {
    vinil_type_base     base;
    vinil_type_array    array;
    vinil_type_struct   structure;
    vinil_type_function function;
    vinil_type_pointer  pointer;
};

/*
** ==========================================================================
** TYPE QUERIES
** ==========================================================================
*/

/**
 * Check if a type is a primitive (scalar or vector)
 */
vinil_bool vinil_type_is_primitive(const vinil_type* type);

/**
 * Check if a type is a scalar
 */
vinil_bool vinil_type_is_scalar(const vinil_type* type);

/**
 * Check if a type is a vector
 */
vinil_bool vinil_type_is_vector(const vinil_type* type);

/**
 * Check if a type is a matrix
 */
vinil_bool vinil_type_is_matrix(const vinil_type* type);

/**
 * Check if a type is floating-point
 */
vinil_bool vinil_type_is_float(const vinil_type* type);

/**
 * Check if a type is integer
 */
vinil_bool vinil_type_is_integer(const vinil_type* type);

/**
 * Check if a type is boolean
 */
vinil_bool vinil_type_is_bool(const vinil_type* type);

/**
 * Get the number of components in a vector/matrix type
 */
vinil_uint32 vinil_type_get_components(const vinil_type* type);

/**
 * Get the base scalar type of a vector/matrix
 */
vinil_type_value vinil_type_get_scalar_type(const vinil_type* type);

/*
** ==========================================================================
** TYPE CREATION
** ==========================================================================
*/

/**
 * Get a basic type (cached, shared instances)
 */
vinil_type* vinil_type_get_basic(vinil_type_value type, vinil_precision prec);

/**
 * Create a vector type
 */
vinil_type* vinil_type_create_vector(vinil_type_value base_type,
                                      vinil_precision prec,
                                      vinil_uint32 dimensions);

/**
 * Create a matrix type
 */
vinil_type* vinil_type_create_matrix(vinil_type_value base_type,
                                      vinil_precision prec,
                                      vinil_uint32 rows,
                                      vinil_uint32 cols);

/**
 * Create an array type
 */
vinil_type* vinil_type_create_array(vinil_memory_pool* pool,
                                     vinil_type* element_type,
                                     vinil_size num_elements);

/**
 * Create a struct type
 */
vinil_type* vinil_type_create_struct(vinil_memory_pool* pool,
                                      const char* name,
                                      vinil_size name_length);

/**
 * Add a field to a struct type
 */
vinil_error vinil_type_struct_add_field(vinil_type* struct_type,
                                         const char* field_name,
                                         vinil_size name_length,
                                         vinil_type* field_type);

/**
 * Create a function type
 */
vinil_type* vinil_type_create_function(vinil_memory_pool* pool,
                                        vinil_type* return_type,
                                        vinil_size num_params);

/**
 * Create a pointer type (compute)
 */
vinil_type* vinil_type_create_pointer(vinil_memory_pool* pool,
                                       vinil_type* pointee_type,
                                       vinil_address_space addr_space);

/*
** ==========================================================================
** TYPE MATCHING
** ==========================================================================
*/

/**
 * Check if two types match exactly
 */
vinil_bool vinil_type_matches(const vinil_type* first, const vinil_type* second);

/**
 * Check if types are compatible for assignment
 */
vinil_bool vinil_type_compatible(const vinil_type* dest, const vinil_type* src);

/**
 * Get the common type for binary operations
 */
vinil_type* vinil_type_common(const vinil_type* first, const vinil_type* second);

/*
** ==========================================================================
** TYPE UTILITIES
** ==========================================================================
*/

/**
 * Get the size of a type in bytes
 */
vinil_size vinil_type_get_size_bytes(const vinil_type* type);

/**
 * Get type name as string (for debugging)
 */
const char* vinil_type_get_name(const vinil_type* type);

/**
 * Initialize the type system (must be called once)
 */
void vinil_type_system_init(void);

/**
 * Shutdown the type system
 */
void vinil_type_system_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* VINIL_TYPES_H */
