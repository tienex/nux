/*
 * ELF handler for objlayout tool
 * Adds custom program headers and sections to ELF files
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ELF definitions */
#define EI_NIDENT 16

#define ET_EXEC 2
#define PT_LOAD 1
#define PT_TLS 7

/* APXH-specific program header types */
#define PT_APXH_INFO        0xAF100000
#define PT_APXH_PHYSMAP     0xAF100002
#define PT_APXH_BATREE      0xAF100004
#define PT_APXH_PFNCACHE    0xAF100005
#define PT_APXH_FBUF        0xAF100006
#define PT_APXH_REGIONS     0xAF100007
#define PT_APXH_TOPPGTALLOC 0xAF100008
#define PT_APXH_LINEAR      0xAF10FFFF

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
} Elf32_Ehdr_Start;

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} Elf64_Shdr;

typedef struct {
    void *data;
    size_t size;
} obj_file_t;

static int is_elf64(void *data)
{
    unsigned char *ident = (unsigned char *)data;
    return ident[4] == 2;  /* ELFCLASS64 */
}

static void print_elf_info(void *data)
{
    unsigned char *ident = (unsigned char *)data;

    printf("  Class: %s\n", ident[4] == 1 ? "ELF32" : "ELF64");
    printf("  Data: %s\n", ident[5] == 1 ? "Little-endian" : "Big-endian");
    printf("  Version: %d\n", ident[6]);

    if (is_elf64(data)) {
        Elf64_Ehdr *ehdr = (Elf64_Ehdr *)data;
        printf("  Type: %d\n", ehdr->e_type);
        printf("  Machine: %d\n", ehdr->e_machine);
        printf("  Entry point: 0x%lx\n", ehdr->e_entry);
        printf("  Program headers: %d (offset 0x%lx)\n", ehdr->e_phnum, ehdr->e_phoff);
        printf("  Section headers: %d (offset 0x%lx)\n", ehdr->e_shnum, ehdr->e_shoff);
    }
}

static int add_apxh_program_headers(obj_file_t *obj)
{
    /*
     * This function would add custom APXH program headers to the ELF file
     * For now, we just verify the existing headers
     */
    if (!is_elf64(obj->data)) {
        fprintf(stderr, "Warning: Only ELF64 fully supported currently\n");
        return 0;
    }

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)obj->data;
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uint8_t *)obj->data + ehdr->e_phoff);

    printf("  Analyzing %d program headers:\n", ehdr->e_phnum);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        printf("    [%d] type=0x%08x vaddr=0x%016lx memsz=0x%lx\n",
               i, phdr[i].p_type, phdr[i].p_vaddr, phdr[i].p_memsz);

        /* Check for APXH headers */
        if ((phdr[i].p_type & 0xAF100000) == 0xAF100000) {
            printf("      -> APXH custom header\n");
        }
    }

    return 0;
}

int handle_elf(obj_file_t *obj)
{
    print_elf_info(obj->data);
    add_apxh_program_headers(obj);

    printf("  ELF processing complete\n");
    return 0;
}
