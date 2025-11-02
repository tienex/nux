/** @file
  VINIL Binary Serialization

  Complete implementation of binary serialization/deserialization for IL programs.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#define COBJMACROS
#include <vinil/vinil.h>
#include <vinil/binary.h>
#include <vinil/il.h>
#include "vinil_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//
// Serialized Instruction Format
//

typedef struct _VINIL_SERIALIZED_INSTRUCTION {
  VINIL_OPCODE  Opcode;
  UINT32        DstId;
  UINT32        Src0Id;
  UINT32        Src1Id;
  UINT32        Src2Id;
} VINIL_SERIALIZED_INSTRUCTION;

//
// Validation
//

HRESULT
VinilValidateBinary (
  CONST VOID  *Buffer,
  UINTN       BufferSize
  )
{
  CONST VINIL_BINARY_HEADER  *Header;

  if (Buffer == NULL) {
    return E_POINTER;
  }

  if (BufferSize < sizeof (VINIL_BINARY_HEADER)) {
    return E_FAIL;
  }

  Header = (CONST VINIL_BINARY_HEADER *)Buffer;

  /* Check magic number */
  if (Header->Magic != VINIL_BINARY_MAGIC) {
    return E_FAIL;
  }

  /* Check version */
  if (Header->Version > VINIL_BINARY_VERSION) {
    return E_FAIL;
  }

  /* Validate execution mode */
  if (Header->Mode > VinilModeHybrid) {
    return E_FAIL;
  }

  return S_OK;
}

//
// Serialization
//

HRESULT
VinilSerializeProgram (
  CONST VOID  *ProgramPtr,
  VOID        *Buffer,
  UINTN       BufferSize,
  UINTN       *BytesWritten
  )
{
  CONST VINIL_PROGRAM_IMPL      *Program;
  CONST VINIL_INSTRUCTION_NODE  *Inst;
  VINIL_BINARY_HEADER           *Header;
  VINIL_SECTION_HEADER          *SectionHdr;
  VINIL_SERIALIZED_INSTRUCTION  *SerializedInst;
  UINT8                         *DataPtr;
  UINTN                         Offset;
  UINTN                         CodeSize;
  UINTN                         TotalSize;
  UINT32                        InstructionCount;
  UINT32                        i;

  if (ProgramPtr == NULL || Buffer == NULL) {
    return E_POINTER;
  }

  Program = (CONST VINIL_PROGRAM_IMPL *)ProgramPtr;

  /* Calculate code section size */
  InstructionCount = Program->InstructionCount;
  CodeSize = InstructionCount * sizeof (VINIL_SERIALIZED_INSTRUCTION);

  /* Calculate total size */
  TotalSize = sizeof (VINIL_BINARY_HEADER);
  TotalSize += sizeof (VINIL_SECTION_HEADER);
  TotalSize += CodeSize;

  if (BytesWritten != NULL) {
    *BytesWritten = TotalSize;
  }

  if (BufferSize < TotalSize) {
    return E_OUTOFMEMORY;
  }

  /* Initialize buffer */
  memset (Buffer, 0, BufferSize);
  DataPtr = (UINT8 *)Buffer;
  Offset = 0;

  /* Write file header */
  Header = (VINIL_BINARY_HEADER *)&DataPtr[Offset];
  Header->Magic = VINIL_BINARY_MAGIC;
  Header->Version = VINIL_BINARY_VERSION;
  Header->Mode = (Program->Mode == VinilExecutionModeGraphics) ? VinilModeGraphics : VinilModeCompute;
  Header->NumSections = 1;
  Header->Flags = 0;
  Offset += sizeof (VINIL_BINARY_HEADER);

  /* Write code section header */
  SectionHdr = (VINIL_SECTION_HEADER *)&DataPtr[Offset];
  SectionHdr->Type = VinilSectionCode;
  SectionHdr->Size = (UINT32)CodeSize;
  SectionHdr->Offset = (UINT32)(Offset + sizeof (VINIL_SECTION_HEADER));
  SectionHdr->Alignment = 4;
  Offset += sizeof (VINIL_SECTION_HEADER);

  /* Write instructions */
  SerializedInst = (VINIL_SERIALIZED_INSTRUCTION *)&DataPtr[Offset];
  Inst = Program->FirstInstruction;
  i = 0;

  while (Inst != NULL && i < InstructionCount) {
    SerializedInst[i].Opcode = Inst->Opcode;

    /* Get variable IDs */
    if (Inst->Dst != NULL) {
      IVinilVariable_GetId (Inst->Dst, &SerializedInst[i].DstId);
    } else {
      SerializedInst[i].DstId = 0xFFFFFFFF;
    }

    if (Inst->Src[0] != NULL) {
      IVinilVariable_GetId (Inst->Src[0], &SerializedInst[i].Src0Id);
    } else {
      SerializedInst[i].Src0Id = 0xFFFFFFFF;
    }

    if (Inst->Src[1] != NULL) {
      IVinilVariable_GetId (Inst->Src[1], &SerializedInst[i].Src1Id);
    } else {
      SerializedInst[i].Src1Id = 0xFFFFFFFF;
    }

    if (Inst->Src[2] != NULL) {
      IVinilVariable_GetId (Inst->Src[2], &SerializedInst[i].Src2Id);
    } else {
      SerializedInst[i].Src2Id = 0xFFFFFFFF;
    }

    Inst = Inst->Next;
    i++;
  }

  return S_OK;
}

//
// Deserialization
//

HRESULT
VinilDeserializeProgram (
  CONST VOID  *Buffer,
  UINTN       BufferSize,
  VOID        **ProgramPtr
  )
{
  CONST VINIL_BINARY_HEADER           *Header;
  CONST VINIL_SECTION_HEADER          *SectionHdr;
  CONST VINIL_SERIALIZED_INSTRUCTION  *SerializedInst;
  CONST UINT8                         *DataPtr;
  IVinilProgram                       *Program;
  IVinilMemoryPool                    *MemoryPool;
  UINTN                               Offset;
  HRESULT                             Result;
  UINT32                              i;
  UINT32                              InstructionCount;
  VINIL_EXECUTION_MODE                Mode;
  IVinilVariable                      **Variables;
  UINT32                              MaxVariableId;

  if (Buffer == NULL || ProgramPtr == NULL) {
    return E_POINTER;
  }

  /* Validate binary format */
  Result = VinilValidateBinary (Buffer, BufferSize);
  if (FAILED (Result)) {
    return Result;
  }

  DataPtr = (CONST UINT8 *)Buffer;
  Header = (CONST VINIL_BINARY_HEADER *)DataPtr;
  Offset = sizeof (VINIL_BINARY_HEADER);

  /* Get execution mode */
  Mode = (Header->Mode == VinilModeGraphics) ? VinilExecutionModeGraphics : VinilExecutionModeCompute;

  /* Read code section */
  if (Offset + sizeof (VINIL_SECTION_HEADER) > BufferSize) {
    return E_FAIL;
  }

  SectionHdr = (CONST VINIL_SECTION_HEADER *)&DataPtr[Offset];
  Offset += sizeof (VINIL_SECTION_HEADER);

  if (SectionHdr->Type != VinilSectionCode) {
    return E_FAIL;
  }

  if (Offset + SectionHdr->Size > BufferSize) {
    return E_FAIL;
  }

  SerializedInst = (CONST VINIL_SERIALIZED_INSTRUCTION *)&DataPtr[Offset];
  InstructionCount = SectionHdr->Size / sizeof (VINIL_SERIALIZED_INSTRUCTION);

  /* Create memory pool and program */
  Result = VinilCreateMemoryPool (0, &MemoryPool);
  if (FAILED (Result)) {
    return Result;
  }

  Result = VinilProgramCreate (Mode, MemoryPool, &Program);
  if (FAILED (Result)) {
    IVinilMemoryPool_Release (MemoryPool);
    return Result;
  }

  /* Find maximum variable ID */
  MaxVariableId = 0;
  for (i = 0; i < InstructionCount; i++) {
    if (SerializedInst[i].DstId != 0xFFFFFFFF && SerializedInst[i].DstId > MaxVariableId) {
      MaxVariableId = SerializedInst[i].DstId;
    }
    if (SerializedInst[i].Src0Id != 0xFFFFFFFF && SerializedInst[i].Src0Id > MaxVariableId) {
      MaxVariableId = SerializedInst[i].Src0Id;
    }
    if (SerializedInst[i].Src1Id != 0xFFFFFFFF && SerializedInst[i].Src1Id > MaxVariableId) {
      MaxVariableId = SerializedInst[i].Src1Id;
    }
    if (SerializedInst[i].Src2Id != 0xFFFFFFFF && SerializedInst[i].Src2Id > MaxVariableId) {
      MaxVariableId = SerializedInst[i].Src2Id;
    }
  }

  /* Create variables array */
  Variables = (IVinilVariable **)malloc ((MaxVariableId + 1) * sizeof (IVinilVariable *));
  if (Variables == NULL) {
    IVinilProgram_Release (Program);
    IVinilMemoryPool_Release (MemoryPool);
    return E_OUTOFMEMORY;
  }

  memset (Variables, 0, (MaxVariableId + 1) * sizeof (IVinilVariable *));

  /* Create all variables */
  for (i = 0; i <= MaxVariableId; i++) {
    IVinilType  *Type;
    CHAR8       Name[32];

    snprintf ((char *)Name, sizeof (Name), "v%u", i);

    /* Use float4 as default type for deserialization */
    Result = VinilGetBasicType (VINIL_TYPE_FLOAT_VEC4, VinilPrecisionHigh, &Type);
    if (FAILED (Result)) {
      goto cleanup;
    }

    Result = VinilVariableCreate (Type, Name, i, &Variables[i]);
    IVinilType_Release (Type);

    if (FAILED (Result)) {
      goto cleanup;
    }
  }

  /* Add instructions to program */
  for (i = 0; i < InstructionCount; i++) {
    IVinilVariable  *Dst = NULL;
    IVinilVariable  *Src0 = NULL;
    IVinilVariable  *Src1 = NULL;
    IVinilVariable  *Src2 = NULL;

    if (SerializedInst[i].DstId != 0xFFFFFFFF && SerializedInst[i].DstId <= MaxVariableId) {
      Dst = Variables[SerializedInst[i].DstId];
    }
    if (SerializedInst[i].Src0Id != 0xFFFFFFFF && SerializedInst[i].Src0Id <= MaxVariableId) {
      Src0 = Variables[SerializedInst[i].Src0Id];
    }
    if (SerializedInst[i].Src1Id != 0xFFFFFFFF && SerializedInst[i].Src1Id <= MaxVariableId) {
      Src1 = Variables[SerializedInst[i].Src1Id];
    }
    if (SerializedInst[i].Src2Id != 0xFFFFFFFF && SerializedInst[i].Src2Id <= MaxVariableId) {
      Src2 = Variables[SerializedInst[i].Src2Id];
    }

    Result = VinilProgramAddInstruction (Program, SerializedInst[i].Opcode, Dst, Src0, Src1, Src2);
    if (FAILED (Result)) {
      goto cleanup;
    }
  }

  /* Clean up variables array */
  for (i = 0; i <= MaxVariableId; i++) {
    if (Variables[i] != NULL) {
      IVinilVariable_Release (Variables[i]);
    }
  }
  free (Variables);

  IVinilMemoryPool_Release (MemoryPool);
  *ProgramPtr = Program;
  return S_OK;

cleanup:
  for (i = 0; i <= MaxVariableId; i++) {
    if (Variables[i] != NULL) {
      IVinilVariable_Release (Variables[i]);
    }
  }
  free (Variables);
  IVinilProgram_Release (Program);
  IVinilMemoryPool_Release (MemoryPool);
  return Result;
}

//
// File I/O
//

HRESULT
VinilSaveProgram (
  CONST VOID  *Program,
  CONST CHAR8 *FilePath
  )
{
  UINT8   Buffer[65536];
  UINTN   BytesWritten;
  FILE    *File;
  HRESULT Result;

  if (Program == NULL || FilePath == NULL) {
    return E_POINTER;
  }

  /* Serialize to buffer */
  Result = VinilSerializeProgram (Program, Buffer, sizeof (Buffer), &BytesWritten);
  if (FAILED (Result)) {
    return Result;
  }

  /* Open file for writing */
  File = fopen ((const char *)FilePath, "wb");
  if (File == NULL) {
    return E_FAIL;
  }

  /* Write buffer to file */
  if (fwrite (Buffer, 1, BytesWritten, File) != BytesWritten) {
    fclose (File);
    return E_FAIL;
  }

  fclose (File);
  return S_OK;
}

HRESULT
VinilLoadProgram (
  CONST CHAR8 *FilePath,
  VOID        **Program
  )
{
  UINT8   Buffer[65536];
  FILE    *File;
  UINTN   BytesRead;
  HRESULT Result;

  if (FilePath == NULL || Program == NULL) {
    return E_POINTER;
  }

  /* Open file for reading */
  File = fopen ((const char *)FilePath, "rb");
  if (File == NULL) {
    return E_FAIL;
  }

  /* Read file into buffer */
  BytesRead = fread (Buffer, 1, sizeof (Buffer), File);
  fclose (File);

  if (BytesRead == 0) {
    return E_FAIL;
  }

  /* Deserialize from buffer */
  Result = VinilDeserializeProgram (Buffer, BytesRead, Program);
  return Result;
}
