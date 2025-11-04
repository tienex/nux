/*++
    Module Name:

        aot.c

    Abstract:

        VINIL AOT compiler - generates native code and packages as object files.
        Uses SLJIT for code generation and implements ELF64 object file format.

    Copyright (C) 2025 NUX Project

    SPDX-License-Identifier:    CDDL-1.0
--*/

#define COBJMACROS
#include <vinil/aot.h>
#include <vinil/binary.h>
#include <vinil/vinil.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vinil_internal.h"

/* ELF64 structures */
#define EI_NIDENT 16

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    uint64_t      e_entry;
    uint64_t      e_phoff;
    uint64_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t   sh_name;
    uint32_t   sh_type;
    uint64_t   sh_flags;
    uint64_t   sh_addr;
    uint64_t   sh_offset;
    uint64_t   sh_size;
    uint32_t   sh_link;
    uint32_t   sh_info;
    uint64_t   sh_addralign;
    uint64_t   sh_entsize;
} Elf64_Shdr;

typedef struct {
    uint32_t      st_name;
    unsigned char st_info;
    unsigned char st_other;
    uint16_t      st_shndx;
    uint64_t      st_value;
    uint64_t      st_size;
} Elf64_Sym;

/* ELF constants */
#define ELFMAG0    0x7f
#define ELFMAG1    'E'
#define ELFMAG2    'L'
#define ELFMAG3    'F'
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define EV_CURRENT  1
#define ET_REL      1
#define EM_X86_64   62
#define EM_AARCH64  183
#define SHT_NULL    0
#define SHT_PROGBITS 1
#define SHT_SYMTAB  2
#define SHT_STRTAB  3
#define SHF_WRITE   0x1
#define SHF_ALLOC   0x2
#define SHF_EXECINSTR 0x4
#define STB_GLOBAL  1
#define STT_FUNC    2
#define STV_DEFAULT 0

/* Architecture names */
static CONST CHAR8 *gArchNames[] = {
    (CONST CHAR8 *)"x86",
    (CONST CHAR8 *)"x86-64",
    (CONST CHAR8 *)"arm",
    (CONST CHAR8 *)"arm64",
    (CONST CHAR8 *)"riscv32",
    (CONST CHAR8 *)"riscv64",
    (CONST CHAR8 *)"powerpc",
    (CONST CHAR8 *)"powerpc64",
    (CONST CHAR8 *)"mips",
    (CONST CHAR8 *)"mips64",
};

static CONST CHAR8 *gFormatNames[] = {
    (CONST CHAR8 *)"ELF",
    (CONST CHAR8 *)"Mach-O",
    (CONST CHAR8 *)"PE/COFF",
    (CONST CHAR8 *)"WebAssembly",
};

CONST CHAR8 *
VinilGetArchName (
    VINIL_AOT_ARCH  Arch
    )
{
    if (Arch < (sizeof(gArchNames) / sizeof(gArchNames[0]))) {
        return gArchNames[Arch];
    }
    return (CONST CHAR8 *)"Unknown";
}

CONST CHAR8 *
VinilGetFormatName (
    VINIL_AOT_FORMAT  Format
    )
{
    if (Format < (sizeof(gFormatNames) / sizeof(gFormatNames[0]))) {
        return gFormatNames[Format];
    }
    return (CONST CHAR8 *)"Unknown";
}

HRESULT
VinilGetDefaultTarget (
    VINIL_AOT_TARGET  *Target
    )
{
    if (Target == NULL) {
        return E_POINTER;
    }

    memset(Target, 0, sizeof(VINIL_AOT_TARGET));

    /* Detect current platform */
#if defined(__x86_64__) || defined(_M_X64)
    Target->Arch = VinilAotX86_64;
    Target->Triple = (CONST CHAR8 *)"x86_64-pc-linux-gnu";
#elif defined(__i386__) || defined(_M_IX86)
    Target->Arch = VinilAotX86;
    Target->Triple = (CONST CHAR8 *)"i686-pc-linux-gnu";
#elif defined(__aarch64__)
    Target->Arch = VinilAotARM64;
    Target->Triple = (CONST CHAR8 *)"aarch64-unknown-linux-gnu";
#elif defined(__arm__)
    Target->Arch = VinilAotARM;
    Target->Triple = (CONST CHAR8 *)"arm-unknown-linux-gnueabihf";
#elif defined(__riscv) && (__riscv_xlen == 64)
    Target->Arch = VinilAotRISCV64;
    Target->Triple = (CONST CHAR8 *)"riscv64-unknown-linux-gnu";
#elif defined(__riscv) && (__riscv_xlen == 32)
    Target->Arch = VinilAotRISCV32;
    Target->Triple = (CONST CHAR8 *)"riscv32-unknown-linux-gnu";
#else
    Target->Arch = VinilAotX86_64;
    Target->Triple = (CONST CHAR8 *)"unknown";
#endif

    /* Detect object format */
#if defined(__linux__) || defined(__FreeBSD__)
    Target->Format = VinilAotELF;
#elif defined(__APPLE__)
    Target->Format = VinilAotMachO;
#elif defined(_WIN32)
    Target->Format = VinilAotCOFF;
#else
    Target->Format = VinilAotELF;
#endif

    Target->OptLevel = VinilAotOptSpeed;
    Target->Flags = VinilAotNone;
    Target->CPU = (CONST CHAR8 *)"generic";
    Target->Features = (CONST CHAR8 *)"";

    return S_OK;
}

HRESULT
VinilGetSupportedArchitectures (
    CONST VINIL_AOT_ARCH  **Architectures,
    UINTN                 *Count
    )
{
    static CONST VINIL_AOT_ARCH Archs[] = {
        VinilAotX86,
        VinilAotX86_64,
        VinilAotARM,
        VinilAotARM64,
        VinilAotRISCV32,
        VinilAotRISCV64,
    };

    if (Architectures == NULL || Count == NULL) {
        return E_POINTER;
    }

    *Architectures = Archs;
    *Count = sizeof(Archs) / sizeof(Archs[0]);

    return S_OK;
}

/* Generate ELF64 object file */
static HRESULT
GenerateELF64Object (
    CONST VINIL_AOT_TARGET  *Target,
    CONST VOID              *CodeData,
    UINTN                   CodeSize,
    VOID                    **ObjectData,
    UINTN                   *ObjectSize
    )
{
    UINT8           *Buffer;
    UINTN           Offset;
    Elf64_Ehdr      *Ehdr;
    Elf64_Shdr      *Shdr;
    Elf64_Sym       *Sym;
    CONST CHAR8     *StringTable;
    UINTN           TotalSize;
    UINT16          Machine;

    /* Determine ELF machine type */
    switch (Target->Arch) {
        case VinilAotX86_64:
            Machine = EM_X86_64;
            break;
        case VinilAotARM64:
            Machine = EM_AARCH64;
            break;
        default:
            Machine = EM_X86_64;
            break;
    }

    /* Calculate sizes */
    /* ELF header + 5 section headers + string table + symbol table + code */
    StringTable = "\0.text\0.symtab\0.strtab\0.shstrtab\0vinil_shader\0";
    UINTN StringTableSize = strlen((const char *)StringTable) + 1;
    UINTN SymbolTableSize = 2 * sizeof(Elf64_Sym);  /* NULL + vinil_shader */

    TotalSize = sizeof(Elf64_Ehdr) +
                5 * sizeof(Elf64_Shdr) +  /* NULL, .text, .symtab, .strtab, .shstrtab */
                StringTableSize +
                SymbolTableSize +
                CodeSize;

    Buffer = (UINT8 *)malloc(TotalSize);
    if (Buffer == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(Buffer, 0, TotalSize);
    Offset = 0;

    /* ELF header */
    Ehdr = (Elf64_Ehdr *)(Buffer + Offset);
    Ehdr->e_ident[0] = ELFMAG0;
    Ehdr->e_ident[1] = ELFMAG1;
    Ehdr->e_ident[2] = ELFMAG2;
    Ehdr->e_ident[3] = ELFMAG3;
    Ehdr->e_ident[4] = ELFCLASS64;
    Ehdr->e_ident[5] = ELFDATA2LSB;
    Ehdr->e_ident[6] = EV_CURRENT;
    Ehdr->e_type = ET_REL;
    Ehdr->e_machine = Machine;
    Ehdr->e_version = EV_CURRENT;
    Ehdr->e_ehsize = sizeof(Elf64_Ehdr);
    Ehdr->e_shentsize = sizeof(Elf64_Shdr);
    Ehdr->e_shnum = 5;
    Ehdr->e_shstrndx = 4;  /* .shstrtab */
    Ehdr->e_shoff = sizeof(Elf64_Ehdr);
    Offset += sizeof(Elf64_Ehdr);

    /* Section headers */
    /* Section 0: NULL */
    Shdr = (Elf64_Shdr *)(Buffer + Offset);
    Offset += sizeof(Elf64_Shdr);

    /* Section 1: .text */
    Shdr = (Elf64_Shdr *)(Buffer + Offset);
    Shdr->sh_name = 1;  /* ".text" in string table */
    Shdr->sh_type = SHT_PROGBITS;
    Shdr->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    Shdr->sh_addr = 0;
    Shdr->sh_offset = sizeof(Elf64_Ehdr) + 5 * sizeof(Elf64_Shdr);
    Shdr->sh_size = CodeSize;
    Shdr->sh_addralign = 16;
    Offset += sizeof(Elf64_Shdr);

    /* Section 2: .symtab */
    Shdr = (Elf64_Shdr *)(Buffer + Offset);
    Shdr->sh_name = 7;  /* ".symtab" */
    Shdr->sh_type = SHT_SYMTAB;
    Shdr->sh_offset = sizeof(Elf64_Ehdr) + 5 * sizeof(Elf64_Shdr) + CodeSize;
    Shdr->sh_size = SymbolTableSize;
    Shdr->sh_link = 3;  /* .strtab */
    Shdr->sh_info = 1;  /* Number of local symbols */
    Shdr->sh_addralign = 8;
    Shdr->sh_entsize = sizeof(Elf64_Sym);
    Offset += sizeof(Elf64_Shdr);

    /* Section 3: .strtab */
    Shdr = (Elf64_Shdr *)(Buffer + Offset);
    Shdr->sh_name = 15;  /* ".strtab" */
    Shdr->sh_type = SHT_STRTAB;
    Shdr->sh_offset = sizeof(Elf64_Ehdr) + 5 * sizeof(Elf64_Shdr) + CodeSize + SymbolTableSize;
    Shdr->sh_size = StringTableSize;
    Shdr->sh_addralign = 1;
    Offset += sizeof(Elf64_Shdr);

    /* Section 4: .shstrtab */
    Shdr = (Elf64_Shdr *)(Buffer + Offset);
    Shdr->sh_name = 23;  /* ".shstrtab" */
    Shdr->sh_type = SHT_STRTAB;
    Shdr->sh_offset = sizeof(Elf64_Ehdr) + 5 * sizeof(Elf64_Shdr) + CodeSize + SymbolTableSize;
    Shdr->sh_size = StringTableSize;
    Shdr->sh_addralign = 1;
    Offset += sizeof(Elf64_Shdr);

    /* Code section */
    memcpy(Buffer + Offset, CodeData, CodeSize);
    Offset += CodeSize;

    /* Symbol table */
    /* Symbol 0: NULL */
    Sym = (Elf64_Sym *)(Buffer + Offset);
    Offset += sizeof(Elf64_Sym);

    /* Symbol 1: vinil_shader */
    Sym = (Elf64_Sym *)(Buffer + Offset);
    Sym->st_name = 33;  /* "vinil_shader" in string table */
    Sym->st_info = (STB_GLOBAL << 4) | STT_FUNC;
    Sym->st_other = STV_DEFAULT;
    Sym->st_shndx = 1;  /* .text section */
    Sym->st_value = 0;
    Sym->st_size = CodeSize;
    Offset += sizeof(Elf64_Sym);

    /* String table (same as section header string table) */
    memcpy(Buffer + Offset, StringTable, StringTableSize);

    *ObjectData = Buffer;
    *ObjectSize = TotalSize;

    return S_OK;
}

/* Forward declaration - implemented in jit.c */
extern HRESULT VinilJitCompileProgram(IVinilProgram *Program, VOID **Code, UINTN *CodeSize);

HRESULT
VinilCompileAOT (
    CONST VOID              *Program,
    CONST VINIL_AOT_TARGET  *Target,
    VOID                    **ObjectData,
    UINTN                   *ObjectSize
    )
{
    VOID    *CodeData;
    UINTN   CodeSize;
    HRESULT Hr;

    if (Program == NULL || Target == NULL || ObjectData == NULL || ObjectSize == NULL) {
        return E_POINTER;
    }

    /* Compile IL to native code using JIT compiler */
    Hr = VinilJitCompileProgram((IVinilProgram *)Program, &CodeData, &CodeSize);
    if (FAILED(Hr)) {
        return Hr;
    }

    /* Package code based on target format */
    switch (Target->Format) {
        case VinilAotELF:
            Hr = GenerateELF64Object(Target, CodeData, CodeSize, ObjectData, ObjectSize);
            break;

        case VinilAotMachO:
        case VinilAotCOFF:
        case VinilAotWasm:
            /* For now, only ELF is fully implemented */
            /* Return raw code with a note that it's in memory format */
            *ObjectData = malloc(CodeSize);
            if (*ObjectData == NULL) {
                return E_OUTOFMEMORY;
            }
            memcpy(*ObjectData, CodeData, CodeSize);
            *ObjectSize = CodeSize;
            Hr = S_OK;
            break;

        default:
            Hr = E_INVALIDARG;
            break;
    }

    return Hr;
}

HRESULT
VinilCompileAOTFile (
    CONST VOID              *Program,
    CONST VINIL_AOT_TARGET  *Target,
    CONST CHAR8             *OutputPath
    )
{
    VOID    *ObjectData;
    UINTN   ObjectSize;
    FILE    *File;
    HRESULT Hr;

    if (Program == NULL || Target == NULL || OutputPath == NULL) {
        return E_POINTER;
    }

    /* Compile to memory */
    Hr = VinilCompileAOT(Program, Target, &ObjectData, &ObjectSize);
    if (FAILED(Hr)) {
        return Hr;
    }

    /* Write to file */
    File = fopen((const char *)OutputPath, "wb");
    if (File == NULL) {
        free(ObjectData);
        return E_FAIL;
    }

    if (fwrite(ObjectData, 1, ObjectSize, File) != ObjectSize) {
        fclose(File);
        free(ObjectData);
        return E_FAIL;
    }

    fclose(File);
    free(ObjectData);

    return S_OK;
}

HRESULT
VinilCompileBinaryToObject (
    CONST CHAR8             *BinaryPath,
    CONST VINIL_AOT_TARGET  *Target,
    CONST CHAR8             *OutputPath
    )
{
    VOID    *Program;
    HRESULT Hr;

    if (BinaryPath == NULL || Target == NULL || OutputPath == NULL) {
        return E_POINTER;
    }

    /* Load IL binary */
    Hr = VinilLoadProgram(BinaryPath, &Program);
    if (FAILED(Hr)) {
        return Hr;
    }

    /* Compile to object file */
    Hr = VinilCompileAOTFile(Program, Target, OutputPath);

    /* Free program */
    if (Program != NULL) {
        IVinilProgram_Release((IVinilProgram *)Program);
    }

    return Hr;
}
