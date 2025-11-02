/*++
    Module Name:

        binary.c

    Abstract:

        VINIL binary serialization format implementation.
        Provides functions to save and load IL programs to/from disk.

    Copyright (C) 2025 NUX Project

    SPDX-License-Identifier:    CDDL-1.0
--*/

#include <vinil/binary.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* --------------------------------------------------------------- */
/*  Validation                                                     */
/* --------------------------------------------------------------- */

HRESULT
VinilValidateBinary (
    CONST VOID  *Buffer,
    UINTN       BufferSize
    )
{
    CONST VINIL_BINARY_HEADER *Header;

    if (Buffer == NULL) {
        return E_POINTER;
    }

    if (BufferSize < sizeof(VINIL_BINARY_HEADER)) {
        return E_FAIL;
    }

    Header = (CONST VINIL_BINARY_HEADER *)Buffer;

    /* Check magic number */
    if (Header->Magic != VINIL_BINARY_MAGIC) {
        return E_FAIL;
    }

    /* Check version */
    if (Header->Version > VINIL_BINARY_VERSION) {
        return E_FAIL;  /* Newer version than we support */
    }

    /* Validate execution mode */
    if (Header->Mode > VinilModeHybrid) {
        return E_FAIL;
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Serialization                                                  */
/* --------------------------------------------------------------- */

HRESULT
VinilSerializeProgram (
    CONST VOID  *Program,
    VOID        *Buffer,
    UINTN       BufferSize,
    UINTN       *BytesWritten
    )
{
    VINIL_BINARY_HEADER     *Header;
    VINIL_SECTION_HEADER    *SectionHdr;
    UINT8                   *DataPtr;
    UINTN                   Offset;
    UINTN                   TotalSize;

    if (Program == NULL || Buffer == NULL) {
        return E_POINTER;
    }

    /* Calculate required size */
    TotalSize = sizeof(VINIL_BINARY_HEADER);
    TotalSize += sizeof(VINIL_SECTION_HEADER);  /* Code section */
    TotalSize += 256;  /* Placeholder for actual code size */

    if (BufferSize < TotalSize) {
        if (BytesWritten != NULL) {
            *BytesWritten = TotalSize;
        }
        return E_OUTOFMEMORY;
    }

    /* Initialize buffer */
    memset(Buffer, 0, BufferSize);
    DataPtr = (UINT8 *)Buffer;
    Offset = 0;

    /* Write file header */
    Header = (VINIL_BINARY_HEADER *)&DataPtr[Offset];
    Header->Magic = VINIL_BINARY_MAGIC;
    Header->Version = VINIL_BINARY_VERSION;
    Header->Mode = VinilModeGraphics;  /* TODO: Extract from program */
    Header->NumSections = 1;
    Header->Flags = 0;
    Offset += sizeof(VINIL_BINARY_HEADER);

    /* Write code section header */
    SectionHdr = (VINIL_SECTION_HEADER *)&DataPtr[Offset];
    SectionHdr->Type = VinilSectionCode;
    SectionHdr->Size = 0;  /* TODO: Calculate actual code size */
    SectionHdr->Offset = (UINT32)Offset + sizeof(VINIL_SECTION_HEADER);
    SectionHdr->Alignment = 4;
    Offset += sizeof(VINIL_SECTION_HEADER);

    /* TODO: Write actual IL instructions */
    /* For now, just write placeholder */

    if (BytesWritten != NULL) {
        *BytesWritten = Offset;
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Deserialization                                                */
/* --------------------------------------------------------------- */

HRESULT
VinilDeserializeProgram (
    CONST VOID  *Buffer,
    UINTN       BufferSize,
    VOID        **Program
    )
{
    CONST VINIL_BINARY_HEADER   *Header;
    CONST VINIL_SECTION_HEADER  *SectionHdr;
    CONST UINT8                 *DataPtr;
    UINTN                       Offset;
    HRESULT                     Hr;
    UINT32                      i;

    if (Buffer == NULL || Program == NULL) {
        return E_POINTER;
    }

    /* Validate binary format */
    Hr = VinilValidateBinary(Buffer, BufferSize);
    if (FAILED(Hr)) {
        return Hr;
    }

    DataPtr = (CONST UINT8 *)Buffer;
    Header = (CONST VINIL_BINARY_HEADER *)DataPtr;
    Offset = sizeof(VINIL_BINARY_HEADER);

    /* Read sections */
    for (i = 0; i < Header->NumSections; i++) {
        if (Offset + sizeof(VINIL_SECTION_HEADER) > BufferSize) {
            return E_FAIL;
        }

        SectionHdr = (CONST VINIL_SECTION_HEADER *)&DataPtr[Offset];
        Offset += sizeof(VINIL_SECTION_HEADER);

        /* Process section based on type */
        switch (SectionHdr->Type) {
        case VinilSectionCode:
            /* TODO: Parse IL instructions */
            break;

        case VinilSectionData:
            /* TODO: Load constant data */
            break;

        case VinilSectionSymbols:
            /* TODO: Load symbol table */
            break;

        default:
            /* Skip unknown sections */
            break;
        }

        Offset += SectionHdr->Size;
    }

    /* TODO: Construct program object */
    *Program = NULL;

    return E_NOTIMPL;
}

/* --------------------------------------------------------------- */
/*  File I/O                                                       */
/* --------------------------------------------------------------- */

HRESULT
VinilSaveProgram (
    CONST VOID  *Program,
    CONST CHAR8 *FilePath
    )
{
    UINT8   Buffer[65536];
    UINTN   BytesWritten;
    FILE    *File;
    HRESULT Hr;

    if (Program == NULL || FilePath == NULL) {
        return E_POINTER;
    }

    /* Serialize to buffer */
    Hr = VinilSerializeProgram(Program, Buffer, sizeof(Buffer), &BytesWritten);
    if (FAILED(Hr)) {
        return Hr;
    }

    /* Open file for writing */
    File = fopen((const char *)FilePath, "wb");
    if (File == NULL) {
        return E_FAIL;
    }

    /* Write buffer to file */
    if (fwrite(Buffer, 1, BytesWritten, File) != BytesWritten) {
        fclose(File);
        return E_FAIL;
    }

    fclose(File);
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
    HRESULT Hr;

    if (FilePath == NULL || Program == NULL) {
        return E_POINTER;
    }

    /* Open file for reading */
    File = fopen((const char *)FilePath, "rb");
    if (File == NULL) {
        return E_FAIL;
    }

    /* Read file into buffer */
    BytesRead = fread(Buffer, 1, sizeof(Buffer), File);
    fclose(File);

    if (BytesRead == 0) {
        return E_FAIL;
    }

    /* Deserialize from buffer */
    Hr = VinilDeserializeProgram(Buffer, BytesRead, Program);
    return Hr;
}
