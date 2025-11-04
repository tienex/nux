/** @file
  VINIL IL Opcode Information Table

  Shared opcode metadata for disassembler and assembler.
  Single source of truth for opcode mnemonics and properties.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#include <vinil/disasm.h>
#include <vinil/il.h>

//
// Complete Opcode Information Table
// Indexed by VINIL_OPCODE enum values from il.h
//

CONST VINIL_OPCODE_INFO gVinilOpcodeTable[VINIL_OP_COUNT] = {
    /* Data Movement */
    [VINIL_OP_MOV]      = { VINIL_OP_MOV,      "MOV",      "Move",                        VinilOpcatArithmetic,     1, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_MOVA]     = { VINIL_OP_MOVA,     "MOVA",     "Move address",                VinilOpcatArithmetic,     1, TRUE,  TRUE,  TRUE  },

    /* Arithmetic - Basic */
    [VINIL_OP_ADD]      = { VINIL_OP_ADD,      "ADD",      "Add",                         VinilOpcatArithmetic,     2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_SUB]      = { VINIL_OP_SUB,      "SUB",      "Subtract",                    VinilOpcatArithmetic,     2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_MUL]      = { VINIL_OP_MUL,      "MUL",      "Multiply",                    VinilOpcatArithmetic,     2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_DIV]      = { VINIL_OP_DIV,      "DIV",      "Divide",                      VinilOpcatArithmetic,     2, TRUE,  FALSE, TRUE  },
    [VINIL_OP_MAD]      = { VINIL_OP_MAD,      "MAD",      "Multiply-add",                VinilOpcatArithmetic,     3, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_NEG]      = { VINIL_OP_NEG,      "NEG",      "Negate",                      VinilOpcatArithmetic,     1, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_ABS]      = { VINIL_OP_ABS,      "ABS",      "Absolute value",              VinilOpcatArithmetic,     1, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_MIN]      = { VINIL_OP_MIN,      "MIN",      "Minimum",                     VinilOpcatArithmetic,     2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_MAX]      = { VINIL_OP_MAX,      "MAX",      "Maximum",                     VinilOpcatArithmetic,     2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_CLAMP]    = { VINIL_OP_CLAMP,    "CLAMP",    "Clamp to range",              VinilOpcatArithmetic,     3, TRUE,  TRUE,  TRUE  },

    /* Arithmetic - Reciprocals */
    [VINIL_OP_RCP]      = { VINIL_OP_RCP,      "RCP",      "Reciprocal",                  VinilOpcatArithmetic,     1, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_RSQ]      = { VINIL_OP_RSQ,      "RSQ",      "Reciprocal square root",      VinilOpcatArithmetic,     1, TRUE,  TRUE,  TRUE  },

    /* Arithmetic - Rounding */
    [VINIL_OP_FRC]      = { VINIL_OP_FRC,      "FRC",      "Fractional part",             VinilOpcatArithmetic,     1, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_FLR]      = { VINIL_OP_FLR,      "FLR",      "Floor",                       VinilOpcatArithmetic,     1, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_CEIL]     = { VINIL_OP_CEIL,     "CEIL",     "Ceiling",                     VinilOpcatArithmetic,     1, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_TRUNC]    = { VINIL_OP_TRUNC,    "TRUNC",    "Truncate",                    VinilOpcatArithmetic,     1, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_ROUND]    = { VINIL_OP_ROUND,    "ROUND",    "Round",                       VinilOpcatArithmetic,     1, TRUE,  TRUE,  TRUE  },

    /* Vector Operations */
    [VINIL_OP_DP2]      = { VINIL_OP_DP2,      "DP2",      "Dot product 2D",              VinilOpcatVector,         2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_DP3]      = { VINIL_OP_DP3,      "DP3",      "Dot product 3D",              VinilOpcatVector,         2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_DP4]      = { VINIL_OP_DP4,      "DP4",      "Dot product 4D",              VinilOpcatVector,         2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_CRS]      = { VINIL_OP_CRS,      "CRS",      "Cross product",               VinilOpcatVector,         2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_NRM]      = { VINIL_OP_NRM,      "NRM",      "Normalize",                   VinilOpcatVector,         1, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_LEN]      = { VINIL_OP_LEN,      "LEN",      "Length",                      VinilOpcatVector,         1, TRUE,  TRUE,  TRUE  },

    /* Transcendental */
    [VINIL_OP_SIN]      = { VINIL_OP_SIN,      "SIN",      "Sine",                        VinilOpcatTranscendental, 1, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_COS]      = { VINIL_OP_COS,      "COS",      "Cosine",                      VinilOpcatTranscendental, 1, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_TAN]      = { VINIL_OP_TAN,      "TAN",      "Tangent",                     VinilOpcatTranscendental, 1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_ASIN]     = { VINIL_OP_ASIN,     "ASIN",     "Arc sine",                    VinilOpcatTranscendental, 1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_ACOS]     = { VINIL_OP_ACOS,     "ACOS",     "Arc cosine",                  VinilOpcatTranscendental, 1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_ATAN]     = { VINIL_OP_ATAN,     "ATAN",     "Arc tangent",                 VinilOpcatTranscendental, 1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_ATAN2]    = { VINIL_OP_ATAN2,    "ATAN2",    "Arc tangent 2",               VinilOpcatTranscendental, 2, TRUE,  FALSE, TRUE  },
    [VINIL_OP_EXP]      = { VINIL_OP_EXP,      "EXP",      "Exponential (base e)",        VinilOpcatTranscendental, 1, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_EXP2]     = { VINIL_OP_EXP2,     "EXP2",     "Exponential (base 2)",        VinilOpcatTranscendental, 1, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_LOG]      = { VINIL_OP_LOG,      "LOG",      "Logarithm (base e)",          VinilOpcatTranscendental, 1, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_LOG2]     = { VINIL_OP_LOG2,     "LOG2",     "Logarithm (base 2)",          VinilOpcatTranscendental, 1, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_POW]      = { VINIL_OP_POW,      "POW",      "Power",                       VinilOpcatTranscendental, 2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_SQRT]     = { VINIL_OP_SQRT,     "SQRT",     "Square root",                 VinilOpcatTranscendental, 1, TRUE,  TRUE,  TRUE  },

    /* Comparison */
    [VINIL_OP_SEQ]      = { VINIL_OP_SEQ,      "SEQ",      "Set if equal",                VinilOpcatComparison,     2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_SNE]      = { VINIL_OP_SNE,      "SNE",      "Set if not equal",            VinilOpcatComparison,     2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_SLT]      = { VINIL_OP_SLT,      "SLT",      "Set if less than",            VinilOpcatComparison,     2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_SLE]      = { VINIL_OP_SLE,      "SLE",      "Set if less or equal",        VinilOpcatComparison,     2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_SGT]      = { VINIL_OP_SGT,      "SGT",      "Set if greater than",         VinilOpcatComparison,     2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_SGE]      = { VINIL_OP_SGE,      "SGE",      "Set if greater or equal",     VinilOpcatComparison,     2, TRUE,  TRUE,  TRUE  },

    /* Logical */
    [VINIL_OP_AND]      = { VINIL_OP_AND,      "AND",      "Logical AND",                 VinilOpcatComparison,     2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_OR]       = { VINIL_OP_OR,       "OR",       "Logical OR",                  VinilOpcatComparison,     2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_XOR]      = { VINIL_OP_XOR,      "XOR",      "Logical XOR",                 VinilOpcatComparison,     2, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_NOT]      = { VINIL_OP_NOT,      "NOT",      "Logical NOT",                 VinilOpcatComparison,     1, TRUE,  TRUE,  TRUE  },

    /* Bitwise */
    [VINIL_OP_SHL]      = { VINIL_OP_SHL,      "SHL",      "Shift left",                  VinilOpcatArithmetic,     2, TRUE,  FALSE, TRUE  },
    [VINIL_OP_SHR]      = { VINIL_OP_SHR,      "SHR",      "Shift right",                 VinilOpcatArithmetic,     2, TRUE,  FALSE, TRUE  },
    [VINIL_OP_SAR]      = { VINIL_OP_SAR,      "SAR",      "Shift arithmetic right",      VinilOpcatArithmetic,     2, TRUE,  FALSE, TRUE  },

    /* Control Flow */
    [VINIL_OP_IF]       = { VINIL_OP_IF,       "IF",       "Begin conditional",           VinilOpcatControlFlow,    1, FALSE, TRUE,  TRUE  },
    [VINIL_OP_ELSE]     = { VINIL_OP_ELSE,     "ELSE",     "Else clause",                 VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    [VINIL_OP_ENDIF]    = { VINIL_OP_ENDIF,    "ENDIF",    "End conditional",             VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    [VINIL_OP_LOOP]     = { VINIL_OP_LOOP,     "LOOP",     "Loop start",                  VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    [VINIL_OP_ENDLOOP]  = { VINIL_OP_ENDLOOP,  "ENDLOOP",  "Loop end",                    VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    [VINIL_OP_BREAK]    = { VINIL_OP_BREAK,    "BREAK",    "Break loop",                  VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    [VINIL_OP_CONTINUE] = { VINIL_OP_CONTINUE, "CONTINUE", "Continue loop",               VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    [VINIL_OP_CALL]     = { VINIL_OP_CALL,     "CALL",     "Function call",               VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    [VINIL_OP_RET]      = { VINIL_OP_RET,      "RET",      "Return",                      VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },
    [VINIL_OP_DISCARD]  = { VINIL_OP_DISCARD,  "DISCARD",  "Discard fragment",            VinilOpcatControlFlow,    0, FALSE, TRUE,  FALSE },

    /* Texture Sampling - Graphics */
    [VINIL_OP_TEX]      = { VINIL_OP_TEX,      "TEX",      "Texture sample",              VinilOpcatTexture,        2, TRUE,  TRUE,  FALSE },
    [VINIL_OP_TXL]      = { VINIL_OP_TXL,      "TXL",      "Texture sample with LOD",     VinilOpcatTexture,        2, TRUE,  TRUE,  FALSE },
    [VINIL_OP_TXB]      = { VINIL_OP_TXB,      "TXB",      "Texture sample with bias",    VinilOpcatTexture,        2, TRUE,  TRUE,  FALSE },
    [VINIL_OP_TXP]      = { VINIL_OP_TXP,      "TXP",      "Texture sample with proj",    VinilOpcatTexture,        2, TRUE,  TRUE,  FALSE },
    [VINIL_OP_TXD]      = { VINIL_OP_TXD,      "TXD",      "Texture sample with deriv",   VinilOpcatTexture,        3, TRUE,  TRUE,  FALSE },
    [VINIL_OP_TXF]      = { VINIL_OP_TXF,      "TXF",      "Texture fetch",               VinilOpcatTexture,        2, TRUE,  TRUE,  FALSE },

    /* Memory - Compute */
    [VINIL_OP_LOAD]      = { VINIL_OP_LOAD,      "LOAD",      "Load from memory",            VinilOpcatMemory,         1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_STORE]     = { VINIL_OP_STORE,     "STORE",     "Store to memory",             VinilOpcatMemory,         2, FALSE, FALSE, TRUE  },
    [VINIL_OP_LOAD_VEC]  = { VINIL_OP_LOAD_VEC,  "LOAD_VEC",  "Load vector",                 VinilOpcatMemory,         1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_STORE_VEC] = { VINIL_OP_STORE_VEC, "STORE_VEC", "Store vector",                VinilOpcatMemory,         2, FALSE, FALSE, TRUE  },

    /* Synchronization - Compute */
    [VINIL_OP_BARRIER]      = { VINIL_OP_BARRIER,      "BARRIER",      "Work-group barrier",          VinilOpcatSync, 0, FALSE, FALSE, TRUE  },
    [VINIL_OP_FENCE]        = { VINIL_OP_FENCE,        "FENCE",        "Fence",                       VinilOpcatSync, 1, FALSE, FALSE, TRUE  },
    [VINIL_OP_MEM_FENCE]    = { VINIL_OP_MEM_FENCE,    "MEM_FENCE",    "Memory fence (global)",       VinilOpcatSync, 1, FALSE, FALSE, TRUE  },
    [VINIL_OP_READ_FENCE]   = { VINIL_OP_READ_FENCE,   "READ_FENCE",   "Read fence",                  VinilOpcatSync, 1, FALSE, FALSE, TRUE  },
    [VINIL_OP_WRITE_FENCE]  = { VINIL_OP_WRITE_FENCE,  "WRITE_FENCE",  "Write fence",                 VinilOpcatSync, 1, FALSE, FALSE, TRUE  },

    /* Atomic Operations - Compute */
    [VINIL_OP_ATOMIC_ADD]  = { VINIL_OP_ATOMIC_ADD,  "ATOMIC.ADD",  "Atomic add",                  VinilOpcatAtomic, 2, TRUE,  FALSE, TRUE  },
    [VINIL_OP_ATOMIC_SUB]  = { VINIL_OP_ATOMIC_SUB,  "ATOMIC.SUB",  "Atomic subtract",             VinilOpcatAtomic, 2, TRUE,  FALSE, TRUE  },
    [VINIL_OP_ATOMIC_MIN]  = { VINIL_OP_ATOMIC_MIN,  "ATOMIC.MIN",  "Atomic minimum",              VinilOpcatAtomic, 2, TRUE,  FALSE, TRUE  },
    [VINIL_OP_ATOMIC_MAX]  = { VINIL_OP_ATOMIC_MAX,  "ATOMIC.MAX",  "Atomic maximum",              VinilOpcatAtomic, 2, TRUE,  FALSE, TRUE  },
    [VINIL_OP_ATOMIC_AND]  = { VINIL_OP_ATOMIC_AND,  "ATOMIC.AND",  "Atomic AND",                  VinilOpcatAtomic, 2, TRUE,  FALSE, TRUE  },
    [VINIL_OP_ATOMIC_OR]   = { VINIL_OP_ATOMIC_OR,   "ATOMIC.OR",   "Atomic OR",                   VinilOpcatAtomic, 2, TRUE,  FALSE, TRUE  },
    [VINIL_OP_ATOMIC_XOR]  = { VINIL_OP_ATOMIC_XOR,  "ATOMIC.XOR",  "Atomic XOR",                  VinilOpcatAtomic, 2, TRUE,  FALSE, TRUE  },
    [VINIL_OP_ATOMIC_XCHG] = { VINIL_OP_ATOMIC_XCHG, "ATOMIC.XCHG", "Atomic exchange",             VinilOpcatAtomic, 2, TRUE,  FALSE, TRUE  },
    [VINIL_OP_ATOMIC_CAS]  = { VINIL_OP_ATOMIC_CAS,  "ATOMIC.CAS",  "Atomic compare-and-swap",     VinilOpcatAtomic, 3, TRUE,  FALSE, TRUE  },

    /* Work-Item Functions - Compute */
    [VINIL_OP_GET_GLOBAL_ID]   = { VINIL_OP_GET_GLOBAL_ID,   "GET_GLOBAL_ID",   "Get global work-item ID",     VinilOpcatWorkItem, 1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_GET_LOCAL_ID]    = { VINIL_OP_GET_LOCAL_ID,    "GET_LOCAL_ID",    "Get local work-item ID",      VinilOpcatWorkItem, 1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_GET_GROUP_ID]    = { VINIL_OP_GET_GROUP_ID,    "GET_GROUP_ID",    "Get work-group ID",           VinilOpcatWorkItem, 1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_GET_GLOBAL_SIZE] = { VINIL_OP_GET_GLOBAL_SIZE, "GET_GLOBAL_SIZE", "Get global work size",        VinilOpcatWorkItem, 1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_GET_LOCAL_SIZE]  = { VINIL_OP_GET_LOCAL_SIZE,  "GET_LOCAL_SIZE",  "Get local work size",         VinilOpcatWorkItem, 1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_GET_NUM_GROUPS]  = { VINIL_OP_GET_NUM_GROUPS,  "GET_NUM_GROUPS",  "Get number of work-groups",   VinilOpcatWorkItem, 1, TRUE,  FALSE, TRUE  },

    /* Miscellaneous */
    [VINIL_OP_SELECT]  = { VINIL_OP_SELECT,  "SELECT",  "Select (ternary)",            VinilOpcatArithmetic,     3, TRUE,  TRUE,  TRUE  },
    [VINIL_OP_SHUFFLE] = { VINIL_OP_SHUFFLE, "SHUFFLE", "Vector shuffle",              VinilOpcatVector,         2, TRUE,  FALSE, TRUE  },
    [VINIL_OP_NOP]     = { VINIL_OP_NOP,     "NOP",     "No operation",                VinilOpcatControlFlow,    0, FALSE, TRUE,  TRUE  },

    /* Stack Operations - Bytecode */
    [VINIL_OP_PUSH]    = { VINIL_OP_PUSH,    "PUSH",    "Push to stack",               VinilOpcatControlFlow,    1, FALSE, FALSE, TRUE  },
    [VINIL_OP_POP]     = { VINIL_OP_POP,     "POP",     "Pop from stack",              VinilOpcatControlFlow,    0, TRUE,  FALSE, TRUE  },
    [VINIL_OP_PUSHN]   = { VINIL_OP_PUSHN,   "PUSHN",   "Push N bytes",                VinilOpcatControlFlow,    1, FALSE, FALSE, TRUE  },
    [VINIL_OP_POPN]    = { VINIL_OP_POPN,    "POPN",    "Pop N bytes",                 VinilOpcatControlFlow,    1, FALSE, FALSE, TRUE  },
    [VINIL_OP_DUP]     = { VINIL_OP_DUP,     "DUP",     "Duplicate top",               VinilOpcatControlFlow,    0, FALSE, FALSE, TRUE  },
    [VINIL_OP_SWAP]    = { VINIL_OP_SWAP,    "SWAP",    "Swap top two",                VinilOpcatControlFlow,    0, FALSE, FALSE, TRUE  },

    /* Sized Moves - Bytecode */
    [VINIL_OP_MOV8]    = { VINIL_OP_MOV8,    "MOV8",    "Move 8-bit",                  VinilOpcatArithmetic,     1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_MOV16]   = { VINIL_OP_MOV16,   "MOV16",   "Move 16-bit",                 VinilOpcatArithmetic,     1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_MOV32]   = { VINIL_OP_MOV32,   "MOV32",   "Move 32-bit",                 VinilOpcatArithmetic,     1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_MOV64]   = { VINIL_OP_MOV64,   "MOV64",   "Move 64-bit",                 VinilOpcatArithmetic,     1, TRUE,  FALSE, TRUE  },

    /* Zero Extension - Bytecode */
    [VINIL_OP_ZEXT8]   = { VINIL_OP_ZEXT8,   "ZEXT8",   "Zero extend 8->32",           VinilOpcatArithmetic,     1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_ZEXT16]  = { VINIL_OP_ZEXT16,  "ZEXT16",  "Zero extend 16->32",          VinilOpcatArithmetic,     1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_ZEXT32]  = { VINIL_OP_ZEXT32,  "ZEXT32",  "Zero extend 32->64",          VinilOpcatArithmetic,     1, TRUE,  FALSE, TRUE  },

    /* Sign Extension - Bytecode */
    [VINIL_OP_SEXT8]   = { VINIL_OP_SEXT8,   "SEXT8",   "Sign extend 8->32",           VinilOpcatArithmetic,     1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_SEXT16]  = { VINIL_OP_SEXT16,  "SEXT16",  "Sign extend 16->32",          VinilOpcatArithmetic,     1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_SEXT32]  = { VINIL_OP_SEXT32,  "SEXT32",  "Sign extend 32->64",          VinilOpcatArithmetic,     1, TRUE,  FALSE, TRUE  },

    /* Truncation - Bytecode */
    [VINIL_OP_TRUNC8]  = { VINIL_OP_TRUNC8,  "TRUNC8",  "Truncate to 8-bit",           VinilOpcatArithmetic,     1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_TRUNC16] = { VINIL_OP_TRUNC16, "TRUNC16", "Truncate to 16-bit",          VinilOpcatArithmetic,     1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_TRUNC32] = { VINIL_OP_TRUNC32, "TRUNC32", "Truncate to 32-bit",          VinilOpcatArithmetic,     1, TRUE,  FALSE, TRUE  },

    /* Unsigned Arithmetic - Bytecode */
    [VINIL_OP_MULU]    = { VINIL_OP_MULU,    "MULU",    "Unsigned multiply",           VinilOpcatArithmetic,     2, TRUE,  FALSE, TRUE  },
    [VINIL_OP_DIVU]    = { VINIL_OP_DIVU,    "DIVU",    "Unsigned divide",             VinilOpcatArithmetic,     2, TRUE,  FALSE, TRUE  },
    [VINIL_OP_MODU]    = { VINIL_OP_MODU,    "MODU",    "Unsigned modulo",             VinilOpcatArithmetic,     2, TRUE,  FALSE, TRUE  },

    /* Comparison - Bytecode */
    [VINIL_OP_CMP]     = { VINIL_OP_CMP,     "CMP",     "Compare (set flags)",         VinilOpcatComparison,     2, FALSE, FALSE, TRUE  },
    [VINIL_OP_CMPU]    = { VINIL_OP_CMPU,    "CMPU",    "Compare unsigned",            VinilOpcatComparison,     2, FALSE, FALSE, TRUE  },

    /* Conditional Branches - Bytecode */
    [VINIL_OP_JEQ]     = { VINIL_OP_JEQ,     "JEQ",     "Jump if equal",               VinilOpcatControlFlow,    1, FALSE, FALSE, TRUE  },
    [VINIL_OP_JNE]     = { VINIL_OP_JNE,     "JNE",     "Jump if not equal",           VinilOpcatControlFlow,    1, FALSE, FALSE, TRUE  },
    [VINIL_OP_JLT]     = { VINIL_OP_JLT,     "JLT",     "Jump if less than",           VinilOpcatControlFlow,    1, FALSE, FALSE, TRUE  },
    [VINIL_OP_JLE]     = { VINIL_OP_JLE,     "JLE",     "Jump if less or equal",       VinilOpcatControlFlow,    1, FALSE, FALSE, TRUE  },
    [VINIL_OP_JGT]     = { VINIL_OP_JGT,     "JGT",     "Jump if greater than",        VinilOpcatControlFlow,    1, FALSE, FALSE, TRUE  },
    [VINIL_OP_JGE]     = { VINIL_OP_JGE,     "JGE",     "Jump if greater or equal",    VinilOpcatControlFlow,    1, FALSE, FALSE, TRUE  },
    [VINIL_OP_JMP]     = { VINIL_OP_JMP,     "JMP",     "Unconditional jump",          VinilOpcatControlFlow,    1, FALSE, FALSE, TRUE  },

    /* Memory with Index - Bytecode */
    [VINIL_OP_LOAD_INDEXED]  = { VINIL_OP_LOAD_INDEXED,  "LOAD.IDX",  "Load indexed",                VinilOpcatMemory,         3, TRUE,  FALSE, TRUE  },
    [VINIL_OP_STORE_INDEXED] = { VINIL_OP_STORE_INDEXED, "STORE.IDX", "Store indexed",               VinilOpcatMemory,         4, FALSE, FALSE, TRUE  },
    [VINIL_OP_LEA]           = { VINIL_OP_LEA,           "LEA",       "Load effective address",      VinilOpcatMemory,         2, TRUE,  FALSE, TRUE  },

    /* Pointer Operations - Bytecode */
    [VINIL_OP_LOAD_PTR]  = { VINIL_OP_LOAD_PTR,  "LOAD.PTR",  "Load via pointer",            VinilOpcatMemory,         1, TRUE,  FALSE, TRUE  },
    [VINIL_OP_STORE_PTR] = { VINIL_OP_STORE_PTR, "STORE.PTR", "Store via pointer",           VinilOpcatMemory,         2, FALSE, FALSE, TRUE  },

    /* System - Bytecode */
    [VINIL_OP_TRAP]      = { VINIL_OP_TRAP,      "TRAP",      "Software trap",               VinilOpcatControlFlow,    0, FALSE, FALSE, TRUE  },
    [VINIL_OP_NOP_HINT]  = { VINIL_OP_NOP_HINT,  "NOP.HINT",  "No-op with hint",             VinilOpcatControlFlow,    1, FALSE, FALSE, TRUE  },
};
