/** @file
  VINIL Binary Format

  Binary serialization format for storing VINIL IL programs on disk.
  Provides efficient storage and loading of compiled IL programs.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#ifndef __vinil_binary_h__
#define __vinil_binary_h__ 1

#include <ananke/types.h>
#include <ananke/hresult.h>

//
// Binary Format Magic Number and Version
//

#define VINIL_BINARY_MAGIC      0x4C494E56  /* "VINIL" in little-endian */
#define VINIL_BINARY_VERSION    0x00000001  /* Version 1.0 */

//
// Section Types
//

typedef enum _VINIL_SECTION_TYPE {
    VinilSectionHeader      = 0,
    VinilSectionCode        = 1,  /* IL instruction stream */
    VinilSectionData        = 2,  /* Constant data */
    VinilSectionSymbols     = 3,  /* Symbol table */
    VinilSectionTypes       = 4,  /* Type information */
    VinilSectionDebug       = 5,  /* Debug information */
    VinilSectionMetadata    = 6,  /* Program metadata */
} VINIL_SECTION_TYPE;

//
// Execution Mode
//

typedef enum _VINIL_EXEC_MODE {
    VinilModeGraphics   = 0,
    VinilModeCompute    = 1,
    VinilModeHybrid     = 2,
} VINIL_EXEC_MODE;

//
// Binary File Header
//

typedef struct _VINIL_BINARY_HEADER {
    UINT32              Magic;          /* VINIL_BINARY_MAGIC */
    UINT32              Version;        /* Format version */
    VINIL_EXEC_MODE     Mode;           /* Execution mode */
    UINT32              NumSections;    /* Number of sections */
    UINT32              Flags;          /* Compilation flags */
    UINT32              Reserved[3];    /* Reserved for future use */
} VINIL_BINARY_HEADER;

//
// Section Header
//

typedef struct _VINIL_SECTION_HEADER {
    VINIL_SECTION_TYPE  Type;           /* Section type */
    UINT32              Size;           /* Section size in bytes */
    UINT32              Offset;         /* Offset from file start */
    UINT32              Alignment;      /* Required alignment */
} VINIL_SECTION_HEADER;

//
// Serialization Functions
//

/**
  Serialize IL program to binary format.

  @param[in]   Program      IL program to serialize.
  @param[out]  Buffer       Output buffer.
  @param[in]   BufferSize   Size of output buffer.
  @param[out]  BytesWritten Number of bytes written.

  @retval  S_OK           Success.
  @retval  E_POINTER      Invalid pointer.
  @retval  E_OUTOFMEMORY  Buffer too small.
  @retval  E_FAIL         Serialization failed.
**/
HRESULT
VinilSerializeProgram (
    CONST VOID  *Program,
    VOID        *Buffer,
    UINTN       BufferSize,
    UINTN       *BytesWritten
    );

/**
  Deserialize IL program from binary format.

  @param[in]   Buffer       Input buffer containing binary data.
  @param[in]   BufferSize   Size of input buffer.
  @param[out]  Program      Deserialized IL program.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Deserialization failed.
**/
HRESULT
VinilDeserializeProgram (
    CONST VOID  *Buffer,
    UINTN       BufferSize,
    VOID        **Program
    );

/**
  Save IL program to file.

  @param[in]  Program   IL program to save.
  @param[in]  FilePath  Path to output file.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     I/O error.
**/
HRESULT
VinilSaveProgram (
    CONST VOID  *Program,
    CONST CHAR8 *FilePath
    );

/**
  Load IL program from file.

  @param[in]   FilePath  Path to input file.
  @param[out]  Program   Loaded IL program.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     I/O error or invalid format.
**/
HRESULT
VinilLoadProgram (
    CONST CHAR8 *FilePath,
    VOID        **Program
    );

/**
  Validate binary format.

  @param[in]  Buffer      Buffer containing binary data.
  @param[in]  BufferSize  Size of buffer.

  @retval  S_OK      Valid format.
  @retval  E_FAIL    Invalid format.
**/
HRESULT
VinilValidateBinary (
    CONST VOID  *Buffer,
    UINTN       BufferSize
    );

#endif // __vinil_binary_h__
