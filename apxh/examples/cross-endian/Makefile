# Makefile for Cross-Endian Boot Examples
#
# Builds test kernels for demonstrating cross-endian boot functionality
# in APXH bootloader.

# Toolchains
RISCV64_CC = riscv64-linux-gnu-gcc
X86_64_CC = x86_64-linux-gnu-gcc

# Common flags
COMMON_CFLAGS = -nostdlib -static -ffreestanding -O2 -Wall -Wextra
COMMON_LDFLAGS = -static -nostdlib

# RISC-V flags
RISCV64_CFLAGS = -march=rv64imafdc -mabi=lp64d
RISCV64_LDFLAGS =

# x86-64 flags
X86_64_CFLAGS = -m64 -mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse -mno-sse2
X86_64_LDFLAGS =

.PHONY: all clean riscv64 x86_64

all: riscv64 x86_64

# ============================================================
# RISC-V64 Examples
# ============================================================

riscv64: riscv64-be-kernel.elf riscv64-le-kernel.elf

# RISC-V64 Big-Endian Kernel
riscv64-be-kernel.elf: riscv64-be-kernel.o riscv64-start-be.o
	$(RISCV64_CC) $(COMMON_LDFLAGS) $(RISCV64_LDFLAGS) \
		-T riscv64-be.lds \
		-o $@ $^
	@echo "Built RISC-V64 big-endian kernel: $@"
	@file $@

riscv64-be-kernel.o: riscv64-be-kernel.c
	$(RISCV64_CC) $(COMMON_CFLAGS) $(RISCV64_CFLAGS) -mbig-endian \
		-c -o $@ $<

riscv64-start-be.o: riscv64-start.S
	$(RISCV64_CC) $(RISCV64_CFLAGS) -mbig-endian \
		-c -o $@ $<

# RISC-V64 Little-Endian Kernel
riscv64-le-kernel.elf: riscv64-le-kernel.o riscv64-start-le.o
	$(RISCV64_CC) $(COMMON_LDFLAGS) $(RISCV64_LDFLAGS) \
		-T riscv64-le.lds \
		-o $@ $^
	@echo "Built RISC-V64 little-endian kernel: $@"
	@file $@

riscv64-le-kernel.o: riscv64-le-kernel.c
	$(RISCV64_CC) $(COMMON_CFLAGS) $(RISCV64_CFLAGS) -mlittle-endian \
		-c -o $@ $<

riscv64-start-le.o: riscv64-start.S
	$(RISCV64_CC) $(RISCV64_CFLAGS) -mlittle-endian \
		-c -o $@ $<

# ============================================================
# x86-64 Examples
# ============================================================

x86_64: x86_64-pseudo-be-kernel.elf

# x86-64 Pseudo-Big-Endian Kernel
x86_64-pseudo-be-kernel.elf: x86_64-pseudo-be-kernel.o x86_64-start.o
	$(X86_64_CC) $(COMMON_LDFLAGS) $(X86_64_LDFLAGS) \
		-T x86_64-pseudo-be.lds \
		-o $@ $^
	@echo "Built x86-64 pseudo-big-endian kernel: $@"
	@echo "Patching ELF header to mark as big-endian..."
	@./patch-elf-endian.sh $@
	@file $@

x86_64-pseudo-be-kernel.o: x86_64-pseudo-be-kernel.c
	$(X86_64_CC) $(COMMON_CFLAGS) $(X86_64_CFLAGS) \
		-c -o $@ $<

x86_64-start.o: x86_64-start.S
	$(X86_64_CC) $(X86_64_CFLAGS) \
		-c -o $@ $<

# ============================================================
# Testing
# ============================================================

test-riscv64-be: riscv64-be-kernel.elf
	@echo "Testing RISC-V64 big-endian kernel in QEMU..."
	@echo "This requires a big-endian APXH bootloader"
	@echo "Command: qemu-system-riscv64 -M virt -kernel riscv64-be-kernel.elf -nographic"

test-riscv64-le: riscv64-le-kernel.elf
	@echo "Testing RISC-V64 little-endian kernel in QEMU..."
	@echo "This requires a little-endian APXH bootloader"
	@echo "Command: qemu-system-riscv64 -M virt -kernel riscv64-le-kernel.elf -nographic"

test-x86_64-pseudo-be: x86_64-pseudo-be-kernel.elf
	@echo "Testing x86-64 pseudo-big-endian kernel in QEMU..."
	@echo "This requires a little-endian APXH bootloader that converts to BE"
	@echo "Command: qemu-system-x86_64 -kernel x86_64-pseudo-be-kernel.elf -nographic"

# ============================================================
# Utility
# ============================================================

clean:
	rm -f *.o *.elf

distclean: clean

info:
	@echo "Cross-Endian Boot Examples"
	@echo "==========================="
	@echo ""
	@echo "RISC-V64 Targets:"
	@echo "  riscv64-be-kernel.elf  - Big-endian RISC-V64 test kernel"
	@echo "  riscv64-le-kernel.elf  - Little-endian RISC-V64 test kernel"
	@echo ""
	@echo "x86-64 Targets:"
	@echo "  x86_64-pseudo-be-kernel.elf  - Pseudo-big-endian x86-64 kernel"
	@echo ""
	@echo "Build all: make all"
	@echo "Clean:     make clean"
