/*
** ==========================================================================
**
** VINIL Interpreter
**
** Portable C fallback execution engine
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
#include <math.h>
#include <string.h>

/*
** ==========================================================================
** EXECUTION CONTEXT
** ==========================================================================
*/

#define MAX_REGISTERS 256
#define MAX_CF_STACK 32
#define MAX_CALL_STACK 32

typedef struct vec4 {
    float x, y, z, w;
} vec4;

typedef enum cf_type {
    CF_IF,
    CF_LOOP,
    CF_REP,
} cf_type;

typedef struct cf_frame {
    cf_type type;
    vinil_uint32 start_ip;
    vinil_uint32 end_ip;
    vinil_uint32 else_ip;
    vinil_int32 counter;
} cf_frame;

typedef struct interp_context {
    vec4 registers[MAX_REGISTERS];      /* Register file */
    vec4 condition_code;                /* Condition code register */

    cf_frame cf_stack[MAX_CF_STACK];    /* Control flow stack */
    vinil_uint32 cf_depth;

    vinil_uint32 call_stack[MAX_CALL_STACK]; /* Call stack (IPs) */
    vinil_uint32 call_depth;

    /* Compute extensions */
    vinil_uint32 global_id[3];          /* Global work-item ID */
    vinil_uint32 local_id[3];           /* Local work-item ID */
    vinil_uint32 group_id[3];           /* Work-group ID */
    vinil_uint32 global_size[3];        /* Global size */
    vinil_uint32 local_size[3];         /* Local size */

    void* user_data;                    /* User context */
} interp_context;

/*
** ==========================================================================
** REGISTER ACCESS HELPERS
** ==========================================================================
*/

static inline void get_src_value(interp_context* ctx,
                                  vinil_src_operand* src,
                                  vec4* result) {
    vec4 temp;
    vinil_uint32 reg_id;

    if (src->var == NULL) {
        result->x = result->y = result->z = result->w = 0.0f;
        return;
    }

    /* Get register ID from variable */
    reg_id = src->var->id % MAX_REGISTERS;
    temp = ctx->registers[reg_id];

    /* Apply swizzle */
    vinil_uint32 x_sel = src->swizzle.x;
    vinil_uint32 y_sel = src->swizzle.y;
    vinil_uint32 z_sel = src->swizzle.z;
    vinil_uint32 w_sel = src->swizzle.w;

    float* components = (float*)&temp;
    result->x = components[x_sel];
    result->y = components[y_sel];
    result->z = components[z_sel];
    result->w = components[w_sel];

    /* Apply negate */
    if (src->negate) {
        result->x = -result->x;
        result->y = -result->y;
        result->z = -result->z;
        result->w = -result->w;
    }
}

static inline void set_dst_value(interp_context* ctx,
                                  vinil_dst_operand* dst,
                                  vec4* value) {
    vinil_uint32 reg_id;
    vec4* target;

    if (dst->var == NULL) {
        return;
    }

    /* Get register ID from variable */
    reg_id = dst->var->id % MAX_REGISTERS;
    target = &ctx->registers[reg_id];

    /* Apply write mask */
    if (dst->mask.x) target->x = value->x;
    if (dst->mask.y) target->y = value->y;
    if (dst->mask.z) target->z = value->z;
    if (dst->mask.w) target->w = value->w;
}

/*
** ==========================================================================
** OPCODE EXECUTION
** ==========================================================================
*/

static vinil_error exec_abs(interp_context* ctx, union vinil_inst* inst) {
    vec4 src, result;
    get_src_value(ctx, &inst->unary.src, &src);

    result.x = fabsf(src.x);
    result.y = fabsf(src.y);
    result.z = fabsf(src.z);
    result.w = fabsf(src.w);

    set_dst_value(ctx, &inst->unary.alu.dst, &result);
    return VINIL_SUCCESS;
}

static vinil_error exec_add(interp_context* ctx, union vinil_inst* inst) {
    vec4 src1, src2, result;
    get_src_value(ctx, &inst->binary.src1, &src1);
    get_src_value(ctx, &inst->binary.src2, &src2);

    result.x = src1.x + src2.x;
    result.y = src1.y + src2.y;
    result.z = src1.z + src2.z;
    result.w = src1.w + src2.w;

    set_dst_value(ctx, &inst->binary.alu.dst, &result);
    return VINIL_SUCCESS;
}

static vinil_error exec_sub(interp_context* ctx, union vinil_inst* inst) {
    vec4 src1, src2, result;
    get_src_value(ctx, &inst->binary.src1, &src1);
    get_src_value(ctx, &inst->binary.src2, &src2);

    result.x = src1.x - src2.x;
    result.y = src1.y - src2.y;
    result.z = src1.z - src2.z;
    result.w = src1.w - src2.w;

    set_dst_value(ctx, &inst->binary.alu.dst, &result);
    return VINIL_SUCCESS;
}

static vinil_error exec_mul(interp_context* ctx, union vinil_inst* inst) {
    vec4 src1, src2, result;
    get_src_value(ctx, &inst->binary.src1, &src1);
    get_src_value(ctx, &inst->binary.src2, &src2);

    result.x = src1.x * src2.x;
    result.y = src1.y * src2.y;
    result.z = src1.z * src2.z;
    result.w = src1.w * src2.w;

    set_dst_value(ctx, &inst->binary.alu.dst, &result);
    return VINIL_SUCCESS;
}

static vinil_error exec_mad(interp_context* ctx, union vinil_inst* inst) {
    vec4 src1, src2, src3, result;
    get_src_value(ctx, &inst->ternary.src1, &src1);
    get_src_value(ctx, &inst->ternary.src2, &src2);
    get_src_value(ctx, &inst->ternary.src3, &src3);

    result.x = src1.x * src2.x + src3.x;
    result.y = src1.y * src2.y + src3.y;
    result.z = src1.z * src2.z + src3.z;
    result.w = src1.w * src2.w + src3.w;

    set_dst_value(ctx, &inst->ternary.alu.dst, &result);
    return VINIL_SUCCESS;
}

static vinil_error exec_dp3(interp_context* ctx, union vinil_inst* inst) {
    vec4 src1, src2, result;
    get_src_value(ctx, &inst->binary.src1, &src1);
    get_src_value(ctx, &inst->binary.src2, &src2);

    float dot = src1.x * src2.x + src1.y * src2.y + src1.z * src2.z;
    result.x = result.y = result.z = result.w = dot;

    set_dst_value(ctx, &inst->binary.alu.dst, &result);
    return VINIL_SUCCESS;
}

static vinil_error exec_dp4(interp_context* ctx, union vinil_inst* inst) {
    vec4 src1, src2, result;
    get_src_value(ctx, &inst->binary.src1, &src1);
    get_src_value(ctx, &inst->binary.src2, &src2);

    float dot = src1.x * src2.x + src1.y * src2.y + src1.z * src2.z + src1.w * src2.w;
    result.x = result.y = result.z = result.w = dot;

    set_dst_value(ctx, &inst->binary.alu.dst, &result);
    return VINIL_SUCCESS;
}

static vinil_error exec_mov(interp_context* ctx, union vinil_inst* inst) {
    vec4 src;
    get_src_value(ctx, &inst->unary.src, &src);
    set_dst_value(ctx, &inst->unary.alu.dst, &src);
    return VINIL_SUCCESS;
}

static vinil_error exec_sin(interp_context* ctx, union vinil_inst* inst) {
    vec4 src, result;
    get_src_value(ctx, &inst->unary.src, &src);

    result.x = sinf(src.x);
    result.y = sinf(src.y);
    result.z = sinf(src.z);
    result.w = sinf(src.w);

    set_dst_value(ctx, &inst->unary.alu.dst, &result);
    return VINIL_SUCCESS;
}

static vinil_error exec_cos(interp_context* ctx, union vinil_inst* inst) {
    vec4 src, result;
    get_src_value(ctx, &inst->unary.src, &src);

    result.x = cosf(src.x);
    result.y = cosf(src.y);
    result.z = cosf(src.z);
    result.w = cosf(src.w);

    set_dst_value(ctx, &inst->unary.alu.dst, &result);
    return VINIL_SUCCESS;
}

/*
** ==========================================================================
** INTERPRETER DISPATCH
** ==========================================================================
*/

vinil_error vinil_interpret(vinil_program_impl* prog, void* user_data) {
    if (!prog) {
        return VINIL_ERROR_INVALID_ARGUMENT;
    }

    /* Initialize context */
    interp_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.user_data = user_data;

    /* Walk through blocks and execute instructions */
    for (vinil_block* block = prog->blocks; block; block = block->next) {
        for (vinil_inst* inst = block->first; inst; inst = inst->base.next) {
            vinil_error err = VINIL_SUCCESS;

            /* Dispatch based on opcode */
            switch (inst->base.opcode) {
            case VINIL_OP_ABS: err = exec_abs(&ctx, inst); break;
            case VINIL_OP_ADD: err = exec_add(&ctx, inst); break;
            case VINIL_OP_SUB: err = exec_sub(&ctx, inst); break;
            case VINIL_OP_MUL: err = exec_mul(&ctx, inst); break;
            case VINIL_OP_MAD: err = exec_mad(&ctx, inst); break;
            case VINIL_OP_DP3: err = exec_dp3(&ctx, inst); break;
            case VINIL_OP_DP4: err = exec_dp4(&ctx, inst); break;
            case VINIL_OP_MOV: err = exec_mov(&ctx, inst); break;
            case VINIL_OP_SIN: err = exec_sin(&ctx, inst); break;
            case VINIL_OP_COS: err = exec_cos(&ctx, inst); break;

            /* Control flow (stub for now) */
            case VINIL_OP_IF:
            case VINIL_OP_ELSE:
            case VINIL_OP_ENDIF:
                /* TODO: Implement control flow */
                break;

            default:
                /* Unimplemented opcode */
                return VINIL_ERROR_NOT_IMPLEMENTED;
            }

            if (err != VINIL_SUCCESS) {
                return err;
            }
        }
    }

    return VINIL_SUCCESS;
}
