/*++
    Module Name:

        disasm.c

    Abstract:

        VINIL IL disassembler implementation with complete opcode table.

    Copyright (C) 2025 NUX Project

    SPDX-License-Identifier:    CDDL-1.0
--*/

#include <vinil/disasm.h>
#include <stdio.h>
#include <string.h>

/* --------------------------------------------------------------- */
/*  Opcode Definitions                                             */
/* --------------------------------------------------------------- */

#define VINIL_OP_ABS                0
#define VINIL_OP_ADD                1
#define VINIL_OP_SUB                2
#define VINIL_OP_MUL                3
#define VINIL_OP_DIV                4
#define VINIL_OP_MAD                5
#define VINIL_OP_MIN                6
#define VINIL_OP_MAX                7
#define VINIL_OP_NEG                8
#define VINIL_OP_FRC                9
#define VINIL_OP_FLR                10
#define VINIL_OP_MOD                11
#define VINIL_OP_DP2                12
#define VINIL_OP_DP3                13
#define VINIL_OP_DP4                14
#define VINIL_OP_DPH                15
#define VINIL_OP_XPD                16
#define VINIL_OP_LRP                17
#define VINIL_OP_DST                18
#define VINIL_OP_NRM                19
#define VINIL_OP_SIN                20
#define VINIL_OP_COS                21
#define VINIL_OP_TAN                22
#define VINIL_OP_ASIN               23
#define VINIL_OP_ACOS               24
#define VINIL_OP_ATAN               25
#define VINIL_OP_EXP                26
#define VINIL_OP_EXP2               27
#define VINIL_OP_LOG                28
#define VINIL_OP_LOG2               29
#define VINIL_OP_POW                30
#define VINIL_OP_SQRT               31
#define VINIL_OP_RSQRT              32
#define VINIL_OP_SEQ                33
#define VINIL_OP_SLT                34
#define VINIL_OP_SGE                35
#define VINIL_OP_SNE                36
#define VINIL_OP_IF                 37
#define VINIL_OP_ELSE               38
#define VINIL_OP_ENDIF              39
#define VINIL_OP_LOOP               40
#define VINIL_OP_ENDLOOP            41
#define VINIL_OP_REP                42
#define VINIL_OP_ENDREP             43
#define VINIL_OP_BRK                44
#define VINIL_OP_CONT               45
#define VINIL_OP_CAL                46
#define VINIL_OP_RET                47
#define VINIL_OP_BRA                48
#define VINIL_OP_TEX                49
#define VINIL_OP_TXB                50
#define VINIL_OP_TXL                51
#define VINIL_OP_TXP                52
#define VINIL_OP_LOAD_GLOBAL        53
#define VINIL_OP_STORE_GLOBAL       54
#define VINIL_OP_LOAD_LOCAL         55
#define VINIL_OP_STORE_LOCAL        56
#define VINIL_OP_LOAD_PRIVATE       57
#define VINIL_OP_STORE_PRIVATE      58
#define VINIL_OP_GET_GLOBAL_ID      59
#define VINIL_OP_GET_LOCAL_ID       60
#define VINIL_OP_GET_GROUP_ID       61
#define VINIL_OP_GET_GLOBAL_SIZE    62
#define VINIL_OP_GET_LOCAL_SIZE     63
#define VINIL_OP_GET_NUM_GROUPS     64
#define VINIL_OP_BARRIER            65
#define VINIL_OP_MEM_FENCE          66
#define VINIL_OP_ATOMIC_ADD         67
#define VINIL_OP_ATOMIC_SUB         68
#define VINIL_OP_ATOMIC_XCHG        69
#define VINIL_OP_ATOMIC_INC         70
#define VINIL_OP_ATOMIC_DEC         71
#define VINIL_OP_ATOMIC_CMPXCHG     72
#define VINIL_OP_ATOMIC_MIN         73
#define VINIL_OP_ATOMIC_MAX         74
#define VINIL_OP_ATOMIC_AND         75
#define VINIL_OP_ATOMIC_OR          76
#define VINIL_OP_ATOMIC_XOR         77
#define VINIL_OP_MOV                78
#define VINIL_OP_NOP                79

#define VINIL_MAX_OPCODE            80

/* --------------------------------------------------------------- */
/*  Opcode Metadata Table                                          */
/* --------------------------------------------------------------- */

static CONST VINIL_OPCODE_INFO gOpcodeTable[VINIL_MAX_OPCODE] = {
    /* Arithmetic */
    { VINIL_OP_ABS,     "ABS",     "Absolute value",               VinilOpcatArithmetic,     1, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_ADD,     "ADD",     "Add",                          VinilOpcatArithmetic,     2, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_SUB,     "SUB",     "Subtract",                     VinilOpcatArithmetic,     2, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_MUL,     "MUL",     "Multiply",                     VinilOpcatArithmetic,     2, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_DIV,     "DIV",     "Divide",                       VinilOpcatArithmetic,     2, TRUE,  FALSE, TRUE  },
    { VINIL_OP_MAD,     "MAD",     "Multiply-add",                 VinilOpcatArithmetic,     3, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_MIN,     "MIN",     "Minimum",                      VinilOpcatArithmetic,     2, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_MAX,     "MAX",     "Maximum",                      VinilOpcatArithmetic,     2, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_NEG,     "NEG",     "Negate",                       VinilOpcatArithmetic,     1, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_FRC,     "FRC",     "Fractional part",              VinilOpcatArithmetic,     1, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_FLR,     "FLR",     "Floor",                        VinilOpcatArithmetic,     1, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_MOD,     "MOD",     "Modulo",                       VinilOpcatArithmetic,     2, TRUE,  FALSE, TRUE  },

    /* Vector */
    { VINIL_OP_DP2,     "DP2",     "Dot product 2D",               VinilOpcatVector,         2, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_DP3,     "DP3",     "Dot product 3D",               VinilOpcatVector,         2, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_DP4,     "DP4",     "Dot product 4D",               VinilOpcatVector,         2, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_DPH,     "DPH",     "Homogeneous dot product",      VinilOpcatVector,         2, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_XPD,     "XPD",     "Cross product",                VinilOpcatVector,         2, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_LRP,     "LRP",     "Linear interpolate",           VinilOpcatVector,         3, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_DST,     "DST",     "Distance vector",              VinilOpcatVector,         2, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_NRM,     "NRM",     "Normalize",                    VinilOpcatVector,         1, TRUE,  TRUE,  TRUE  },

    /* Transcendental */
    { VINIL_OP_SIN,     "SIN",     "Sine",                         VinilOpcatTranscendental, 1, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_COS,     "COS",     "Cosine",                       VinilOpcatTranscendental, 1, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_TAN,     "TAN",     "Tangent",                      VinilOpcatTranscendental, 1, TRUE,  FALSE, TRUE  },
    { VINIL_OP_ASIN,    "ASIN",    "Arc sine",                     VinilOpcatTranscendental, 1, TRUE,  FALSE, TRUE  },
    { VINIL_OP_ACOS,    "ACOS",    "Arc cosine",                   VinilOpcatTranscendental, 1, TRUE,  FALSE, TRUE  },
    { VINIL_OP_ATAN,    "ATAN",    "Arc tangent",                  VinilOpcatTranscendental, 1, TRUE,  FALSE, TRUE  },
    { VINIL_OP_EXP,     "EXP",     "Exponential (base e)",         VinilOpcatTranscendental, 1, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_EXP2,    "EXP2",    "Exponential (base 2)",         VinilOpcatTranscendental, 1, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_LOG,     "LOG",     "Logarithm (base e)",           VinilOpcatTranscendental, 1, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_LOG2,    "LOG2",    "Logarithm (base 2)",           VinilOpcatTranscendental, 1, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_POW,     "POW",     "Power",                        VinilOpcatTranscendental, 2, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_SQRT,    "SQRT",    "Square root",                  VinilOpcatTranscendental, 1, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_RSQRT,   "RSQRT",   "Reciprocal square root",       VinilOpcatTranscendental, 1, TRUE,  TRUE,  TRUE  },

    /* Comparison */
    { VINIL_OP_SEQ,     "SEQ",     "Set if equal",                 VinilOpcatComparison,     2, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_SLT,     "SLT",     "Set if less than",             VinilOpcatComparison,     2, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_SGE,     "SGE",     "Set if greater or equal",      VinilOpcatComparison,     2, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_SNE,     "SNE",     "Set if not equal",             VinilOpcatComparison,     2, TRUE,  TRUE,  TRUE  },

    /* Control Flow */
    { VINIL_OP_IF,      "IF",      "Begin conditional",            VinilOpcatControlFlow,    1, FALSE, TRUE,  TRUE  },
    { VINIL_OP_ELSE,    "ELSE",    "Else clause",                  VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    { VINIL_OP_ENDIF,   "ENDIF",   "End conditional",              VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    { VINIL_OP_LOOP,    "LOOP",    "Begin loop",                   VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    { VINIL_OP_ENDLOOP, "ENDLOOP", "End loop",                     VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    { VINIL_OP_REP,     "REP",     "Begin repeat",                 VinilOpcatControlFlow,    1, FALSE, TRUE,  TRUE  },
    { VINIL_OP_ENDREP,  "ENDREP",  "End repeat",                   VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    { VINIL_OP_BRK,     "BRK",     "Break loop",                   VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    { VINIL_OP_CONT,    "CONT",    "Continue loop",                VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    { VINIL_OP_CAL,     "CAL",     "Call subroutine",              VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    { VINIL_OP_RET,     "RET",     "Return from subroutine",       VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    { VINIL_OP_BRA,     "BRA",     "Branch",                       VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },

    /* Texture */
    { VINIL_OP_TEX,     "TEX",     "Texture lookup",               VinilOpcatTexture,        2, TRUE,  TRUE,  FALSE },
    { VINIL_OP_TXB,     "TXB",     "Texture lookup with bias",     VinilOpcatTexture,        2, TRUE,  TRUE,  FALSE },
    { VINIL_OP_TXL,     "TXL",     "Texture lookup with LOD",      VinilOpcatTexture,        2, TRUE,  TRUE,  FALSE },
    { VINIL_OP_TXP,     "TXP",     "Texture lookup with projection", VinilOpcatTexture,      2, TRUE,  TRUE,  FALSE },

    /* Memory */
    { VINIL_OP_LOAD_GLOBAL,  "LD.GLOBAL",  "Load from global memory",  VinilOpcatMemory, 1, TRUE,  FALSE, TRUE  },
    { VINIL_OP_STORE_GLOBAL, "ST.GLOBAL",  "Store to global memory",   VinilOpcatMemory, 2, FALSE, FALSE, TRUE  },
    { VINIL_OP_LOAD_LOCAL,   "LD.LOCAL",   "Load from local memory",   VinilOpcatMemory, 1, TRUE,  FALSE, TRUE  },
    { VINIL_OP_STORE_LOCAL,  "ST.LOCAL",   "Store to local memory",    VinilOpcatMemory, 2, FALSE, FALSE, TRUE  },
    { VINIL_OP_LOAD_PRIVATE,  "LD.PRIVATE", "Load from private memory", VinilOpcatMemory, 1, TRUE,  FALSE, TRUE  },
    { VINIL_OP_STORE_PRIVATE, "ST.PRIVATE", "Store to private memory",  VinilOpcatMemory, 2, FALSE, FALSE, TRUE  },

    /* Work-Item */
    { VINIL_OP_GET_GLOBAL_ID,   "GET_GLOBAL_ID",   "Get global work-item ID",      VinilOpcatWorkItem, 1, TRUE,  FALSE, TRUE  },
    { VINIL_OP_GET_LOCAL_ID,    "GET_LOCAL_ID",    "Get local work-item ID",       VinilOpcatWorkItem, 1, TRUE,  FALSE, TRUE  },
    { VINIL_OP_GET_GROUP_ID,    "GET_GROUP_ID",    "Get work-group ID",            VinilOpcatWorkItem, 1, TRUE,  FALSE, TRUE  },
    { VINIL_OP_GET_GLOBAL_SIZE, "GET_GLOBAL_SIZE", "Get global work size",         VinilOpcatWorkItem, 1, TRUE,  FALSE, TRUE  },
    { VINIL_OP_GET_LOCAL_SIZE,  "GET_LOCAL_SIZE",  "Get local work size",          VinilOpcatWorkItem, 1, TRUE,  FALSE, TRUE  },
    { VINIL_OP_GET_NUM_GROUPS,  "GET_NUM_GROUPS",  "Get number of work-groups",    VinilOpcatWorkItem, 1, TRUE,  FALSE, TRUE  },

    /* Synchronization */
    { VINIL_OP_BARRIER,     "BARRIER",   "Work-group barrier",           VinilOpcatSync, 0, FALSE, FALSE, TRUE  },
    { VINIL_OP_MEM_FENCE,   "MEM_FENCE", "Memory fence",                 VinilOpcatSync, 1, FALSE, FALSE, TRUE  },

    /* Atomics */
    { VINIL_OP_ATOMIC_ADD,     "ATOMIC.ADD",     "Atomic add",              VinilOpcatAtomic, 2, TRUE,  FALSE, TRUE  },
    { VINIL_OP_ATOMIC_SUB,     "ATOMIC.SUB",     "Atomic subtract",         VinilOpcatAtomic, 2, TRUE,  FALSE, TRUE  },
    { VINIL_OP_ATOMIC_XCHG,    "ATOMIC.XCHG",    "Atomic exchange",         VinilOpcatAtomic, 2, TRUE,  FALSE, TRUE  },
    { VINIL_OP_ATOMIC_INC,     "ATOMIC.INC",     "Atomic increment",        VinilOpcatAtomic, 1, TRUE,  FALSE, TRUE  },
    { VINIL_OP_ATOMIC_DEC,     "ATOMIC.DEC",     "Atomic decrement",        VinilOpcatAtomic, 1, TRUE,  FALSE, TRUE  },
    { VINIL_OP_ATOMIC_CMPXCHG, "ATOMIC.CMPXCHG", "Atomic compare-exchange", VinilOpcatAtomic, 3, TRUE,  FALSE, TRUE  },
    { VINIL_OP_ATOMIC_MIN,     "ATOMIC.MIN",     "Atomic minimum",          VinilOpcatAtomic, 2, TRUE,  FALSE, TRUE  },
    { VINIL_OP_ATOMIC_MAX,     "ATOMIC.MAX",     "Atomic maximum",          VinilOpcatAtomic, 2, TRUE,  FALSE, TRUE  },
    { VINIL_OP_ATOMIC_AND,     "ATOMIC.AND",     "Atomic AND",              VinilOpcatAtomic, 2, TRUE,  FALSE, TRUE  },
    { VINIL_OP_ATOMIC_OR,      "ATOMIC.OR",      "Atomic OR",               VinilOpcatAtomic, 2, TRUE,  FALSE, TRUE  },
    { VINIL_OP_ATOMIC_XOR,     "ATOMIC.XOR",     "Atomic XOR",              VinilOpcatAtomic, 2, TRUE,  FALSE, TRUE  },

    /* Misc */
    { VINIL_OP_MOV,     "MOV",     "Move",                         VinilOpcatArithmetic,     1, TRUE,  TRUE,  TRUE  },
    { VINIL_OP_NOP,     "NOP",     "No operation",                 VinilOpcatArithmetic,     0, FALSE, TRUE,  TRUE  },
};

/* --------------------------------------------------------------- */
/*  Public API Implementation                                      */
/* --------------------------------------------------------------- */

CONST VINIL_OPCODE_INFO *
VinilGetOpcodeInfo (
    UINT32  Opcode
    )
{
    if (Opcode >= VINIL_MAX_OPCODE) {
        return NULL;
    }

    return &gOpcodeTable[Opcode];
}

CONST CHAR8 *
VinilGetOpcodeName (
    UINT32  Opcode
    )
{
    CONST VINIL_OPCODE_INFO *Info;

    Info = VinilGetOpcodeInfo(Opcode);
    if (Info == NULL) {
        return "UNKNOWN";
    }

    return Info->Name;
}

BOOLEAN
VinilIsValidOpcode (
    UINT32  Opcode
    )
{
    return (Opcode < VINIL_MAX_OPCODE);
}

/* --------------------------------------------------------------- */
/*  Helper Functions                                               */
/* --------------------------------------------------------------- */

static VOID
FormatSwizzle (
    CHAR8   *Buffer,
    UINTN   BufferSize,
    UINT32  Swizzle
    )
{
    CONST CHAR8 Components[] = "xyzw";
    UINT32      X, Y, Z, W;

    X = (Swizzle >> 0) & 3;
    Y = (Swizzle >> 2) & 3;
    Z = (Swizzle >> 4) & 3;
    W = (Swizzle >> 6) & 3;

    /* Check if this is the identity swizzle .xyzw */
    if (X == 0 && Y == 1 && Z == 2 && W == 3) {
        Buffer[0] = '\0';
        return;
    }

    snprintf((char *)Buffer, BufferSize, ".%c%c%c%c",
        Components[X], Components[Y], Components[Z], Components[W]);
}

static VOID
FormatWriteMask (
    CHAR8   *Buffer,
    UINTN   BufferSize,
    UINT32  Mask
    )
{
    CHAR8   Temp[8];
    UINTN   Pos;

    /* Check if all components enabled (no mask needed) */
    if ((Mask & 0xF) == 0xF) {
        Buffer[0] = '\0';
        return;
    }

    Pos = 0;
    Temp[Pos++] = '.';

    if (Mask & 0x1) Temp[Pos++] = 'x';
    if (Mask & 0x2) Temp[Pos++] = 'y';
    if (Mask & 0x4) Temp[Pos++] = 'z';
    if (Mask & 0x8) Temp[Pos++] = 'w';

    Temp[Pos] = '\0';
    snprintf((char *)Buffer, BufferSize, "%s", Temp);
}

static VOID
FormatVariable (
    CHAR8   *Buffer,
    UINTN   BufferSize,
    CONST VOID *Var
    )
{
    /* For now, just format as generic register */
    /* TODO: Extract actual variable info when structure is available */
    snprintf((char *)Buffer, BufferSize, "r%p", Var);
}

static VOID
FormatSourceOperand (
    CHAR8   *Buffer,
    UINTN   BufferSize,
    CONST VOID *Operand
    )
{
    /* TODO: Extract actual source operand fields */
    /* For now, simplified formatting */
    snprintf((char *)Buffer, BufferSize, "src");
}

static VOID
FormatDestOperand (
    CHAR8   *Buffer,
    UINTN   BufferSize,
    CONST VOID *Operand
    )
{
    /* TODO: Extract actual dest operand fields */
    /* For now, simplified formatting */
    snprintf((char *)Buffer, BufferSize, "dst");
}

HRESULT
VinilDisasmInstruction (
    CONST VOID          *Instruction,
    VINIL_DISASM_FLAGS  Flags,
    CHAR8               *Buffer,
    UINTN               BufferSize
    )
{
    CONST UINT32            *InstData;
    UINT32                  Opcode;
    CONST VINIL_OPCODE_INFO *Info;
    CHAR8                   Temp[256];
    UINTN                   Pos;

    if (Instruction == NULL || Buffer == NULL) {
        return E_POINTER;
    }

    /* Extract opcode from instruction */
    InstData = (CONST UINT32 *)Instruction;
    Opcode = InstData[0] & 0xFF;  /* Assume opcode is in lowest byte */

    Info = VinilGetOpcodeInfo(Opcode);
    if (Info == NULL) {
        snprintf((char *)Buffer, BufferSize, "INVALID_OPCODE 0x%02X", Opcode);
        return E_FAIL;
    }

    Pos = 0;

    /* Show address if requested */
    if (Flags & VinilDisasmShowAddresses) {
        Pos += snprintf((char *)&Buffer[Pos], BufferSize - Pos,
            "%p: ", Instruction);
    }

    /* Format opcode name */
    Pos += snprintf((char *)&Buffer[Pos], BufferSize - Pos,
        "%-12s", Info->Name);

    /* Format operands based on instruction type */
    if (Info->HasDestination) {
        FormatDestOperand(Temp, sizeof(Temp), Instruction);
        Pos += snprintf((char *)&Buffer[Pos], BufferSize - Pos, "%s", Temp);

        if (Info->NumSources > 0) {
            Pos += snprintf((char *)&Buffer[Pos], BufferSize - Pos, ", ");
        }
    }

    /* Format source operands */
    for (UINT32 i = 0; i < Info->NumSources; i++) {
        if (i > 0) {
            Pos += snprintf((char *)&Buffer[Pos], BufferSize - Pos, ", ");
        }

        FormatSourceOperand(Temp, sizeof(Temp), Instruction);
        Pos += snprintf((char *)&Buffer[Pos], BufferSize - Pos, "%s", Temp);
    }

    /* Add verbose information if requested */
    if (Flags & VinilDisasmVerbose) {
        Pos += snprintf((char *)&Buffer[Pos], BufferSize - Pos,
            "  ; %s", Info->Description);
    }

    return S_OK;
}

HRESULT
VinilDisasmProgram (
    CONST VOID          *Program,
    VINIL_DISASM_FLAGS  Flags,
    CHAR8               *Buffer,
    UINTN               BufferSize
    )
{
    CHAR8   InstBuffer[256];
    UINTN   Pos;
    HRESULT Hr;

    if (Program == NULL || Buffer == NULL) {
        return E_POINTER;
    }

    Pos = 0;

    /* Add program header */
    Pos += snprintf((char *)&Buffer[Pos], BufferSize - Pos,
        "; VINIL IL Program Disassembly\n\n");

    /* TODO: Iterate through all instructions in program */
    /* For now, just show placeholder */
    Hr = VinilDisasmInstruction(Program, Flags, (CHAR8 *)InstBuffer, sizeof(InstBuffer));
    if (FAILED(Hr)) {
        return Hr;
    }

    Pos += snprintf((char *)&Buffer[Pos], BufferSize - Pos,
        "%s\n", InstBuffer);

    return S_OK;
}
