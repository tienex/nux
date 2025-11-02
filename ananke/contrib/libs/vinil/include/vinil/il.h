/*
** ==========================================================================
**
** VINIL Intermediate Language
**
** Unified IL supporting graphics and compute workloads
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

#ifndef VINIL_IL_H
#define VINIL_IL_H 1

#include <vinil/vinil.h>
#include <vinil/memory.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** ==========================================================================
** OPCODE DEFINITIONS
** ==========================================================================
**
** Opcodes are organized into categories:
** 1. Arithmetic Operations (Graphics + Compute)
** 2. Vector Operations (Graphics + Compute)
** 3. Transcendental Math (Graphics + Compute)
** 4. Comparison Operations (Graphics + Compute)
** 5. Control Flow (Graphics + Compute)
** 6. Texture Operations (Graphics only)
** 7. Memory Operations (Compute extensions)
** 8. Work-Item Builtins (Compute extensions)
** 9. Synchronization (Compute extensions)
** 10. Atomic Operations (Compute extensions)
**
** Each opcode has JIT and Interpreter implementations that must be
** kept in sync.
**
** ==========================================================================
*/

typedef enum vinil_opcode {
    /* =====================================================================
     * ARITHMETIC OPERATIONS (Graphics + Compute)
     * ===================================================================== */

    VINIL_OP_ABS,               /* v = abs(v)                               */
    VINIL_OP_ADD,               /* v = v + v                                */
    VINIL_OP_SUB,               /* v = v - v                                */
    VINIL_OP_MUL,               /* v = v * v                                */
    VINIL_OP_DIV,               /* v = v / v (compute extension)            */
    VINIL_OP_MAD,               /* v = v * v + v (multiply-add)             */
    VINIL_OP_MIN,               /* v = min(v, v)                            */
    VINIL_OP_MAX,               /* v = max(v, v)                            */
    VINIL_OP_NEG,               /* v = -v                                   */
    VINIL_OP_FRC,               /* v = frac(v) (fractional part)            */
    VINIL_OP_FLR,               /* v = floor(v)                             */
    VINIL_OP_MOD,               /* v = v % v (compute extension)            */

    /* =====================================================================
     * VECTOR OPERATIONS (Graphics + Compute)
     * ===================================================================== */

    VINIL_OP_DP2,               /* s = dot(v2, v2) (2-component)            */
    VINIL_OP_DP3,               /* s = dot(v3, v3) (3-component)            */
    VINIL_OP_DP4,               /* s = dot(v4, v4) (4-component)            */
    VINIL_OP_DPH,               /* s = dot(v4.xyz, v4.xyz) + v4.w           */
    VINIL_OP_XPD,               /* v3 = cross(v3, v3)                       */
    VINIL_OP_LRP,               /* v = lerp(v, v, v) = v1*(1-v3) + v2*v3    */
    VINIL_OP_DST,               /* v = distance vector                      */
    VINIL_OP_MOV,               /* v = v (move/copy)                        */
    VINIL_OP_SWZ,               /* v = swizzle(v, mask)                     */

    /* =====================================================================
     * TRANSCENDENTAL MATH (Graphics + Compute)
     * ===================================================================== */

    VINIL_OP_SIN,               /* s = sin(s)                               */
    VINIL_OP_COS,               /* s = cos(s)                               */
    VINIL_OP_TAN,               /* s = tan(s) (compute extension)           */
    VINIL_OP_ASIN,              /* s = asin(s) (compute extension)          */
    VINIL_OP_ACOS,              /* s = acos(s) (compute extension)          */
    VINIL_OP_ATAN,              /* s = atan(s) (compute extension)          */
    VINIL_OP_ATAN2,             /* s = atan2(s, s) (compute extension)      */
    VINIL_OP_SCS,               /* v2 = (sin(s), cos(s))                    */
    VINIL_OP_EXP,               /* s = exp(s) (base e)                      */
    VINIL_OP_EX2,               /* s = exp2(s) (base 2)                     */
    VINIL_OP_LOG,               /* s = log(s) (base e)                      */
    VINIL_OP_LG2,               /* s = log2(s) (base 2)                     */
    VINIL_OP_POW,               /* s = pow(s, s)                            */
    VINIL_OP_RCP,               /* s = 1/s (reciprocal)                     */
    VINIL_OP_RSQ,               /* s = 1/sqrt(s) (reciprocal square root)   */
    VINIL_OP_SQRT,              /* s = sqrt(s) (compute extension)          */

    /* =====================================================================
     * COMPARISON OPERATIONS (Graphics + Compute)
     * ===================================================================== */

    VINIL_OP_SEQ,               /* v = (v == v) ? 1.0 : 0.0                 */
    VINIL_OP_SNE,               /* v = (v != v) ? 1.0 : 0.0                 */
    VINIL_OP_SLT,               /* v = (v < v) ? 1.0 : 0.0                  */
    VINIL_OP_SLE,               /* v = (v <= v) ? 1.0 : 0.0                 */
    VINIL_OP_SGT,               /* v = (v > v) ? 1.0 : 0.0                  */
    VINIL_OP_SGE,               /* v = (v >= v) ? 1.0 : 0.0                 */
    VINIL_OP_CMP,               /* v = (v0 < 0) ? v1 : v2                   */
    VINIL_OP_SELECT,            /* v = (cond) ? v1 : v2 (compute extension) */
    VINIL_OP_SSG,               /* v = sign(v) = -1, 0, or 1                */

    /* =====================================================================
     * CONTROL FLOW (Graphics + Compute)
     * ===================================================================== */

    VINIL_OP_IF,                /* if (cond) { ... }                        */
    VINIL_OP_ELSE,              /* } else { ...                             */
    VINIL_OP_ENDIF,             /* }                                        */
    VINIL_OP_LOOP,              /* loop { ... }                             */
    VINIL_OP_ENDLOOP,           /* }                                        */
    VINIL_OP_REP,               /* repeat(count) { ... }                    */
    VINIL_OP_ENDREP,            /* }                                        */
    VINIL_OP_BRK,               /* break (exit loop)                        */
    VINIL_OP_CONT,              /* continue (compute extension)             */
    VINIL_OP_BRA,               /* branch to label (conditional)            */
    VINIL_OP_CAL,               /* call subroutine                          */
    VINIL_OP_RET,               /* return from subroutine                   */
    VINIL_OP_SCC,               /* set condition code register              */
    VINIL_OP_KIL,               /* kill fragment (graphics only)            */

    /* =====================================================================
     * TEXTURE OPERATIONS (Graphics only)
     * ===================================================================== */

    VINIL_OP_TEX,               /* v = texture(sampler, coords)             */
    VINIL_OP_TXB,               /* v = texture(sampler, coords, bias)       */
    VINIL_OP_TXL,               /* v = texture(sampler, coords, lod)        */
    VINIL_OP_TXP,               /* v = texture(sampler, coords/coords.w)    */

    /* =====================================================================
     * MEMORY OPERATIONS (Compute extensions)
     * ===================================================================== */

    VINIL_OP_LOAD_GLOBAL,       /* v = load(__global ptr)                   */
    VINIL_OP_STORE_GLOBAL,      /* store(__global ptr, v)                   */
    VINIL_OP_LOAD_LOCAL,        /* v = load(__local ptr)                    */
    VINIL_OP_STORE_LOCAL,       /* store(__local ptr, v)                    */
    VINIL_OP_LOAD_PRIVATE,      /* v = load(__private ptr)                  */
    VINIL_OP_STORE_PRIVATE,     /* store(__private ptr, v)                  */
    VINIL_OP_LOAD_CONSTANT,     /* v = load(__constant ptr)                 */
    VINIL_OP_VLOAD,             /* v = vload(offset, ptr) (vector load)     */
    VINIL_OP_VSTORE,            /* vstore(v, offset, ptr) (vector store)    */

    /* =====================================================================
     * WORK-ITEM BUILTINS (Compute extensions)
     * ===================================================================== */

    VINIL_OP_GET_GLOBAL_ID,     /* id = get_global_id(dim)                  */
    VINIL_OP_GET_LOCAL_ID,      /* id = get_local_id(dim)                   */
    VINIL_OP_GET_GROUP_ID,      /* id = get_group_id(dim)                   */
    VINIL_OP_GET_GLOBAL_SIZE,   /* size = get_global_size(dim)              */
    VINIL_OP_GET_LOCAL_SIZE,    /* size = get_local_size(dim)               */
    VINIL_OP_GET_NUM_GROUPS,    /* count = get_num_groups(dim)              */
    VINIL_OP_GET_WORK_DIM,      /* dim = get_work_dim()                     */
    VINIL_OP_GET_GLOBAL_OFFSET, /* offset = get_global_offset(dim)          */

    /* =====================================================================
     * SYNCHRONIZATION (Compute extensions)
     * ===================================================================== */

    VINIL_OP_BARRIER,           /* barrier(flags) - work-group barrier      */
    VINIL_OP_MEM_FENCE,         /* mem_fence(flags) - memory fence          */
    VINIL_OP_READ_MEM_FENCE,    /* read_mem_fence(flags)                    */
    VINIL_OP_WRITE_MEM_FENCE,   /* write_mem_fence(flags)                   */

    /* =====================================================================
     * ATOMIC OPERATIONS (Compute extensions)
     * ===================================================================== */

    VINIL_OP_ATOMIC_ADD,        /* old = atomic_add(ptr, val)               */
    VINIL_OP_ATOMIC_SUB,        /* old = atomic_sub(ptr, val)               */
    VINIL_OP_ATOMIC_XCHG,       /* old = atomic_xchg(ptr, val)              */
    VINIL_OP_ATOMIC_CMPXCHG,    /* old = atomic_cmpxchg(ptr, cmp, val)      */
    VINIL_OP_ATOMIC_INC,        /* old = atomic_inc(ptr)                    */
    VINIL_OP_ATOMIC_DEC,        /* old = atomic_dec(ptr)                    */
    VINIL_OP_ATOMIC_MIN,        /* old = atomic_min(ptr, val)               */
    VINIL_OP_ATOMIC_MAX,        /* old = atomic_max(ptr, val)               */
    VINIL_OP_ATOMIC_AND,        /* old = atomic_and(ptr, val)               */
    VINIL_OP_ATOMIC_OR,         /* old = atomic_or(ptr, val)                */
    VINIL_OP_ATOMIC_XOR,        /* old = atomic_xor(ptr, val)               */

    /* =====================================================================
     * ADDRESS REGISTER (Graphics)
     * ===================================================================== */

    VINIL_OP_ARL,               /* addr_reg = address_load(v)               */

    /* =====================================================================
     * PSEUDO-INSTRUCTIONS (Assembly/Declaration)
     * ===================================================================== */

    VINIL_OP_INPUT,             /* input variable declaration               */
    VINIL_OP_OUTPUT,            /* output variable declaration              */
    VINIL_OP_PARAM,             /* parameter/uniform declaration            */
    VINIL_OP_TEMP,              /* temporary variable declaration           */
    VINIL_OP_ADDRESS,           /* address variable declaration             */

    VINIL_OP_COUNT              /* Total number of opcodes                  */
} vinil_opcode;

/*
** ==========================================================================
** INSTRUCTION CATEGORIES
**
** These flags help backends (JIT/Interpreter) handle opcodes efficiently
** ==========================================================================
*/

typedef enum vinil_opcode_flags {
    VINIL_FLAG_ARITHMETIC       = 0x0001,   /* Basic arithmetic op      */
    VINIL_FLAG_VECTOR           = 0x0002,   /* Vector operation         */
    VINIL_FLAG_TRANSCENDENTAL   = 0x0004,   /* Transcendental math      */
    VINIL_FLAG_COMPARISON       = 0x0008,   /* Comparison operation     */
    VINIL_FLAG_CONTROL_FLOW     = 0x0010,   /* Control flow             */
    VINIL_FLAG_TEXTURE          = 0x0020,   /* Texture sampling         */
    VINIL_FLAG_MEMORY           = 0x0040,   /* Memory access            */
    VINIL_FLAG_WORKITEM         = 0x0080,   /* Work-item builtin        */
    VINIL_FLAG_SYNC             = 0x0100,   /* Synchronization          */
    VINIL_FLAG_ATOMIC           = 0x0200,   /* Atomic operation         */
    VINIL_FLAG_GRAPHICS_ONLY    = 0x1000,   /* Graphics-only opcode     */
    VINIL_FLAG_COMPUTE_ONLY     = 0x2000,   /* Compute-only opcode      */
} vinil_opcode_flags;

/*
** ==========================================================================
** OPCODE METADATA
**
** Information about each opcode for validation and backend generation
** ==========================================================================
*/

typedef struct vinil_opcode_info {
    vinil_opcode        opcode;
    const char*         name;
    vinil_uint32        flags;
    vinil_uint32        num_src_operands;   /* Number of source operands */
    vinil_bool          has_dst;            /* Has destination operand */
    vinil_bool          jit_implemented;    /* JIT backend ready */
    vinil_bool          interp_implemented; /* Interpreter ready */
} vinil_opcode_info;

/**
 * Get metadata for an opcode
 *
 * @param op Opcode
 * @return Opcode metadata, or NULL if invalid
 */
const vinil_opcode_info* vinil_get_opcode_info(vinil_opcode op);

/*
** ==========================================================================
** INSTRUCTION STRUCTURES
** ==========================================================================
*/

/* Forward declarations */
typedef union vinil_inst vinil_inst;
typedef struct vinil_block vinil_block;

/* Instruction kinds (for union discrimination) */
typedef enum vinil_inst_kind {
    VINIL_INST_BASE,        /* No operands */
    VINIL_INST_UNARY,       /* One source operand */
    VINIL_INST_BINARY,      /* Two source operands */
    VINIL_INST_TERNARY,     /* Three source operands */
    VINIL_INST_BRANCH,      /* Branch instruction */
    VINIL_INST_MEMORY,      /* Memory access */
    VINIL_INST_ATOMIC,      /* Atomic operation */
    VINIL_INST_BARRIER,     /* Barrier/fence */
} vinil_inst_kind;

/* Precision hint for operations */
typedef enum vinil_precision {
    VINIL_PRECISION_LOW,
    VINIL_PRECISION_MEDIUM,
    VINIL_PRECISION_HIGH,
} vinil_precision;

/* Memory address space (for compute) */
typedef enum vinil_address_space {
    VINIL_ADDR_PRIVATE,     /* Per work-item, registers/stack */
    VINIL_ADDR_GLOBAL,      /* All work-items, main memory */
    VINIL_ADDR_LOCAL,       /* Work-group shared memory */
    VINIL_ADDR_CONSTANT,    /* Read-only global memory */
} vinil_address_space;

/* Condition codes for branching */
typedef enum vinil_cond {
    VINIL_COND_FALSE,       /* Always false */
    VINIL_COND_LT,          /* Less than */
    VINIL_COND_EQ,          /* Equal */
    VINIL_COND_LE,          /* Less or equal */
    VINIL_COND_GT,          /* Greater than */
    VINIL_COND_NE,          /* Not equal */
    VINIL_COND_GE,          /* Greater or equal */
    VINIL_COND_TRUE,        /* Always true */
} vinil_cond;

/* Swizzle mask (for vector component selection) */
typedef struct vinil_swizzle {
    vinil_uint32 x : 2;     /* X component selector (0=x, 1=y, 2=z, 3=w) */
    vinil_uint32 y : 2;     /* Y component selector */
    vinil_uint32 z : 2;     /* Z component selector */
    vinil_uint32 w : 2;     /* W component selector */
} vinil_swizzle;

/* Write mask (for destination) */
typedef struct vinil_writemask {
    vinil_bool x : 1;       /* Write X component */
    vinil_bool y : 1;       /* Write Y component */
    vinil_bool z : 1;       /* Write Z component */
    vinil_bool w : 1;       /* Write W component */
} vinil_writemask;

/*
** ==========================================================================
** INSTRUCTION STRUCTURES
**
** Full definitions are in src/il_impl.h - these are forward declarations
** for the public API
** ==========================================================================
*/

typedef struct vinil_block vinil_block;
typedef struct vinil_label vinil_label;
typedef struct vinil_variable vinil_variable;

/* Instruction union - actual definition varies by backend */
union vinil_inst {
    struct {
        union vinil_inst* prev;
        union vinil_inst* next;
        vinil_inst_kind kind;
        vinil_opcode opcode;
    } base;

    /* Other instruction variants defined in il_impl.h */
    char data[256];  /* Placeholder - actual size determined by impl */
};

#ifdef __cplusplus
}
#endif

#endif /* VINIL_IL_H */
