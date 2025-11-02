/** @file
  VINIL Intermediate Language COM Interfaces

  Unified IL COM interfaces for graphics and compute workloads.

  Copyright (C) 2003-2007 Hans-Martin Will.
  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#pragma once
#include <vinil/base.h>
#include <vinil/memory.h>
#include <ananke/com.h>

//
// GUIDs
//

ANX_DEFINE_GUID(IID_IVinilInstruction, 0x45678901, 0x4567, 0x4567, 0x45, 0x67, 0x89, 0x01, 0xAB, 0xCD, 0xEF, 0x23);
ANX_DEFINE_GUID(IID_IVinilBlock, 0x56789012, 0x5678, 0x5678, 0x56, 0x78, 0x90, 0x12, 0xAB, 0xCD, 0xEF, 0x34);
ANX_DEFINE_GUID(IID_IVinilVariable, 0x67890123, 0x6789, 0x6789, 0x67, 0x89, 0x01, 0x23, 0xAB, 0xCD, 0xEF, 0x45);

//
// Forward Declarations
//

typedef struct IVinilInstruction IVinilInstruction;
typedef struct IVinilBlock IVinilBlock;
typedef struct IVinilVariable IVinilVariable;

//
// Opcode Enumeration (complete set for graphics + compute)
//

typedef enum _VINIL_OPCODE {
    /* Data Movement */
    VINIL_OP_MOV = 0,       /* Move */
    VINIL_OP_MOVA,          /* Move address */

    /* Arithmetic - Basic */
    VINIL_OP_ADD,           /* Add */
    VINIL_OP_SUB,           /* Subtract */
    VINIL_OP_MUL,           /* Multiply */
    VINIL_OP_DIV,           /* Divide */
    VINIL_OP_MAD,           /* Multiply-add */
    VINIL_OP_NEG,           /* Negate */
    VINIL_OP_ABS,           /* Absolute value */
    VINIL_OP_MIN,           /* Minimum */
    VINIL_OP_MAX,           /* Maximum */
    VINIL_OP_CLAMP,         /* Clamp to range */

    /* Arithmetic - Reciprocals */
    VINIL_OP_RCP,           /* Reciprocal */
    VINIL_OP_RSQ,           /* Reciprocal square root */

    /* Arithmetic - Rounding */
    VINIL_OP_FRC,           /* Fractional part */
    VINIL_OP_FLR,           /* Floor */
    VINIL_OP_CEIL,          /* Ceiling */
    VINIL_OP_TRUNC,         /* Truncate */
    VINIL_OP_ROUND,         /* Round */

    /* Vector Operations */
    VINIL_OP_DP2,           /* 2D dot product */
    VINIL_OP_DP3,           /* 3D dot product */
    VINIL_OP_DP4,           /* 4D dot product */
    VINIL_OP_CRS,           /* Cross product */
    VINIL_OP_NRM,           /* Normalize */
    VINIL_OP_LEN,           /* Length */

    /* Transcendental */
    VINIL_OP_SIN,           /* Sine */
    VINIL_OP_COS,           /* Cosine */
    VINIL_OP_TAN,           /* Tangent */
    VINIL_OP_ASIN,          /* Arcsine */
    VINIL_OP_ACOS,          /* Arccosine */
    VINIL_OP_ATAN,          /* Arctangent */
    VINIL_OP_ATAN2,         /* Arctangent2 */
    VINIL_OP_EXP,           /* Exponential */
    VINIL_OP_EXP2,          /* Exponential base 2 */
    VINIL_OP_LOG,           /* Logarithm */
    VINIL_OP_LOG2,          /* Logarithm base 2 */
    VINIL_OP_POW,           /* Power */
    VINIL_OP_SQRT,          /* Square root */

    /* Comparison */
    VINIL_OP_SEQ,           /* Set if equal */
    VINIL_OP_SNE,           /* Set if not equal */
    VINIL_OP_SLT,           /* Set if less than */
    VINIL_OP_SLE,           /* Set if less or equal */
    VINIL_OP_SGT,           /* Set if greater than */
    VINIL_OP_SGE,           /* Set if greater or equal */

    /* Logical */
    VINIL_OP_AND,           /* Logical AND */
    VINIL_OP_OR,            /* Logical OR */
    VINIL_OP_XOR,           /* Logical XOR */
    VINIL_OP_NOT,           /* Logical NOT */

    /* Bitwise */
    VINIL_OP_SHL,           /* Shift left */
    VINIL_OP_SHR,           /* Shift right */
    VINIL_OP_SAR,           /* Shift arithmetic right */

    /* Control Flow */
    VINIL_OP_IF,            /* Conditional branch */
    VINIL_OP_ELSE,          /* Else branch */
    VINIL_OP_ENDIF,         /* End if */
    VINIL_OP_LOOP,          /* Loop start */
    VINIL_OP_ENDLOOP,       /* Loop end */
    VINIL_OP_BREAK,         /* Break loop */
    VINIL_OP_CONTINUE,      /* Continue loop */
    VINIL_OP_CALL,          /* Function call */
    VINIL_OP_RET,           /* Return */
    VINIL_OP_DISCARD,       /* Discard fragment */

    /* Texture Sampling - Graphics */
    VINIL_OP_TEX,           /* Texture sample */
    VINIL_OP_TXL,           /* Texture sample with LOD */
    VINIL_OP_TXB,           /* Texture sample with bias */
    VINIL_OP_TXP,           /* Texture sample with projection */
    VINIL_OP_TXD,           /* Texture sample with derivatives */
    VINIL_OP_TXF,           /* Texture fetch */

    /* Memory - Compute */
    VINIL_OP_LOAD,          /* Load from memory */
    VINIL_OP_STORE,         /* Store to memory */
    VINIL_OP_LOAD_VEC,      /* Load vector */
    VINIL_OP_STORE_VEC,     /* Store vector */

    /* Synchronization - Compute */
    VINIL_OP_BARRIER,       /* Work-group barrier */
    VINIL_OP_FENCE,         /* Memory fence */
    VINIL_OP_MEM_FENCE,     /* Memory fence (global) */
    VINIL_OP_READ_FENCE,    /* Read fence */
    VINIL_OP_WRITE_FENCE,   /* Write fence */

    /* Atomic Operations - Compute */
    VINIL_OP_ATOMIC_ADD,    /* Atomic add */
    VINIL_OP_ATOMIC_SUB,    /* Atomic subtract */
    VINIL_OP_ATOMIC_MIN,    /* Atomic minimum */
    VINIL_OP_ATOMIC_MAX,    /* Atomic maximum */
    VINIL_OP_ATOMIC_AND,    /* Atomic AND */
    VINIL_OP_ATOMIC_OR,     /* Atomic OR */
    VINIL_OP_ATOMIC_XOR,    /* Atomic XOR */
    VINIL_OP_ATOMIC_XCHG,   /* Atomic exchange */
    VINIL_OP_ATOMIC_CAS,    /* Atomic compare-and-swap */

    /* Work-Item Functions - Compute */
    VINIL_OP_GET_GLOBAL_ID, /* Get global work-item ID */
    VINIL_OP_GET_LOCAL_ID,  /* Get local work-item ID */
    VINIL_OP_GET_GROUP_ID,  /* Get work-group ID */
    VINIL_OP_GET_GLOBAL_SIZE, /* Get global work size */
    VINIL_OP_GET_LOCAL_SIZE,  /* Get local work size */
    VINIL_OP_GET_NUM_GROUPS,  /* Get number of work-groups */

    /* Miscellaneous */
    VINIL_OP_SELECT,        /* Select (ternary) */
    VINIL_OP_SHUFFLE,       /* Vector shuffle */
    VINIL_OP_NOP,           /* No operation */

    VINIL_OP_COUNT
} VINIL_OPCODE;

//
// IVinilInstruction Interface
//

ANX_BEGIN_INTERFACE(IVinilInstruction, IUnknown, IID_IVinilInstruction, "45678901-4567-4567-4567-8901ABCDEF23")
    /**
      Get instruction opcode.

      @param[out]  Opcode  Instruction opcode.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, GetOpcode, (VINIL_OPCODE *Opcode))

    /**
      Get instruction precision.

      @param[out]  Precision  Instruction precision.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, GetPrecision, (VINIL_PRECISION *Precision))

    /**
      Get destination operand.

      @param[out]  Destination  Destination variable.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, GetDestination, (IVinilVariable **Destination))

    /**
      Get source operand by index.

      @param[in]   Index   Source operand index (0-2).
      @param[out]  Source  Source variable.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
      @retval  E_INVALIDARG  Invalid index.
    **/
    ANX_IFACE_METHOD(HRESULT, GetSource, (UINT32 Index, IVinilVariable **Source))
ANX_END_INTERFACE(IVinilInstruction, IID_IVinilInstruction)

//
// IVinilBlock Interface
//

ANX_BEGIN_INTERFACE(IVinilBlock, IUnknown, IID_IVinilBlock, "56789012-5678-5678-5678-9012ABCDEF34")
    /**
      Get number of instructions in block.

      @param[out]  Count  Instruction count.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, GetInstructionCount, (UINT32 *Count))

    /**
      Get instruction by index.

      @param[in]   Index        Instruction index.
      @param[out]  Instruction  Instruction interface.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
      @retval  E_INVALIDARG  Invalid index.
    **/
    ANX_IFACE_METHOD(HRESULT, GetInstruction, (UINT32 Index, IVinilInstruction **Instruction))

    /**
      Append instruction to block.

      @param[in]  Instruction  Instruction to append.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, AppendInstruction, (IVinilInstruction *Instruction))
ANX_END_INTERFACE(IVinilBlock, IID_IVinilBlock)

//
// IVinilVariable Interface
//

ANX_BEGIN_INTERFACE(IVinilVariable, IUnknown, IID_IVinilVariable, "67890123-6789-6789-6789-0123ABCDEF45")
    /**
      Get variable ID.

      @param[out]  Id  Variable ID.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, GetId, (UINT32 *Id))

    /**
      Get variable name.

      @param[out]  Name        Variable name.
      @param[out]  NameLength  Name length.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, GetName, (CONST CHAR8 **Name, UINTN *NameLength))
ANX_END_INTERFACE(IVinilVariable, IID_IVinilVariable)

