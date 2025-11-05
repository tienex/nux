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

/* Mach-O 64-bit structures */
#define MH_MAGIC_64     0xfeedfacf
#define MH_OBJECT       0x1
#define CPU_TYPE_X86_64 0x01000007
#define CPU_TYPE_ARM64  0x0100000c
#define LC_SEGMENT_64   0x19
#define LC_SYMTAB       0x2
#define S_ATTR_PURE_INSTRUCTIONS 0x80000000
#define S_ATTR_SOME_INSTRUCTIONS 0x00000400

typedef struct {
    uint32_t magic;
    uint32_t cputype;
    uint32_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
    uint32_t reserved;
} mach_header_64;

typedef struct {
    uint32_t cmd;
    uint32_t cmdsize;
    char segname[16];
    uint64_t vmaddr;
    uint64_t vmsize;
    uint64_t fileoff;
    uint64_t filesize;
    uint32_t maxprot;
    uint32_t initprot;
    uint32_t nsects;
    uint32_t flags;
} segment_command_64;

typedef struct {
    char sectname[16];
    char segname[16];
    uint64_t addr;
    uint64_t size;
    uint32_t offset;
    uint32_t align;
    uint32_t reloff;
    uint32_t nreloc;
    uint32_t flags;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
} section_64;

typedef struct {
    uint32_t cmd;
    uint32_t cmdsize;
    uint32_t symoff;
    uint32_t nsyms;
    uint32_t stroff;
    uint32_t strsize;
} symtab_command;

typedef struct {
    uint32_t n_strx;
    uint8_t n_type;
    uint8_t n_sect;
    uint16_t n_desc;
    uint64_t n_value;
} nlist_64;

/* Generate Mach-O 64-bit object file */
static HRESULT
GenerateMachO64Object (
    CONST VINIL_AOT_TARGET  *Target,
    CONST VOID              *CodeData,
    UINTN                   CodeSize,
    VOID                    **ObjectData,
    UINTN                   *ObjectSize
    )
{
    UINT8 *Buffer;
    UINTN Offset;
    mach_header_64 *Header;
    segment_command_64 *SegCmd;
    section_64 *Sect;
    symtab_command *SymCmd;
    nlist_64 *Sym;
    CONST CHAR8 *StrTable;
    UINTN StrTableSize;
    UINTN SymTableSize;
    UINTN TotalSize;
    uint32_t CpuType;

    /* Determine CPU type */
    switch (Target->Arch) {
        case VinilAotX86_64:
            CpuType = CPU_TYPE_X86_64;
            break;
        case VinilAotARM64:
            CpuType = CPU_TYPE_ARM64;
            break;
        default:
            CpuType = CPU_TYPE_X86_64;
            break;
    }

    StrTable = "\0_vinil_shader\0";
    StrTableSize = 16;
    SymTableSize = sizeof(nlist_64);

    TotalSize = sizeof(mach_header_64) +
                sizeof(segment_command_64) + sizeof(section_64) +
                sizeof(symtab_command) +
                CodeSize +
                SymTableSize +
                StrTableSize;

    Buffer = (UINT8 *)malloc(TotalSize);
    if (Buffer == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(Buffer, 0, TotalSize);
    Offset = 0;

    /* Mach-O header */
    Header = (mach_header_64 *)(Buffer + Offset);
    Header->magic = MH_MAGIC_64;
    Header->cputype = CpuType;
    Header->cpusubtype = 0;
    Header->filetype = MH_OBJECT;
    Header->ncmds = 2;
    Header->sizeofcmds = sizeof(segment_command_64) + sizeof(section_64) + sizeof(symtab_command);
    Header->flags = 0;
    Offset += sizeof(mach_header_64);

    /* Segment command */
    SegCmd = (segment_command_64 *)(Buffer + Offset);
    SegCmd->cmd = LC_SEGMENT_64;
    SegCmd->cmdsize = sizeof(segment_command_64) + sizeof(section_64);
    memset(SegCmd->segname, 0, 16);
    SegCmd->vmaddr = 0;
    SegCmd->vmsize = CodeSize;
    SegCmd->fileoff = sizeof(mach_header_64) + sizeof(segment_command_64) + sizeof(section_64) + sizeof(symtab_command);
    SegCmd->filesize = CodeSize;
    SegCmd->maxprot = 7;
    SegCmd->initprot = 7;
    SegCmd->nsects = 1;
    SegCmd->flags = 0;
    Offset += sizeof(segment_command_64);

    /* Section */
    Sect = (section_64 *)(Buffer + Offset);
    strncpy(Sect->sectname, "__text", 16);
    strncpy(Sect->segname, "__TEXT", 16);
    Sect->addr = 0;
    Sect->size = CodeSize;
    Sect->offset = SegCmd->fileoff;
    Sect->align = 4;
    Sect->reloff = 0;
    Sect->nreloc = 0;
    Sect->flags = S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS;
    Offset += sizeof(section_64);

    /* Symbol table command */
    SymCmd = (symtab_command *)(Buffer + Offset);
    SymCmd->cmd = LC_SYMTAB;
    SymCmd->cmdsize = sizeof(symtab_command);
    SymCmd->symoff = SegCmd->fileoff + CodeSize;
    SymCmd->nsyms = 1;
    SymCmd->stroff = SymCmd->symoff + SymTableSize;
    SymCmd->strsize = StrTableSize;
    Offset += sizeof(symtab_command);

    /* Code */
    memcpy(Buffer + Offset, CodeData, CodeSize);
    Offset += CodeSize;

    /* Symbol table */
    Sym = (nlist_64 *)(Buffer + Offset);
    Sym->n_strx = 1;
    Sym->n_type = 0x0f;
    Sym->n_sect = 1;
    Sym->n_desc = 0;
    Sym->n_value = 0;
    Offset += SymTableSize;

    /* String table */
    memcpy(Buffer + Offset, StrTable, StrTableSize);

    *ObjectData = Buffer;
    *ObjectSize = TotalSize;

    return S_OK;
}

/* COFF structures for Windows */
#define IMAGE_FILE_MACHINE_AMD64 0x8664
#define IMAGE_FILE_MACHINE_ARM64 0xAA64
#define IMAGE_SCN_CNT_CODE 0x00000020
#define IMAGE_SCN_MEM_EXECUTE 0x20000000
#define IMAGE_SCN_MEM_READ 0x40000000
#define IMAGE_SYM_CLASS_EXTERNAL 2
#define IMAGE_SYM_TYPE_FUNC 0x20

typedef struct {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} IMAGE_FILE_HEADER;

typedef struct {
    char Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} IMAGE_SECTION_HEADER;

typedef struct {
    union {
        char ShortName[8];
        struct {
            uint32_t Zeros;
            uint32_t Offset;
        } Name;
    } N;
    uint32_t Value;
    int16_t SectionNumber;
    uint16_t Type;
    uint8_t StorageClass;
    uint8_t NumberOfAuxSymbols;
} IMAGE_SYMBOL;

/* Generate COFF object file */
static HRESULT
GenerateCOFFObject (
    CONST VINIL_AOT_TARGET  *Target,
    CONST VOID              *CodeData,
    UINTN                   CodeSize,
    VOID                    **ObjectData,
    UINTN                   *ObjectSize
    )
{
    UINT8 *Buffer;
    UINTN Offset;
    IMAGE_FILE_HEADER *FileHdr;
    IMAGE_SECTION_HEADER *SectHdr;
    IMAGE_SYMBOL *Sym;
    CONST CHAR8 *StrTable;
    UINTN StrTableSize;
    UINTN TotalSize;
    uint16_t Machine;

    /* Determine machine type */
    switch (Target->Arch) {
        case VinilAotX86_64:
            Machine = IMAGE_FILE_MACHINE_AMD64;
            break;
        case VinilAotARM64:
            Machine = IMAGE_FILE_MACHINE_ARM64;
            break;
        default:
            Machine = IMAGE_FILE_MACHINE_AMD64;
            break;
    }

    StrTable = "\x0e\x00\x00\x00vinil_shader\0";
    StrTableSize = 4 + 13;

    TotalSize = sizeof(IMAGE_FILE_HEADER) +
                sizeof(IMAGE_SECTION_HEADER) +
                CodeSize +
                sizeof(IMAGE_SYMBOL) +
                StrTableSize;

    Buffer = (UINT8 *)malloc(TotalSize);
    if (Buffer == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(Buffer, 0, TotalSize);
    Offset = 0;

    /* File header */
    FileHdr = (IMAGE_FILE_HEADER *)(Buffer + Offset);
    FileHdr->Machine = Machine;
    FileHdr->NumberOfSections = 1;
    FileHdr->TimeDateStamp = 0;
    FileHdr->PointerToSymbolTable = sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_SECTION_HEADER) + CodeSize;
    FileHdr->NumberOfSymbols = 1;
    FileHdr->SizeOfOptionalHeader = 0;
    FileHdr->Characteristics = 0;
    Offset += sizeof(IMAGE_FILE_HEADER);

    /* Section header */
    SectHdr = (IMAGE_SECTION_HEADER *)(Buffer + Offset);
    strncpy(SectHdr->Name, ".text", 8);
    SectHdr->VirtualSize = 0;
    SectHdr->VirtualAddress = 0;
    SectHdr->SizeOfRawData = CodeSize;
    SectHdr->PointerToRawData = sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_SECTION_HEADER);
    SectHdr->PointerToRelocations = 0;
    SectHdr->PointerToLinenumbers = 0;
    SectHdr->NumberOfRelocations = 0;
    SectHdr->NumberOfLinenumbers = 0;
    SectHdr->Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
    Offset += sizeof(IMAGE_SECTION_HEADER);

    /* Code */
    memcpy(Buffer + Offset, CodeData, CodeSize);
    Offset += CodeSize;

    /* Symbol table */
    Sym = (IMAGE_SYMBOL *)(Buffer + Offset);
    Sym->N.Name.Zeros = 0;
    Sym->N.Name.Offset = 4;
    Sym->Value = 0;
    Sym->SectionNumber = 1;
    Sym->Type = IMAGE_SYM_TYPE_FUNC;
    Sym->StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    Sym->NumberOfAuxSymbols = 0;
    Offset += sizeof(IMAGE_SYMBOL);

    /* String table */
    memcpy(Buffer + Offset, StrTable, StrTableSize);

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
            Hr = GenerateMachO64Object(Target, CodeData, CodeSize, ObjectData, ObjectSize);
            break;

        case VinilAotCOFF:
            Hr = GenerateCOFFObject(Target, CodeData, CodeSize, ObjectData, ObjectSize);
            break;

        case VinilAotWasm:
            /* WebAssembly binary format not yet implemented */
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
