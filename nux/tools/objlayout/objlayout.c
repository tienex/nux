/*
 * objlayout - Object file layout tool
 * Manipulates ELF/PE/Mach-O binaries to add custom sections and headers
 * Replaces linker script functionality for compiler portability
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define ELF_MAGIC 0x464C457F  /* "\x7FELF" */
#define PE_MAGIC 0x00004550   /* "PE\0\0" */
#define MACHO_MAGIC_32 0xFEEDFACE
#define MACHO_MAGIC_64 0xFEEDFACF
#define MACHO_FAT_MAGIC 0xCAFEBABE

typedef enum {
    FORMAT_UNKNOWN,
    FORMAT_ELF,
    FORMAT_PE,
    FORMAT_MACHO
} obj_format_t;

typedef struct {
    void *data;
    size_t size;
    obj_format_t format;
    int fd;
} obj_file_t;

/* Forward declarations */
int handle_elf(obj_file_t *obj);
int handle_pe(obj_file_t *obj);
int handle_macho(obj_file_t *obj);

static obj_format_t detect_format(void *data, size_t size)
{
    if (size < 4) {
        return FORMAT_UNKNOWN;
    }

    uint32_t magic = *(uint32_t *)data;

    if (magic == ELF_MAGIC) {
        return FORMAT_ELF;
    } else if (magic == MACHO_MAGIC_32 || magic == MACHO_MAGIC_64 ||
               magic == MACHO_FAT_MAGIC) {
        return FORMAT_MACHO;
    }

    /* Check for PE signature at offset 0x3C */
    if (size >= 0x40) {
        uint32_t pe_offset = *(uint32_t *)((uint8_t *)data + 0x3C);
        if (pe_offset < size - 4) {
            uint32_t pe_sig = *(uint32_t *)((uint8_t *)data + pe_offset);
            if (pe_sig == PE_MAGIC) {
                return FORMAT_PE;
            }
        }
    }

    return FORMAT_UNKNOWN;
}

static int load_object_file(const char *filename, obj_file_t *obj)
{
    struct stat st;
    int fd;
    void *data;

    fd = open(filename, O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return -1;
    }

    data = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }

    obj->data = data;
    obj->size = st.st_size;
    obj->fd = fd;
    obj->format = detect_format(data, st.st_size);

    return 0;
}

static void unload_object_file(obj_file_t *obj)
{
    if (obj->data) {
        munmap(obj->data, obj->size);
    }
    if (obj->fd >= 0) {
        close(obj->fd);
    }
}

static void print_usage(const char *progname)
{
    fprintf(stderr, "Usage: %s [options] <object-file>\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -f <format>    Force format: elf, pe, macho\n");
    fprintf(stderr, "  -a <arch>      Target architecture\n");
    fprintf(stderr, "  -s <section>   Add custom section\n");
    fprintf(stderr, "  -p <phdr>      Add custom program header (ELF)\n");
    fprintf(stderr, "  -h             Show this help\n");
}

int main(int argc, char **argv)
{
    obj_file_t obj = {0};
    const char *filename = NULL;
    int opt;
    int ret = 0;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    /* Parse command line options */
    while ((opt = getopt(argc, argv, "f:a:s:p:h")) != -1) {
        switch (opt) {
        case 'f':
            /* Force format */
            break;
        case 'a':
            /* Architecture */
            break;
        case 's':
            /* Add section */
            break;
        case 'p':
            /* Add program header */
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Error: No object file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    filename = argv[optind];

    /* Load object file */
    if (load_object_file(filename, &obj) < 0) {
        fprintf(stderr, "Error: Failed to load object file: %s\n", filename);
        return 1;
    }

    printf("Detected format: ");
    switch (obj.format) {
    case FORMAT_ELF:
        printf("ELF\n");
        ret = handle_elf(&obj);
        break;
    case FORMAT_PE:
        printf("PE/COFF\n");
        ret = handle_pe(&obj);
        break;
    case FORMAT_MACHO:
        printf("Mach-O\n");
        ret = handle_macho(&obj);
        break;
    default:
        printf("Unknown\n");
        fprintf(stderr, "Error: Unsupported object file format\n");
        ret = 1;
        break;
    }

    /* Unload object file */
    unload_object_file(&obj);

    return ret;
}
