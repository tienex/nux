/** @file
  VINIL Assembly Language

  Text-based assembly language for authoring and editing VINIL IL programs.
  Provides human-readable syntax for creating IL programs.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#ifndef __vinil_asm_h__
#define __vinil_asm_h__ 1

#include <ananke/types.h>
#include <ananke/hresult.h>

//
// Assembly Error Information
//

typedef struct _VINIL_ASM_ERROR {
    UINT32      Line;           /* Line number where error occurred */
    UINT32      Column;         /* Column number */
    CONST CHAR8 *Message;       /* Error message */
} VINIL_ASM_ERROR;

//
// Assembly Options
//

typedef enum _VINIL_ASM_FLAGS {
    VinilAsmNone            = 0,
    VinilAsmOptimize        = (1 << 0),  /* Optimize during assembly */
    VinilAsmDebugInfo       = (1 << 1),  /* Include debug information */
    VinilAsmVerbose         = (1 << 2),  /* Verbose error messages */
} VINIL_ASM_FLAGS;

//
// Assembler Functions
//

/**
  Assemble IL program from text.

  @param[in]   Source       Assembly source text.
  @param[in]   SourceSize   Size of source in bytes.
  @param[in]   Flags        Assembly flags.
  @param[out]  Program      Assembled IL program.
  @param[out]  Error        Error information (optional).

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Assembly failed (check Error for details).
**/
HRESULT
VinilAssemble (
    CONST CHAR8         *Source,
    UINTN               SourceSize,
    VINIL_ASM_FLAGS     Flags,
    VOID                **Program,
    VINIL_ASM_ERROR     *Error
    );

/**
  Assemble IL program from file.

  @param[in]   FilePath  Path to assembly source file.
  @param[in]   Flags     Assembly flags.
  @param[out]  Program   Assembled IL program.
  @param[out]  Error     Error information (optional).

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Assembly failed or I/O error.
**/
HRESULT
VinilAssembleFile (
    CONST CHAR8         *FilePath,
    VINIL_ASM_FLAGS     Flags,
    VOID                **Program,
    VINIL_ASM_ERROR     *Error
    );

/**
  Validate assembly syntax without assembling.

  @param[in]   Source      Assembly source text.
  @param[in]   SourceSize  Size of source in bytes.
  @param[out]  Error       Error information (optional).

  @retval  S_OK   Valid syntax.
  @retval  E_FAIL Invalid syntax (check Error for details).
**/
HRESULT
VinilValidateAsm (
    CONST CHAR8         *Source,
    UINTN               SourceSize,
    VINIL_ASM_ERROR     *Error
    );

//
// Assembly Language Syntax Reference
//

/*
  VINIL Assembly Language Syntax:

  1. COMMENTS
     ; This is a line comment
     // This is also a line comment

  2. DIRECTIVES
     .mode graphics          ; Set execution mode (graphics/compute)
     .version 1 0            ; Set IL version
     .precision mediump      ; Set default precision

  3. VARIABLE DECLARATIONS
     input  vec4 position;   ; Input variable
     output vec4 color;      ; Output variable
     temp   vec4 t0, t1;     ; Temporary variables
     param  mat4 mvp;        ; Uniform parameter

  4. INSTRUCTIONS
     MOV    t0, position     ; Move/copy
     MUL    t1, mvp, t0      ; Matrix multiply
     MAD    color, t1, c0, c1 ; Multiply-add
     DP3    t0.x, t1, t2     ; Dot product with write mask
     TEX    color, t0, s0    ; Texture sample

  5. OPERAND SYNTAX
     r0              ; Register
     r0.xyz          ; Swizzle
     r0.xyzw         ; Full swizzle
     -r0             ; Negate
     r0[a0]          ; Indexed access
     c[5]            ; Constant array access

  6. CONTROL FLOW
     IF     t0.x             ; Conditional
       MOV  color, c0
     ELSE
       MOV  color, c1
     ENDIF

     LOOP                    ; Loop
       ADD  t0, t0, c0
       BRK                   ; Break
     ENDLOOP

  7. LABELS
     main:                   ; Define label
       CAL  subroutine       ; Call subroutine
       RET                   ; Return

     subroutine:
       MOV  t0, c0
       RET

  8. COMPUTE EXTENSIONS
     LD.GLOBAL  t0, g[0]     ; Load from global memory
     ST.GLOBAL  g[0], t0     ; Store to global memory
     GET_GLOBAL_ID t0.x, 0   ; Get work-item ID
     BARRIER                 ; Work-group barrier
     ATOMIC.ADD t0, g[0], c0 ; Atomic add
*/

#endif // __vinil_asm_h__
