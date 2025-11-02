/*
** ==========================================================================
**
** VINIL IL Implementation
**
** Intermediate language construction and manipulation
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

#include "il_impl.h"
#include <string.h>

/*
** ==========================================================================
** OPCODE METADATA
** ==========================================================================
*/

static const vinil_opcode_info opcode_table[] = {
    /* Arithmetic operations */
    { VINIL_OP_ABS, "abs", VINIL_FLAG_ARITHMETIC, 1, VINIL_TRUE, VINIL_FALSE, VINIL_FALSE },
    { VINIL_OP_ADD, "add", VINIL_FLAG_ARITHMETIC, 2, VINIL_TRUE, VINIL_FALSE, VINIL_FALSE },
    { VINIL_OP_SUB, "sub", VINIL_FLAG_ARITHMETIC, 2, VINIL_TRUE, VINIL_FALSE, VINIL_FALSE },
    { VINIL_OP_MUL, "mul", VINIL_FLAG_ARITHMETIC, 2, VINIL_TRUE, VINIL_FALSE, VINIL_FALSE },
    { VINIL_OP_MAD, "mad", VINIL_FLAG_ARITHMETIC, 3, VINIL_TRUE, VINIL_FALSE, VINIL_FALSE },

    /* Vector operations */
    { VINIL_OP_DP3, "dp3", VINIL_FLAG_VECTOR, 2, VINIL_TRUE, VINIL_FALSE, VINIL_FALSE },
    { VINIL_OP_DP4, "dp4", VINIL_FLAG_VECTOR, 2, VINIL_TRUE, VINIL_FALSE, VINIL_FALSE },
    { VINIL_OP_MOV, "mov", VINIL_FLAG_VECTOR, 1, VINIL_TRUE, VINIL_FALSE, VINIL_FALSE },

    /* Control flow */
    { VINIL_OP_IF, "if", VINIL_FLAG_CONTROL_FLOW, 0, VINIL_FALSE, VINIL_FALSE, VINIL_FALSE },
    { VINIL_OP_ELSE, "else", VINIL_FLAG_CONTROL_FLOW, 0, VINIL_FALSE, VINIL_FALSE, VINIL_FALSE },
    { VINIL_OP_ENDIF, "endif", VINIL_FLAG_CONTROL_FLOW, 0, VINIL_FALSE, VINIL_FALSE, VINIL_FALSE },

    /* Sentinel */
    { VINIL_OP_COUNT, NULL, 0, 0, VINIL_FALSE, VINIL_FALSE, VINIL_FALSE },
};

const vinil_opcode_info* vinil_get_opcode_info(vinil_opcode op) {
    if (op >= VINIL_OP_COUNT) {
        return NULL;
    }

    /* Linear search for now - can optimize later with direct indexing */
    for (vinil_uint32 i = 0; opcode_table[i].name != NULL; i++) {
        if (opcode_table[i].opcode == op) {
            return &opcode_table[i];
        }
    }

    return NULL;
}

/*
** ==========================================================================
** BLOCK FUNCTIONS
** ==========================================================================
*/

vinil_block* vinil_block_create(vinil_program_impl* prog) {
    vinil_block* block = (vinil_block*)vinil_memory_pool_allocate(
        prog->memory, sizeof(vinil_block));

    if (!block) {
        return NULL;
    }

    memset(block, 0, sizeof(vinil_block));
    block->id = prog->num_blocks++;

    /* Append to program's block list */
    if (!prog->blocks) {
        prog->blocks = block;
    } else {
        vinil_block* last = prog->blocks;
        while (last->next) {
            last = last->next;
        }
        last->next = block;
        block->prev = last;
    }

    return block;
}

void vinil_block_append(vinil_block* block, union vinil_inst* inst) {
    if (!block || !inst) {
        return;
    }

    inst->base.prev = block->last;
    inst->base.next = NULL;

    if (block->last) {
        block->last->base.next = inst;
    } else {
        block->first = inst;
    }

    block->last = inst;
}

/*
** ==========================================================================
** LABEL FUNCTIONS
** ==========================================================================
*/

vinil_label* vinil_label_create(vinil_program_impl* prog,
                                 const char* name,
                                 vinil_size name_len) {
    vinil_label* label = (vinil_label*)vinil_memory_pool_allocate(
        prog->memory, sizeof(vinil_label));

    if (!label) {
        return NULL;
    }

    memset(label, 0, sizeof(vinil_label));

    if (name && name_len > 0) {
        char* name_copy = (char*)vinil_memory_pool_allocate(prog->memory, name_len + 1);
        if (name_copy) {
            memcpy(name_copy, name, name_len);
            name_copy[name_len] = '\0';
            label->name = name_copy;
            label->name_length = name_len;
        }
    }

    /* Append to program's label list */
    label->next = prog->labels;
    prog->labels = label;

    return label;
}

/*
** ==========================================================================
** VARIABLE FUNCTIONS
** ==========================================================================
*/

vinil_variable* vinil_variable_create(vinil_program_impl* prog,
                                      vinil_var_kind kind,
                                      vinil_type* type,
                                      const char* name,
                                      vinil_size name_len) {
    vinil_variable* var = (vinil_variable*)vinil_memory_pool_allocate(
        prog->memory, sizeof(vinil_variable));

    if (!var) {
        return NULL;
    }

    memset(var, 0, sizeof(vinil_variable));

    var->kind = kind;
    var->id = prog->num_vars++;
    var->type = type;

    if (name && name_len > 0) {
        char* name_copy = (char*)vinil_memory_pool_allocate(prog->memory, name_len + 1);
        if (name_copy) {
            memcpy(name_copy, name, name_len);
            name_copy[name_len] = '\0';
            var->name = name_copy;
            var->name_length = name_len;
        }
    }

    /* Add to appropriate list */
    switch (kind) {
    case VINIL_VAR_PARAM:
        var->next = prog->params;
        prog->params = var;
        var->segment = VINIL_SEG_PARAM;
        break;

    case VINIL_VAR_TEMP:
        var->next = prog->temps;
        prog->temps = var;
        var->segment = VINIL_SEG_LOCAL;
        break;

    case VINIL_VAR_INPUT:
        var->next = prog->inputs;
        prog->inputs = var;
        var->segment = VINIL_SEG_ATTRIB;
        break;

    case VINIL_VAR_OUTPUT:
        var->next = prog->outputs;
        prog->outputs = var;
        var->segment = VINIL_SEG_VARYING;
        break;

    case VINIL_VAR_CONST:
    case VINIL_VAR_ADDR:
        /* No segment for these */
        break;
    }

    return var;
}

/*
** ==========================================================================
** INSTRUCTION CREATION
** ==========================================================================
*/

union vinil_inst* vinil_inst_create_unary(vinil_program_impl* prog,
                                    vinil_opcode opcode,
                                    vinil_dst_operand* dst,
                                    vinil_src_operand* src) {
    union vinil_inst* inst = (union vinil_inst*)vinil_memory_pool_allocate(
        prog->memory, sizeof(union vinil_inst));

    if (!inst) {
        return NULL;
    }

    memset(inst, 0, sizeof(union vinil_inst));

    inst->base.kind = VINIL_INST_UNARY;
    inst->base.opcode = opcode;
    inst->unary.alu.dst = *dst;
    inst->unary.src = *src;

    return inst;
}

union vinil_inst* vinil_inst_create_binary(vinil_program_impl* prog,
                                     vinil_opcode opcode,
                                     vinil_dst_operand* dst,
                                     vinil_src_operand* src1,
                                     vinil_src_operand* src2) {
    union vinil_inst* inst = (union vinil_inst*)vinil_memory_pool_allocate(
        prog->memory, sizeof(union vinil_inst));

    if (!inst) {
        return NULL;
    }

    memset(inst, 0, sizeof(union vinil_inst));

    inst->base.kind = VINIL_INST_BINARY;
    inst->base.opcode = opcode;
    inst->binary.alu.dst = *dst;
    inst->binary.src1 = *src1;
    inst->binary.src2 = *src2;

    return inst;
}

union vinil_inst* vinil_inst_create_ternary(vinil_program_impl* prog,
                                      vinil_opcode opcode,
                                      vinil_dst_operand* dst,
                                      vinil_src_operand* src1,
                                      vinil_src_operand* src2,
                                      vinil_src_operand* src3) {
    union vinil_inst* inst = (union vinil_inst*)vinil_memory_pool_allocate(
        prog->memory, sizeof(union vinil_inst));

    if (!inst) {
        return NULL;
    }

    memset(inst, 0, sizeof(union vinil_inst));

    inst->base.kind = VINIL_INST_TERNARY;
    inst->base.opcode = opcode;
    inst->ternary.alu.dst = *dst;
    inst->ternary.src1 = *src1;
    inst->ternary.src2 = *src2;
    inst->ternary.src3 = *src3;

    return inst;
}
