/*
** ==========================================================================
**
** VINIL IL Implementation Structures
**
** Internal structures for IL implementation
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

#ifndef VINIL_IL_IMPL_H
#define VINIL_IL_IMPL_H 1

#include <vinil/il.h>
#include <vinil/types.h>
#include <vinil/memory.h>

/*
** ==========================================================================
** FORWARD DECLARATIONS
** ==========================================================================
*/

/* vinil_inst is defined in il.h as a union */
typedef struct vinil_block vinil_block;
typedef struct vinil_label vinil_label;
typedef struct vinil_variable vinil_variable;
typedef struct vinil_program_impl vinil_program_impl;

/*
** ==========================================================================
** VARIABLES AND REGISTERS
** ==========================================================================
*/

typedef enum vinil_var_kind {
    VINIL_VAR_PARAM,        /* Parameter/uniform */
    VINIL_VAR_TEMP,         /* Temporary register */
    VINIL_VAR_INPUT,        /* Input (attribute/varying) */
    VINIL_VAR_OUTPUT,       /* Output (varying/fragment) */
    VINIL_VAR_CONST,        /* Constant */
    VINIL_VAR_ADDR,         /* Address register */
} vinil_var_kind;

typedef enum vinil_var_segment {
    VINIL_SEG_NONE,
    VINIL_SEG_PARAM,        /* Parameter segment */
    VINIL_SEG_ATTRIB,       /* Attribute segment */
    VINIL_SEG_VARYING,      /* Varying segment */
    VINIL_SEG_LOCAL,        /* Local/temp segment */
} vinil_var_segment;

struct vinil_variable {
    vinil_variable*     next;           /* Linked list */
    vinil_var_kind      kind;           /* Variable kind */
    vinil_uint32        id;             /* Unique identifier */
    vinil_type*         type;           /* Variable type */
    vinil_var_segment   segment;        /* Memory segment */
    vinil_uint32        location;       /* Offset in segment */
    vinil_uint32        shift;          /* Index within vec4 */
    vinil_bool          used;           /* Usage flag */
    const char*         name;           /* Variable name (optional) */
    vinil_size          name_length;    /* Name length */
};

/* Note: Operand and instruction structures are now defined in il.h */

/*
** ==========================================================================
** BASIC BLOCKS
** ==========================================================================
*/

struct vinil_block {
    vinil_uint32        id;             /* Block identifier */
    vinil_block*        prev;           /* Doubly-linked list */
    vinil_block*        next;
    union vinil_inst*   first;          /* First instruction */
    union vinil_inst*   last;           /* Last instruction */
};

struct vinil_label {
    vinil_label*        next;           /* Linked list */
    vinil_block*        target;         /* Target block */
    const char*         name;           /* Label name (optional) */
    vinil_size          name_length;
};

/*
** ==========================================================================
** PROGRAM
** ==========================================================================
*/

struct vinil_program_impl {
    vinil_memory_pool*  memory;         /* Memory pool */
    vinil_block*        blocks;         /* Block list */
    vinil_label*        labels;         /* Label list */
    vinil_label*        entry;          /* Entry point */
    vinil_uint32        num_blocks;     /* Block count */
    vinil_uint32        num_vars;       /* Variable count */

    /* Variable lists */
    vinil_variable*     params;         /* Parameters/uniforms */
    vinil_variable*     temps;          /* Temporaries */
    vinil_variable*     inputs;         /* Inputs */
    vinil_variable*     outputs;        /* Outputs */
};

/*
** ==========================================================================
** BUILDER FUNCTIONS
** ==========================================================================
*/

/**
 * Create a new block
 */
vinil_block* vinil_block_create(vinil_program_impl* prog);

/**
 * Create a new label
 */
vinil_label* vinil_label_create(vinil_program_impl* prog, const char* name, vinil_size name_len);

/**
 * Create a new variable
 */
vinil_variable* vinil_variable_create(vinil_program_impl* prog,
                                      vinil_var_kind kind,
                                      vinil_type* type,
                                      const char* name,
                                      vinil_size name_len);

/**
 * Append instruction to block
 */
void vinil_block_append(vinil_block* block, union vinil_inst* inst);

/**
 * Create unary instruction
 */
union vinil_inst* vinil_inst_create_unary(vinil_program_impl* prog,
                                    vinil_opcode opcode,
                                    vinil_dst_operand* dst,
                                    vinil_src_operand* src);

/**
 * Create binary instruction
 */
union vinil_inst* vinil_inst_create_binary(vinil_program_impl* prog,
                                     vinil_opcode opcode,
                                     vinil_dst_operand* dst,
                                     vinil_src_operand* src1,
                                     vinil_src_operand* src2);

/**
 * Create ternary instruction
 */
union vinil_inst* vinil_inst_create_ternary(vinil_program_impl* prog,
                                      vinil_opcode opcode,
                                      vinil_dst_operand* dst,
                                      vinil_src_operand* src1,
                                      vinil_src_operand* src2,
                                      vinil_src_operand* src3);

#endif /* VINIL_IL_IMPL_H */
