/**
 * RISC-V64 Little-Endian Test Kernel
 *
 * Demonstrates cross-endian boot where a big-endian bootloader
 * boots a little-endian RISC-V kernel. Verifies that boot structures
 * are correctly byte-swapped.
 *
 * Build with:
 *   riscv64-linux-gnu-gcc -march=rv64imafdc -mabi=lp64d -mlittle-endian \
 *     -nostdlib -static -Wl,-T,riscv64-le.lds -o riscv64-le-kernel.elf \
 *     riscv64-le-kernel.c riscv64-start.S
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
    uint8_t  KernelArchitecture;    // ArchRiscV64 = 8
    uint8_t  UserArchitecture;
    uint8_t  HostArchitecture;
    uint8_t  KernelEndianness;      // ImgEndianLittle = 1
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

// UART base address for QEMU virt machine
#define UART_BASE  0x10000000
#define UART_THR   ((volatile uint8_t *)(UART_BASE + 0))
#define UART_LSR   ((volatile uint8_t *)(UART_BASE + 5))

// Architecture enum values
#define ArchRiscV64  8

// Endianness enum values
#define ImgEndianUnknown  0
#define ImgEndianLittle   1
#define ImgEndianBig      2

// Global pointer to boot info (passed from bootloader)
static APXH_BOOT_INFO *gBootInfo = NULL;
static APXH_REGION *gMemRegions = NULL;

// Simple UART output
static void uart_putc(char c) {
    while ((*UART_LSR & 0x20) == 0);  // Wait for transmit ready
    *UART_THR = c;
}

static void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

// Hex printing
static void print_hex64(uint64_t val) {
    const char *hex = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        uart_putc(hex[(val >> i) & 0xF]);
    }
}

static void print_hex32(uint32_t val) {
    const char *hex = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uart_putc(hex[(val >> i) & 0xF]);
    }
}

static void print_dec(uint64_t val) {
    char buf[32];
    int i = 0;

    if (val == 0) {
        uart_putc('0');
        return;
    }

    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }

    while (i > 0) {
        uart_putc(buf[--i]);
    }
}

// Get native endianness (should always be little-endian for this kernel)
static const char* get_native_endian_name(void) {
    uint32_t test = 0x12345678;
    uint8_t *p = (uint8_t*)&test;

    if (p[0] == 0x12) return "big-endian";
    if (p[0] == 0x78) return "little-endian";
    return "unknown";
}

// Verify boot info magic number
static int verify_magic(void) {
    uart_puts("Verifying boot info magic number...\n");
    uart_puts("  Expected: ");
    print_hex64(APXH_BOOTINFO_MAGIC);
    uart_puts("\n  Got:      ");
    print_hex64(gBootInfo->Magic);
    uart_puts("\n");

    if (gBootInfo->Magic == APXH_BOOTINFO_MAGIC) {
        uart_puts("  [OK] Magic number matches!\n");
        return 1;
    } else {
        uart_puts("  [FAIL] Magic number mismatch - endianness conversion failed!\n");
        return 0;
    }
}

// Verify endianness fields
static int verify_endianness(void) {
    uart_puts("\nVerifying endianness fields...\n");

    uart_puts("  Kernel endianness: ");
    print_dec(gBootInfo->KernelEndianness);
    if (gBootInfo->KernelEndianness == ImgEndianLittle) {
        uart_puts(" (little-endian) [OK]\n");
    } else {
        uart_puts(" [FAIL - expected little-endian]\n");
        return 0;
    }

    uart_puts("  User endianness:   ");
    print_dec(gBootInfo->UserEndianness);
    switch (gBootInfo->UserEndianness) {
        case ImgEndianUnknown:
            uart_puts(" (unknown/no-user) [OK]\n");
            break;
        case ImgEndianLittle:
            uart_puts(" (little-endian)\n");
            break;
        case ImgEndianBig:
            uart_puts(" (big-endian)\n");
            break;
    }

    uart_puts("  Mixed-endian:      ");
    print_dec(gBootInfo->MixedEndian);
    if (gBootInfo->MixedEndian) {
        uart_puts(" (TRUE - kernel and user have different endianness)\n");
    } else {
        uart_puts(" (FALSE)\n");
    }

    return 1;
}

// Verify architecture fields
static int verify_architecture(void) {
    uart_puts("\nVerifying architecture fields...\n");

    uart_puts("  Kernel architecture: ");
    print_dec(gBootInfo->KernelArchitecture);
    if (gBootInfo->KernelArchitecture == ArchRiscV64) {
        uart_puts(" (RISCV64) [OK]\n");
    } else {
        uart_puts(" [FAIL - expected RISCV64]\n");
        return 0;
    }

    uart_puts("  Host architecture:   ");
    print_dec(gBootInfo->HostArchitecture);
    uart_puts("\n");

    return 1;
}

// Verify memory regions
static int verify_memory_regions(void) {
    uart_puts("\nVerifying memory regions...\n");
    uart_puts("  Number of regions: ");
    print_dec(gBootInfo->NumRegions);
    uart_puts("\n");

    if (gBootInfo->NumRegions == 0 || gBootInfo->NumRegions > 1000) {
        uart_puts("  [FAIL] Invalid region count - endianness conversion failed!\n");
        return 0;
    }

    // Check first region
    uart_puts("  First region:\n");
    uart_puts("    Type:   ");
    print_dec(gMemRegions[0].Type);
    uart_puts("\n");
    uart_puts("    PFN:    ");
    print_hex64(gMemRegions[0].Pfn);
    uart_puts("\n");
    uart_puts("    Length: ");
    print_dec(gMemRegions[0].Length);
    uart_puts("\n");

    if (gMemRegions[0].Type > 20 || gMemRegions[0].Length == 0) {
        uart_puts("  [FAIL] Region data corrupted - endianness conversion failed!\n");
        return 0;
    }

    uart_puts("  [OK] Memory regions valid!\n");
    return 1;
}

// Kernel entry point (called from assembly startup)
void kernel_main(uint64_t boot_info_addr, uint64_t regions_addr) {
    gBootInfo = (APXH_BOOT_INFO*)boot_info_addr;
    gMemRegions = (APXH_REGION*)regions_addr;

    uart_puts("\n");
    uart_puts("========================================\n");
    uart_puts("RISC-V64 Little-Endian Test Kernel\n");
    uart_puts("========================================\n");
    uart_puts("\n");

    uart_puts("Testing cross-endian boot scenario:\n");
    uart_puts("  Bootloader: big-endian (assumed)\n");
    uart_puts("  Kernel:     little-endian (this kernel)\n");
    uart_puts("\n");

    uart_puts("Native CPU endianness: ");
    uart_puts(get_native_endian_name());
    uart_puts("\n\n");

    // Run verification tests
    int all_passed = 1;

    all_passed &= verify_magic();
    all_passed &= verify_endianness();
    all_passed &= verify_architecture();
    all_passed &= verify_memory_regions();

    uart_puts("\n");
    uart_puts("========================================\n");
    if (all_passed) {
        uart_puts("RESULT: ALL TESTS PASSED!\n");
        uart_puts("Cross-endian boot working correctly.\n");
    } else {
        uart_puts("RESULT: TESTS FAILED!\n");
        uart_puts("Cross-endian boot NOT working.\n");
    }
    uart_puts("========================================\n");
    uart_puts("\n");

    // Halt
    while (1) {
        __asm__ volatile ("wfi");  // Wait for interrupt
    }
}
