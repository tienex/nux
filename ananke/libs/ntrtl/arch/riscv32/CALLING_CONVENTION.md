# RISC-V RV32 Calling Convention

## Overview

The **RISC-V RV32** architecture implements the 32-bit RISC-V instruction set with the **ILP32** data model (32-bit integers, longs, and pointers). The RISC-V calling convention is defined in the official **RISC-V ABIs Specification** and is used by GCC, Clang, and other RISC-V compilers.

## Benefits

- Clean, orthogonal instruction set
- 32 general-purpose integer registers
- 32 floating-point registers (with F/D extensions)
- Efficient parameter passing
- Scalable vector extension support (RVV)
- Modern ABI design

## Data Model

| Type | Size (bytes) | Alignment |
|------|--------------|-----------|
| `char` | 1 | 1 |
| `short` | 2 | 2 |
| `int` | 4 | 4 |
| `long` | 4 | 4 |
| `long long` | 8 | 8 |
| `pointer` | 4 | 4 |
| `size_t` | 4 | 4 |
| `float` | 4 | 4 |
| `double` | 8 | 8 |
| `long double` | 16 | 16 |

## Register Usage

### Integer Registers

RISC-V has 32 integer registers (x0-x31) with specific calling convention roles:

| Register | ABI Name | Usage | Preserved across calls |
|----------|----------|-------|------------------------|
| x0 | zero | Hard-wired zero | N/A |
| x1 | ra | Return address | No (caller-saved) |
| x2 | sp | Stack pointer | Yes (callee-saved) |
| x3 | gp | Global pointer | N/A (fixed) |
| x4 | tp | Thread pointer | N/A (fixed) |
| x5-x7 | t0-t2 | Temporaries | No (caller-saved) |
| x8 | s0/fp | Saved register / frame pointer | Yes (callee-saved) |
| x9 | s1 | Saved register | Yes (callee-saved) |
| x10-x11 | a0-a1 | Arguments / return values | No (caller-saved) |
| x12-x17 | a2-a7 | Arguments | No (caller-saved) |
| x18-x27 | s2-s11 | Saved registers | Yes (callee-saved) |
| x28-x31 | t3-t6 | Temporaries | No (caller-saved) |

**Key Points**:
- **x0 (zero)**: Always reads as 0, writes are discarded
- **x1 (ra)**: Return address link register, set by `jal`/`jalr`
- **x2 (sp)**: Stack pointer, grows downward (toward lower addresses)
- **x8 (s0/fp)**: Can be used as frame pointer or saved register
- **x10-x11 (a0-a1)**: Both arguments AND return values

### Floating-Point Registers

With F (single-precision) or D (double-precision) extensions:

| Register | ABI Name | Usage | Preserved across calls |
|----------|----------|-------|------------------------|
| f0-f7 | ft0-ft7 | FP temporaries | No (caller-saved) |
| f8-f9 | fs0-fs1 | FP saved registers | Yes (callee-saved) |
| f10-f11 | fa0-fa1 | FP arguments / return values | No (caller-saved) |
| f12-f17 | fa2-fa7 | FP arguments | No (caller-saved) |
| f18-f27 | fs2-fs11 | FP saved registers | Yes (callee-saved) |
| f28-f31 | ft8-ft11 | FP temporaries | No (caller-saved) |

## ABI Variants

RISC-V defines several ILP32 ABI variants:

| ABI | Description | Integer args | FP args | Use case |
|-----|-------------|--------------|---------|----------|
| **ilp32** | Soft-float | Integer regs only | Integer regs | No FPU hardware |
| **ilp32f** | Hard-float (single) | Integer regs | Float regs (SP) | F extension only |
| **ilp32d** | Hard-float (double) | Integer regs | Float regs (DP) | F+D extensions |
| **ilp32e** | Embedded (16 regs) | Integer regs (x0-x15) | Integer regs | Embedded systems |

This documentation focuses on **ilp32**, **ilp32f**, and **ilp32d** as used in NTRTL.

## Calling Convention

### Parameter Passing (ilp32/ilp32f/ilp32d)

**Integer/Pointer Parameters** (in order):
1. a0 (x10)
2. a1 (x11)
3. a2 (x12)
4. a3 (x13)
5. a4 (x14)
6. a5 (x15)
7. a6 (x16)
8. a7 (x17)
9. Stack (8-byte aligned)

**Floating-Point Parameters** (ilp32f/ilp32d only):
1. fa0 (f10)
2. fa1 (f11)
3. fa2 (f12)
4. fa3 (f13)
5. fa4 (f14)
6. fa5 (f15)
7. fa6 (f16)
8. fa7 (f17)
9. Stack (8-byte aligned)

**ilp32 (Soft-Float)**: All floating-point values are passed in integer registers using the integer calling convention.

Example for `double foo(int a, double b, long double c)` with **ilp32**:
- `a` passed in **a0**
- `b` passed in **a2:a3** (double split across two registers, a1 skipped for alignment)
- `c` passed **by reference** in **a4** (pointer to stack)

Example for the same function with **ilp32d**:
- `a` passed in **a0**
- `b` passed in **fa0** (double in FP register)
- `c` passed **by reference** in **a1** (pointer to stack)

### Return Values

**Integer/Pointer Returns**:
- a0 (32-bit values, pointers)
- a0:a1 (64-bit values split across two registers)

**Floating-Point Returns** (ilp32f/ilp32d):
- fa0 (float in ilp32f, float/double in ilp32d)
- fa0:fa1 (long double, complex types)

**ilp32 (Soft-Float)**:
- Float returned in a0
- Double returned in a0:a1
- Long double returned by reference (pointer in a0)

**Structures**:
- Small structures (≤ 8 bytes): Returned in a0 or a0:a1
- Structures with two FP fields (ilp32f/ilp32d): Returned in fa0:fa1
- Large structures: Caller passes pointer in a0, function returns pointer in a0

### Stack Alignment

- **Stack must be 16-byte aligned** at all times
- Function prologue maintains alignment
- Stack grows downward (sp decrements when pushing)
- No red zone (like x86-64 has)

Example:
```asm
function_entry:
    addi sp, sp, -32        # Allocate 32 bytes (maintains 16-byte alignment)
    sw   ra, 28(sp)         # Save return address
    sw   s0, 24(sp)         # Save frame pointer
    addi s0, sp, 32         # Set frame pointer
    # ... function body ...
    lw   ra, 28(sp)         # Restore return address
    lw   s0, 24(sp)         # Restore frame pointer
    addi sp, sp, 32         # Deallocate stack frame
    ret                     # Return (jalr x0, ra, 0)
```

### Function Prologue/Epilogue

**Leaf function** (doesn't call other functions):
```asm
leaf_function:
    # No prologue needed if no saved registers used
    # ... function body ...
    ret
```

**Non-leaf function**:
```asm
non_leaf_function:
    addi sp, sp, -32        # Allocate frame
    sw   ra, 28(sp)         # Save return address
    sw   s0, 24(sp)         # Save s0 if used
    sw   s1, 20(sp)         # Save s1 if used
    # ... function body ...
    lw   ra, 28(sp)         # Restore return address
    lw   s0, 24(sp)         # Restore s0
    lw   s1, 20(sp)         # Restore s1
    addi sp, sp, 32         # Deallocate frame
    ret
```

## System Calls

### ECALL Instruction

RISC-V uses the `ecall` instruction for system calls:

- Syscall number in a7 (x17)
- Arguments in a0-a5 (x10-x15)
- Return value in a0 (x10)
- Negative return values indicate errors

**Syscall Register Mapping**:
1. a0 (x10) - arg1, also return value
2. a1 (x11) - arg2
3. a2 (x12) - arg3
4. a3 (x13) - arg4
5. a4 (x14) - arg5
6. a5 (x15) - arg6
7. a7 (x17) - syscall number

Example:
```asm
    li   a7, 64             # syscall number: __NR_write
    li   a0, 1              # fd = 1 (stdout)
    la   a1, message        # buf pointer
    li   a2, 14             # count
    ecall                   # Invoke kernel
```

## Atomic Operations

RISC-V provides atomic memory operations (AMO) with the **A** extension:

### Load-Reserved / Store-Conditional

```asm
    lr.w    t0, (a0)        # Load-reserved word
    # ... modify t0 ...
    sc.w    t1, t0, (a0)    # Store-conditional word
    bnez    t1, retry       # Retry if sc.w failed
```

### Atomic Memory Operations

- `amoadd.w` - Atomic add
- `amoswap.w` - Atomic swap
- `amoand.w` - Atomic AND
- `amoor.w` - Atomic OR
- `amoxor.w` - Atomic XOR
- `amomin.w` / `amomax.w` - Atomic min/max
- `amominu.w` / `amomaxu.w` - Atomic unsigned min/max

**Ordering Suffixes**:
- `.aq` - Acquire semantics
- `.rl` - Release semantics
- `.aqrl` - Sequential consistency

Example:
```asm
    amoswap.w.aqrl a0, a1, (a0)    # Atomic exchange with seq-cst ordering
```

## Memory Ordering

RISC-V provides the `fence` instruction for memory ordering:

```asm
    fence   rw, rw          # Full memory barrier
    fence   r, rw           # Load barrier (acquire)
    fence   rw, w           # Store barrier (release)
    fence   iorw, iorw      # I/O fence
```

**Operands**: `fence` predecessor, successor
- `r` - Loads
- `w` - Stores
- `i` - Device input
- `o` - Device output

## Compiler Detection

Detect RV32 in preprocessor:

```c
#if defined(__riscv) && (__riscv_xlen == 32)
    /* RV32 code */
#elif defined(__riscv) && (__riscv_xlen == 64)
    /* RV64 code */
#endif

/* Detect ABI variant */
#if defined(__riscv_float_abi_soft)
    /* ilp32 soft-float */
#elif defined(__riscv_float_abi_single)
    /* ilp32f hard-float single */
#elif defined(__riscv_float_abi_double)
    /* ilp32d hard-float double */
#endif

/* Detect extensions */
#if defined(__riscv_a)
    /* Atomics extension */
#endif
#if defined(__riscv_f)
    /* Single-precision FP */
#endif
#if defined(__riscv_d)
    /* Double-precision FP */
#endif
#if defined(__riscv_v)
    /* Vector extension */
#endif
```

## Compilation

### GCC/Clang

```bash
# RV32 with base ISA + atomics
riscv32-unknown-elf-gcc -march=rv32ima -mabi=ilp32 source.c -o binary

# RV32 with single-precision FP
riscv32-unknown-elf-gcc -march=rv32imaf -mabi=ilp32f source.c -o binary

# RV32 with double-precision FP
riscv32-unknown-elf-gcc -march=rv32imafd -mabi=ilp32d source.c -o binary

# RV32 with compressed instructions
riscv32-unknown-elf-gcc -march=rv32gc -mabi=ilp32d source.c -o binary
```

**ISA String Components**:
- `i` - Base integer ISA (required)
- `m` - Integer multiplication/division
- `a` - Atomic instructions
- `f` - Single-precision floating-point
- `d` - Double-precision floating-point
- `c` - Compressed instructions (16-bit)
- `v` - Vector extension
- `g` - Shorthand for `imafd` (general-purpose)

### Assembler

```asm
    .option arch, rv32ima      # Specify architecture

    .text
    .align 2                   # Align to 4-byte boundary
    .globl my_function
    .type my_function, @function
my_function:
    addi sp, sp, -16           # Allocate stack frame
    sw   ra, 12(sp)            # Save return address

    # Function body
    mv   a0, a1                # Copy argument
    jal  other_function        # Call function

    lw   ra, 12(sp)            # Restore return address
    addi sp, sp, 16            # Deallocate frame
    ret                        # Return
    .size my_function, .-my_function
```

## Vector Extension (RVV)

RISC-V Vector (RVV) provides scalable SIMD operations. NTRTL includes optimized implementations for both RVV 0.9 and RVV 1.0.

### Key Concepts

- **VLEN**: Vector register length in bits (implementation-defined)
- **SEW**: Selected element width (8, 16, 32, 64 bits)
- **LMUL**: Vector register grouping (1/8, 1/4, 1/2, 1, 2, 4, 8)
- **vl**: Vector length (number of elements to operate on)

### Vector Configuration

```asm
    vsetvli t0, a2, e8, m8, ta, ma    # Set vtype for 8-bit elements, LMUL=8
    # t0 = actual vector length granted
    # a2 = requested vector length
    # e8 = 8-bit elements (SEW=8)
    # m8 = LMUL=8 (use 8 vector registers)
    # ta = tail agnostic
    # ma = mask agnostic
```

### Vector Memory Operations

```asm
    vle8.v   v0, (a1)          # Load vector (8-bit elements)
    vse8.v   v0, (a0)          # Store vector (8-bit elements)
    vlse32.v v0, (a1), a3      # Strided load (32-bit elements)
    vsuxei32.v v0, (a0), v8    # Indexed store
```

### Vector Arithmetic

```asm
    vadd.vv  v0, v1, v2        # v0[i] = v1[i] + v2[i]
    vadd.vx  v0, v1, a0        # v0[i] = v1[i] + a0
    vadd.vi  v0, v1, 5         # v0[i] = v1[i] + 5
    vsub.vv  v0, v1, v2        # v0[i] = v1[i] - v2[i]
    vmul.vv  v0, v1, v2        # v0[i] = v1[i] * v2[i]
```

## Bit Manipulation Extensions

### Zba - Address Generation

```asm
    sh1add a0, a1, a2          # a0 = a2 + (a1 << 1)
    sh2add a0, a1, a2          # a0 = a2 + (a1 << 2)
    sh3add a0, a1, a2          # a0 = a2 + (a1 << 3)
```

### Zbb - Basic Bit Manipulation

```asm
    andn   a0, a1, a2          # a0 = a1 & ~a2
    orn    a0, a1, a2          # a0 = a1 | ~a2
    xnor   a0, a1, a2          # a0 = ~(a1 ^ a2)
    clz    a0, a1              # a0 = count leading zeros
    ctz    a0, a1              # a0 = count trailing zeros
    cpop   a0, a1              # a0 = population count
    min    a0, a1, a2          # a0 = min(a1, a2)
    max    a0, a1, a2          # a0 = max(a1, a2)
    rol    a0, a1, a2          # a0 = rotate left
    ror    a0, a1, a2          # a0 = rotate right
    rev8   a0, a1              # Byte-reverse
    sext.b a0, a1              # Sign-extend byte
    sext.h a0, a1              # Sign-extend halfword
    zext.h a0, a1              # Zero-extend halfword
```

### Zbs - Single-Bit Operations

```asm
    bclr   a0, a1, a2          # Clear bit a2 in a1
    bset   a0, a1, a2          # Set bit a2 in a1
    binv   a0, a1, a2          # Invert bit a2 in a1
    bext   a0, a1, a2          # Extract bit a2 from a1
```

## Performance Considerations

### Advantages

1. **Clean ISA**: Orthogonal, regular instruction encoding
2. **Register-rich**: 32 integer + 32 FP registers
3. **Efficient encoding**: Compressed instructions save code size
4. **Scalable vectors**: RVV adapts to implementation VLEN
5. **Modern design**: Learned from decades of ISA evolution

### Best Practices

1. **Use compressed instructions** (`-march=rv32gc`) for code density
2. **Align loops** to 4-byte or 8-byte boundaries
3. **Prefer post-increment addressing** (more efficient than pre-increment)
4. **Use vector instructions** for data-parallel operations when available
5. **Minimize caller-saved register usage** across calls
6. **Use bit manipulation extensions** (Zba/Zbb/Zbs) when available

## References

- RISC-V ABIs Specification Version 1.1 (August 2024)
- RISC-V Instruction Set Manual Volume I: User-Level ISA
- RISC-V Instruction Set Manual Volume II: Privileged Architecture
- RISC-V Vector Extension Specification Version 1.0
- GCC RISC-V Options: https://gcc.gnu.org/onlinedocs/gcc/RISC-V-Options.html

## See Also

- `ananke/libs/ntrtl/arch/riscv32/*.S` - Assembly implementations
- `ananke/libs/ccrt/sources/startup/gnu/arch/riscv32/` - CRT startup code
- `ananke/libs/cxxcrt/` - C++ runtime support
