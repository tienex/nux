/**
 * x86-64 Pseudo-Big-Endian Test Kernel
 *
 * Demonstrates pseudo-endian boot where a little-endian x86-64 bootloader
 * boots a "pseudo-big-endian" x86-64 kernel. The CPU remains in native
 * little-endian mode, but boot structures are provided in big-endian format.
 *
 * This simulates cross-endian boot on architectures that don't natively
 * support multiple endianness modes.
 *
 * Build with:
 *   x86_64-linux-gnu-gcc -m64 -nostdlib -static -ffreestanding \
 *     -Wl,-T,x86_64-pseudo-be.lds -o x86_64-pseudo-be-kernel.elf \
 *     x86_64-pseudo-be-kernel.c x86_64-start.S
 *
 * To mark ELF as big-endian (for bootloader detection):
 *   After building, patch ELF header byte 5 (EI_DATA) to 2 (ELFDATA2MSB)
 */

#include <stdint.h>

// APXH Boot Info Structures
#define APXH_BOOTINFO_MAGIC  0x4150584842494E46ULL  // "APXHBINF"

typedef struct {
    uint64_t InitializedDataVaddr;
    uint64_t InitializedDataSize;
    uint64_t TotalSize;
} APXH_TLS_INFO;

typedef struct {
    uint64_t Magic;
    uint64_t MaxPfn;
    uint64_t MaxRamPfn;
    uint64_t NumRegions;
    uint64_t UserEntry;

    // Framebuffer (simplified)
    uint8_t  FramebufferData[64];

    // Platform descriptor (simplified)
    uint8_t  PlatformData[16];

    APXH_TLS_INFO KernelTls;
    APXH_TLS_INFO UserTls;

    // Architecture and endianness information (UINT8 - endian-safe)
    uint8_t  KernelArchitecture;    // ArchAmd64 = 3
    uint8_t  UserArchitecture;
    uint8_t  HostArchitecture;
    uint8_t  KernelEndianness;      // ImgEndianBig = 2 (pseudo)
    uint8_t  UserEndianness;
    uint8_t  MixedEndian;
    uint16_t Reserved1;

    uint32_t MixedModeFlags;
} APXH_BOOT_INFO;

typedef struct {
    uint32_t Type;
    uint64_t Pfn;
    uint64_t Length;
} APXH_REGION;

// VGA text mode buffer
#define VGA_BUFFER  0xB8000
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_COLOR   0x0F  // White on black

// Architecture enum values
#define ArchAmd64  3

// Endianness enum values
#define ImgEndianUnknown  0
#define ImgEndianLittle   1
#define ImgEndianBig      2

// Global pointer to boot info (passed from bootloader)
static APXH_BOOT_INFO *gBootInfo = NULL;
static APXH_REGION *gMemRegions = NULL;
static int gVgaRow = 0;
static int gVgaCol = 0;

// Byte-swap utilities (for pseudo-BE mode)
static inline uint16_t bswap16(uint16_t x) {
    return ((x & 0xFF) << 8) | ((x >> 8) & 0xFF);
}

static inline uint32_t bswap32(uint32_t x) {
    return ((x & 0xFF) << 24) |
           ((x & 0xFF00) << 8) |
           ((x & 0xFF0000) >> 8) |
           ((x >> 24) & 0xFF);
}

static inline uint64_t bswap64(uint64_t x) {
    return ((x & 0xFFULL) << 56) |
           ((x & 0xFF00ULL) << 40) |
           ((x & 0xFF0000ULL) << 24) |
           ((x & 0xFF000000ULL) << 8) |
           ((x & 0xFF00000000ULL) >> 8) |
           ((x & 0xFF0000000000ULL) >> 24) |
           ((x & 0xFF000000000000ULL) >> 40) |
           ((x >> 56) & 0xFFULL);
}

// VGA text output
static void vga_putc(char c) {
    volatile uint16_t *vga = (volatile uint16_t*)VGA_BUFFER;

    if (c == '\n') {
        gVgaCol = 0;
        gVgaRow++;
        if (gVgaRow >= VGA_HEIGHT) gVgaRow = 0;
        return;
    }

    vga[gVgaRow * VGA_WIDTH + gVgaCol] = (VGA_COLOR << 8) | c;
    gVgaCol++;

    if (gVgaCol >= VGA_WIDTH) {
        gVgaCol = 0;
        gVgaRow++;
        if (gVgaRow >= VGA_HEIGHT) gVgaRow = 0;
    }
}

static void vga_puts(const char *s) {
    while (*s) {
        vga_putc(*s++);
    }
}

static void vga_clear(void) {
    volatile uint16_t *vga = (volatile uint16_t*)VGA_BUFFER;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = (VGA_COLOR << 8) | ' ';
    }
    gVgaRow = 0;
    gVgaCol = 0;
}

// Hex printing
static void print_hex64(uint64_t val) {
    const char *hex = "0123456789ABCDEF";
    vga_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        vga_putc(hex[(val >> i) & 0xF]);
    }
}

static void print_hex32(uint32_t val) {
    const char *hex = "0123456789ABCDEF";
    vga_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        vga_putc(hex[(val >> i) & 0xF]);
    }
}

static void print_dec(uint64_t val) {
    char buf[32];
    int i = 0;

    if (val == 0) {
        vga_putc('0');
        return;
    }

    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }

    while (i > 0) {
        vga_putc(buf[--i]);
    }
}

// Get native endianness (always little-endian on x86)
static const char* get_native_endian_name(void) {
    uint32_t test = 0x12345678;
    uint8_t *p = (uint8_t*)&test;

    if (p[0] == 0x12) return "big-endian (HW)";
    if (p[0] == 0x78) return "little-endian (HW)";
    return "unknown";
}

// Verify boot info magic number (boot info is in big-endian)
static int verify_magic(void) {
    vga_puts("Verifying boot info magic number...\n");

    // Boot info is in big-endian, CPU is little-endian
    // We need to byte-swap to read it
    uint64_t magic_be = gBootInfo->Magic;
    uint64_t magic_le = bswap64(magic_be);

    vga_puts("  Expected (LE):    ");
    print_hex64(APXH_BOOTINFO_MAGIC);
    vga_puts("\n  Got (BE):         ");
    print_hex64(magic_be);
    vga_puts("\n  Got (swapped LE): ");
    print_hex64(magic_le);
    vga_puts("\n");

    if (magic_le == APXH_BOOTINFO_MAGIC) {
        vga_puts("  [OK] Magic number matches after byte-swap!\n");
        return 1;
    } else {
        vga_puts("  [FAIL] Magic number mismatch!\n");
        return 0;
    }
}

// Verify endianness fields (UINT8 - no swapping needed)
static int verify_endianness(void) {
    vga_puts("\nVerifying endianness fields...\n");

    vga_puts("  Kernel endianness: ");
    print_dec(gBootInfo->KernelEndianness);
    if (gBootInfo->KernelEndianness == ImgEndianBig) {
        vga_puts(" (big-endian/pseudo) [OK]\n");
    } else {
        vga_puts(" [FAIL - expected big-endian]\n");
        return 0;
    }

    vga_puts("  User endianness:   ");
    print_dec(gBootInfo->UserEndianness);
    switch (gBootInfo->UserEndianness) {
        case ImgEndianUnknown:
            vga_puts(" (unknown/no-user) [OK]\n");
            break;
        case ImgEndianLittle:
            vga_puts(" (little-endian)\n");
            break;
        case ImgEndianBig:
            vga_puts(" (big-endian)\n");
            break;
    }

    vga_puts("  Mixed-endian:      ");
    print_dec(gBootInfo->MixedEndian);
    if (gBootInfo->MixedEndian) {
        vga_puts(" (TRUE)\n");
    } else {
        vga_puts(" (FALSE)\n");
    }

    return 1;
}

// Verify architecture fields (UINT8 - no swapping needed)
static int verify_architecture(void) {
    vga_puts("\nVerifying architecture fields...\n");

    vga_puts("  Kernel architecture: ");
    print_dec(gBootInfo->KernelArchitecture);
    if (gBootInfo->KernelArchitecture == ArchAmd64) {
        vga_puts(" (AMD64) [OK]\n");
    } else {
        vga_puts(" [FAIL - expected AMD64]\n");
        return 0;
    }

    vga_puts("  Host architecture:   ");
    print_dec(gBootInfo->HostArchitecture);
    vga_puts("\n");

    return 1;
}

// Verify memory regions (need byte-swapping)
static int verify_memory_regions(void) {
    vga_puts("\nVerifying memory regions...\n");

    uint64_t num_regions_be = gBootInfo->NumRegions;
    uint64_t num_regions_le = bswap64(num_regions_be);

    vga_puts("  Number of regions: ");
    print_dec(num_regions_le);
    vga_puts("\n");

    if (num_regions_le == 0 || num_regions_le > 1000) {
        vga_puts("  [FAIL] Invalid region count!\n");
        return 0;
    }

    // Check first region (need to byte-swap all fields)
    uint32_t type_be = gMemRegions[0].Type;
    uint64_t pfn_be = gMemRegions[0].Pfn;
    uint64_t length_be = gMemRegions[0].Length;

    uint32_t type_le = bswap32(type_be);
    uint64_t pfn_le = bswap64(pfn_be);
    uint64_t length_le = bswap64(length_be);

    vga_puts("  First region (after swap):\n");
    vga_puts("    Type:   ");
    print_dec(type_le);
    vga_puts("\n");
    vga_puts("    PFN:    ");
    print_hex64(pfn_le);
    vga_puts("\n");
    vga_puts("    Length: ");
    print_dec(length_le);
    vga_puts("\n");

    if (type_le > 20 || length_le == 0) {
        vga_puts("  [FAIL] Region data corrupted!\n");
        return 0;
    }

    vga_puts("  [OK] Memory regions valid!\n");
    return 1;
}

// Kernel entry point (called from assembly startup)
void kernel_main(uint64_t boot_info_addr, uint64_t regions_addr) {
    gBootInfo = (APXH_BOOT_INFO*)boot_info_addr;
    gMemRegions = (APXH_REGION*)regions_addr;

    vga_clear();

    vga_puts("========================================\n");
    vga_puts("x86-64 Pseudo-Big-Endian Test Kernel\n");
    vga_puts("========================================\n");
    vga_puts("\n");

    vga_puts("Testing pseudo-endian boot scenario:\n");
    vga_puts("  Bootloader: little-endian (native)\n");
    vga_puts("  Kernel:     pseudo-big-endian\n");
    vga_puts("  CPU:        little-endian (x86 HW)\n");
    vga_puts("\n");

    vga_puts("Native CPU endianness: ");
    vga_puts(get_native_endian_name());
    vga_puts("\n");
    vga_puts("Kernel expects: big-endian boot data\n");
    vga_puts("Kernel performs: software byte-swapping\n");
    vga_puts("\n");

    // Run verification tests
    int all_passed = 1;

    all_passed &= verify_magic();
    all_passed &= verify_endianness();
    all_passed &= verify_architecture();
    all_passed &= verify_memory_regions();

    vga_puts("\n");
    vga_puts("========================================\n");
    if (all_passed) {
        vga_puts("RESULT: ALL TESTS PASSED!\n");
        vga_puts("Pseudo-endian boot working correctly.\n");
    } else {
        vga_puts("RESULT: TESTS FAILED!\n");
        vga_puts("Pseudo-endian boot NOT working.\n");
    }
    vga_puts("========================================\n");

    // Halt
    while (1) {
        __asm__ volatile ("hlt");
    }
}
