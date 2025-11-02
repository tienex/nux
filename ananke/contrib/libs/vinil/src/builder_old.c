/*++
    Module Name:

        builder.c

    Abstract:

        VINIL IL program builder implementation.
        Provides high-level API for constructing IL programs programmatically.

    Copyright (C) 2025 NUX Project

    SPDX-License-Identifier:    CDDL-1.0
--*/

#include <vinil/builder.h>
#include <vinil/memory.h>
#include "il_impl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* --------------------------------------------------------------- */
/*  Builder Structure                                              */
/* --------------------------------------------------------------- */

struct IVinilBuilder {
    vinil_program_impl  *Program;
    vinil_block         *CurrentBlock;
};

/* --------------------------------------------------------------- */
/*  Builder Creation/Destruction                                   */
/* --------------------------------------------------------------- */

HRESULT
VinilCreateBuilder (
    IVinilBuilder  **Builder
    )
{
    IVinilBuilder       *NewBuilder;
    vinil_program_impl  *Program;
    vinil_memory_pool   *Pool;

    if (Builder == NULL) {
        return E_POINTER;
    }

    NewBuilder = (IVinilBuilder *)malloc(sizeof(IVinilBuilder));
    if (NewBuilder == NULL) {
        return E_OUTOFMEMORY;
    }

    /* Create memory pool for program */
    Pool = vinil_memory_pool_create(4096, NULL);
    if (Pool == NULL) {
        free(NewBuilder);
        return E_OUTOFMEMORY;
    }

    /* Create program structure */
    Program = (vinil_program_impl *)malloc(sizeof(vinil_program_impl));
    if (Program == NULL) {
        vinil_memory_pool_destroy(Pool);
        free(NewBuilder);
        return E_OUTOFMEMORY;
    }

    memset(Program, 0, sizeof(vinil_program_impl));
    Program->memory = Pool;

    NewBuilder->Program = Program;
    NewBuilder->CurrentBlock = NULL;

    *Builder = NewBuilder;
    return S_OK;
}

HRESULT
VinilDestroyBuilder (
    IVinilBuilder  *Builder
    )
{
    if (Builder == NULL) {
        return E_POINTER;
    }

    if (Builder->Program != NULL) {
        if (Builder->Program->memory != NULL) {
            vinil_memory_pool_destroy(Builder->Program->memory);
        }
        free(Builder->Program);
    }

    free(Builder);
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Variable Creation                                              */
/* --------------------------------------------------------------- */

HRESULT
VinilBuilderCreateVariable (
    IVinilBuilder   *Builder,
    VINIL_VAR_TYPE  Type,
    CONST CHAR8     *Name,
    VINIL_VARIABLE  *Variable
    )
{
    vinil_variable  *Var;
    vinil_type      *VarType;

    if (Builder == NULL || Variable == NULL) {
        return E_POINTER;
    }

    /* Allocate variable from pool */
    Var = (vinil_variable *)vinil_memory_pool_allocate(
        Builder->Program->memory,
        sizeof(vinil_variable)
    );

    if (Var == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(Var, 0, sizeof(vinil_variable));

    /* Create type based on VINIL_VAR_TYPE */
    switch (Type) {
    case VinilVarFloat:
        VarType = vinil_type_get_basic(VINIL_TYPE_FLOAT, VINIL_PRECISION_HIGH);
        break;
    case VinilVarFloat2:
        VarType = vinil_type_create_vector(VINIL_TYPE_FLOAT, VINIL_PRECISION_HIGH, 2);
        break;
    case VinilVarFloat3:
        VarType = vinil_type_create_vector(VINIL_TYPE_FLOAT, VINIL_PRECISION_HIGH, 3);
        break;
    case VinilVarFloat4:
        VarType = vinil_type_create_vector(VINIL_TYPE_FLOAT, VINIL_PRECISION_HIGH, 4);
        break;
    case VinilVarInt:
        VarType = vinil_type_get_basic(VINIL_TYPE_INT, VINIL_PRECISION_HIGH);
        break;
    case VinilVarInt2:
        VarType = vinil_type_create_vector(VINIL_TYPE_INT, VINIL_PRECISION_HIGH, 2);
        break;
    case VinilVarInt3:
        VarType = vinil_type_create_vector(VINIL_TYPE_INT, VINIL_PRECISION_HIGH, 3);
        break;
    case VinilVarInt4:
        VarType = vinil_type_create_vector(VINIL_TYPE_INT, VINIL_PRECISION_HIGH, 4);
        break;
    default:
        return E_FAIL;
    }

    if (VarType == NULL) {
        return E_FAIL;
    }

    Var->kind = VINIL_VAR_TEMP;
    Var->id = Builder->Program->num_vars++;
    Var->type = VarType;
    Var->name = (const char *)Name;
    Var->name_length = Name ? strlen((const char *)Name) : 0;

    /* Add to temp list */
    Var->next = Builder->Program->temps;
    Builder->Program->temps = Var;

    *Variable = (VINIL_VARIABLE)Var;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Block Creation                                                 */
/* --------------------------------------------------------------- */

HRESULT
VinilBuilderCreateBlock (
    IVinilBuilder  *Builder,
    VINIL_BLOCK    *Block
    )
{
    vinil_block  *NewBlock;

    if (Builder == NULL || Block == NULL) {
        return E_POINTER;
    }

    NewBlock = (vinil_block *)vinil_memory_pool_allocate(
        Builder->Program->memory,
        sizeof(vinil_block)
    );

    if (NewBlock == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(NewBlock, 0, sizeof(vinil_block));
    NewBlock->id = Builder->Program->num_blocks++;

    /* Add to block list */
    NewBlock->next = Builder->Program->blocks;
    if (Builder->Program->blocks != NULL) {
        Builder->Program->blocks->prev = NewBlock;
    }
    Builder->Program->blocks = NewBlock;

    *Block = (VINIL_BLOCK)NewBlock;
    return S_OK;
}

HRESULT
VinilBuilderSetInsertBlock (
    IVinilBuilder  *Builder,
    VINIL_BLOCK    Block
    )
{
    if (Builder == NULL) {
        return E_POINTER;
    }

    Builder->CurrentBlock = (vinil_block *)Block;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Helper Functions                                               */
/* --------------------------------------------------------------- */

static vinil_dst_operand
MakeDstOperand (
    VINIL_VARIABLE  Var
    )
{
    vinil_dst_operand  Dst;

    memset(&Dst, 0, sizeof(Dst));
    Dst.var = (vinil_variable *)Var;
    Dst.mask.x = 1;
    Dst.mask.y = 1;
    Dst.mask.z = 1;
    Dst.mask.w = 1;

    return Dst;
}

static vinil_src_operand
MakeSrcOperand (
    VINIL_VARIABLE  Var
    )
{
    vinil_src_operand  Src;

    memset(&Src, 0, sizeof(Src));
    Src.var = (vinil_variable *)Var;
    Src.swizzle.x = 0;  /* .xyzw */
    Src.swizzle.y = 1;
    Src.swizzle.z = 2;
    Src.swizzle.w = 3;
    Src.negate = 0;

    return Src;
}

static VOID
AppendInstruction (
    vinil_block         *Block,
    union vinil_inst    *Inst
    )
{
    if (Block == NULL || Inst == NULL) {
        return;
    }

    Inst->base.next = NULL;
    Inst->base.prev = Block->last;

    if (Block->last != NULL) {
        Block->last->base.next = Inst;
    }

    Block->last = Inst;

    if (Block->first == NULL) {
        Block->first = Inst;
    }
}

/* --------------------------------------------------------------- */
/*  Instruction Building                                           */
/* --------------------------------------------------------------- */

HRESULT
VinilBuilderBuildAdd (
    IVinilBuilder   *Builder,
    VINIL_VARIABLE  Dst,
    VINIL_VARIABLE  Src1,
    VINIL_VARIABLE  Src2
    )
{
    vinil_inst_binary  *Inst;

    if (Builder == NULL || Builder->CurrentBlock == NULL) {
        return E_POINTER;
    }

    Inst = (vinil_inst_binary *)vinil_memory_pool_allocate(
        Builder->Program->memory,
        sizeof(vinil_inst_binary)
    );

    if (Inst == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(Inst, 0, sizeof(vinil_inst_binary));
    Inst->alu.base.kind = VINIL_INST_BINARY;
    Inst->alu.base.opcode = VINIL_OP_ADD;
    Inst->alu.dst = MakeDstOperand(Dst);
    Inst->alu.prec = VINIL_PRECISION_HIGH;
    Inst->src1 = MakeSrcOperand(Src1);
    Inst->src2 = MakeSrcOperand(Src2);

    AppendInstruction(Builder->CurrentBlock, (union vinil_inst *)Inst);
    return S_OK;
}

HRESULT
VinilBuilderBuildSub (
    IVinilBuilder   *Builder,
    VINIL_VARIABLE  Dst,
    VINIL_VARIABLE  Src1,
    VINIL_VARIABLE  Src2
    )
{
    vinil_inst_binary  *Inst;

    if (Builder == NULL || Builder->CurrentBlock == NULL) {
        return E_POINTER;
    }

    Inst = (vinil_inst_binary *)vinil_memory_pool_allocate(
        Builder->Program->memory,
        sizeof(vinil_inst_binary)
    );

    if (Inst == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(Inst, 0, sizeof(vinil_inst_binary));
    Inst->alu.base.kind = VINIL_INST_BINARY;
    Inst->alu.base.opcode = VINIL_OP_SUB;
    Inst->alu.dst = MakeDstOperand(Dst);
    Inst->alu.prec = VINIL_PRECISION_HIGH;
    Inst->src1 = MakeSrcOperand(Src1);
    Inst->src2 = MakeSrcOperand(Src2);

    AppendInstruction(Builder->CurrentBlock, (union vinil_inst *)Inst);
    return S_OK;
}

HRESULT
VinilBuilderBuildMul (
    IVinilBuilder   *Builder,
    VINIL_VARIABLE  Dst,
    VINIL_VARIABLE  Src1,
    VINIL_VARIABLE  Src2
    )
{
    vinil_inst_binary  *Inst;

    if (Builder == NULL || Builder->CurrentBlock == NULL) {
        return E_POINTER;
    }

    Inst = (vinil_inst_binary *)vinil_memory_pool_allocate(
        Builder->Program->memory,
        sizeof(vinil_inst_binary)
    );

    if (Inst == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(Inst, 0, sizeof(vinil_inst_binary));
    Inst->alu.base.kind = VINIL_INST_BINARY;
    Inst->alu.base.opcode = VINIL_OP_MUL;
    Inst->alu.dst = MakeDstOperand(Dst);
    Inst->alu.prec = VINIL_PRECISION_HIGH;
    Inst->src1 = MakeSrcOperand(Src1);
    Inst->src2 = MakeSrcOperand(Src2);

    AppendInstruction(Builder->CurrentBlock, (union vinil_inst *)Inst);
    return S_OK;
}

HRESULT
VinilBuilderBuildMad (
    IVinilBuilder   *Builder,
    VINIL_VARIABLE  Dst,
    VINIL_VARIABLE  Src1,
    VINIL_VARIABLE  Src2,
    VINIL_VARIABLE  Src3
    )
{
    vinil_inst_ternary  *Inst;

    if (Builder == NULL || Builder->CurrentBlock == NULL) {
        return E_POINTER;
    }

    Inst = (vinil_inst_ternary *)vinil_memory_pool_allocate(
        Builder->Program->memory,
        sizeof(vinil_inst_ternary)
    );

    if (Inst == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(Inst, 0, sizeof(vinil_inst_ternary));
    Inst->alu.base.kind = VINIL_INST_TERNARY;
    Inst->alu.base.opcode = VINIL_OP_MAD;
    Inst->alu.dst = MakeDstOperand(Dst);
    Inst->alu.prec = VINIL_PRECISION_HIGH;
    Inst->src1 = MakeSrcOperand(Src1);
    Inst->src2 = MakeSrcOperand(Src2);
    Inst->src3 = MakeSrcOperand(Src3);

    AppendInstruction(Builder->CurrentBlock, (union vinil_inst *)Inst);
    return S_OK;
}

HRESULT
VinilBuilderBuildMov (
    IVinilBuilder   *Builder,
    VINIL_VARIABLE  Dst,
    VINIL_VARIABLE  Src
    )
{
    vinil_inst_unary  *Inst;

    if (Builder == NULL || Builder->CurrentBlock == NULL) {
        return E_POINTER;
    }

    Inst = (vinil_inst_unary *)vinil_memory_pool_allocate(
        Builder->Program->memory,
        sizeof(vinil_inst_unary)
    );

    if (Inst == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(Inst, 0, sizeof(vinil_inst_unary));
    Inst->alu.base.kind = VINIL_INST_UNARY;
    Inst->alu.base.opcode = VINIL_OP_MOV;
    Inst->alu.dst = MakeDstOperand(Dst);
    Inst->alu.prec = VINIL_PRECISION_HIGH;
    Inst->src = MakeSrcOperand(Src);

    AppendInstruction(Builder->CurrentBlock, (union vinil_inst *)Inst);
    return S_OK;
}

HRESULT
VinilBuilderBuildDp3 (
    IVinilBuilder   *Builder,
    VINIL_VARIABLE  Dst,
    VINIL_VARIABLE  Src1,
    VINIL_VARIABLE  Src2
    )
{
    vinil_inst_binary  *Inst;

    if (Builder == NULL || Builder->CurrentBlock == NULL) {
        return E_POINTER;
    }

    Inst = (vinil_inst_binary *)vinil_memory_pool_allocate(
        Builder->Program->memory,
        sizeof(vinil_inst_binary)
    );

    if (Inst == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(Inst, 0, sizeof(vinil_inst_binary));
    Inst->alu.base.kind = VINIL_INST_BINARY;
    Inst->alu.base.opcode = VINIL_OP_DP3;
    Inst->alu.dst = MakeDstOperand(Dst);
    Inst->alu.prec = VINIL_PRECISION_HIGH;
    Inst->src1 = MakeSrcOperand(Src1);
    Inst->src2 = MakeSrcOperand(Src2);

    AppendInstruction(Builder->CurrentBlock, (union vinil_inst *)Inst);
    return S_OK;
}

HRESULT
VinilBuilderBuildDp4 (
    IVinilBuilder   *Builder,
    VINIL_VARIABLE  Dst,
    VINIL_VARIABLE  Src1,
    VINIL_VARIABLE  Src2
    )
{
    vinil_inst_binary  *Inst;

    if (Builder == NULL || Builder->CurrentBlock == NULL) {
        return E_POINTER;
    }

    Inst = (vinil_inst_binary *)vinil_memory_pool_allocate(
        Builder->Program->memory,
        sizeof(vinil_inst_binary)
    );

    if (Inst == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(Inst, 0, sizeof(vinil_inst_binary));
    Inst->alu.base.kind = VINIL_INST_BINARY;
    Inst->alu.base.opcode = VINIL_OP_DP4;
    Inst->alu.dst = MakeDstOperand(Dst);
    Inst->alu.prec = VINIL_PRECISION_HIGH;
    Inst->src1 = MakeSrcOperand(Src1);
    Inst->src2 = MakeSrcOperand(Src2);

    AppendInstruction(Builder->CurrentBlock, (union vinil_inst *)Inst);
    return S_OK;
}

HRESULT
VinilBuilderBuildRet (
    IVinilBuilder  *Builder
    )
{
    vinil_inst_base  *Inst;

    if (Builder == NULL || Builder->CurrentBlock == NULL) {
        return E_POINTER;
    }

    Inst = (vinil_inst_base *)vinil_memory_pool_allocate(
        Builder->Program->memory,
        sizeof(vinil_inst_base)
    );

    if (Inst == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(Inst, 0, sizeof(vinil_inst_base));
    Inst->kind = VINIL_INST_BASE;
    Inst->opcode = VINIL_OP_RET;

    AppendInstruction(Builder->CurrentBlock, (union vinil_inst *)Inst);
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Finalization                                                   */
/* --------------------------------------------------------------- */

HRESULT
VinilBuilderFinalize (
    IVinilBuilder  *Builder,
    VOID           **Program
    )
{
    if (Builder == NULL || Program == NULL) {
        return E_POINTER;
    }

    *Program = (VOID *)Builder->Program;
    return S_OK;
}
