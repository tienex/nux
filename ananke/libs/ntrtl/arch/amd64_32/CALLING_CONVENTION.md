# x32 ABI Calling Convention

## Overview

The **x32 ABI** (also known as **amd64_32** or **ILP32 for x86-64**) provides 32-bit integers, longs, and pointers (ILP32 data model) on Intel and AMD 64-bit hardware. This ABI allows programs to take advantage of the benefits of the x86-64 instruction set while avoiding the memory overhead of 64-bit pointers.

## Benefits

- Access to all 16 x86-64 general-purpose registers (RAX-R15)
- Better floating-point performance (16 XMM/YMM/ZMM registers)
- Faster position-independent code
- Function parameters passed via registers
- Faster `syscall` instruction
- Reduced memory footprint compared to x86-64 LP64

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

### General Purpose Registers

x32 uses the same registers as x86-64 but with 32-bit operations for addresses and pointer arithmetic.

| Register | Usage | Preserved across calls |
|----------|-------|------------------------|
| %rax/%eax | Return value, syscall number | No (caller-saved) |
| %rbx/%ebx | General purpose | Yes (callee-saved) |
| %rcx/%ecx | 4th argument | No (caller-saved) |
| %rdx/%edx | 3rd argument, return value 2 | No (caller-saved) |
| %rsi/%esi | 2nd argument | No (caller-saved) |
| %rdi/%edi | 1st argument | No (caller-saved) |
| %rbp/%ebp | Frame pointer (optional) | Yes (callee-saved) |
| %rsp/%esp | Stack pointer | Yes (callee-saved) |
| %r8/%r8d | 5th argument | No (caller-saved) |
| %r9/%r9d | 6th argument | No (caller-saved) |
| %r10/%r10d | Temporary, syscall clobber | No (caller-saved) |
| %r11/%r11d | Temporary, syscall clobber | No (caller-saved) |
| %r12/%r12d | General purpose | Yes (callee-saved) |
| %r13/%r13d | General purpose | Yes (callee-saved) |
| %r14/%r14d | General purpose | Yes (callee-saved) |
| %r15/%r15d | General purpose | Yes (callee-saved) |

**Important**: When working with pointers and addresses in x32:
- Use 32-bit register forms for address arithmetic (%edi, %esi, etc.)
- Pointers are **zero-extended** to 64-bit when placed in registers
- Memory operands use 32-bit addressing modes
- Push/pop operations still use 64-bit register forms

### SIMD Registers

| Register | Usage | Preserved across calls |
|----------|-------|------------------------|
| %xmm0-%xmm1 | FP arguments, FP return values | No (caller-saved) |
| %xmm2-%xmm7 | FP arguments | No (caller-saved) |
| %xmm8-%xmm15 | Temporary | No (caller-saved) |
| %ymm0-%ymm15 | AVX extension of %xmm0-%xmm15 | No (caller-saved) |
| %zmm0-%zmm31 | AVX-512 extension | No (caller-saved) |

## Calling Convention

### Parameter Passing

The x32 ABI follows the System V AMD64 calling convention with 32-bit pointer adjustments.

**Integer/Pointer Parameters** (in order):
1. %edi (RDI with 32-bit value)
2. %esi (RSI with 32-bit value)
3. %edx (RDX with 32-bit value)
4. %ecx (RCX with 32-bit value)
5. %r8d (R8 with 32-bit value)
6. %r9d (R9 with 32-bit value)
7. Stack (8-byte aligned)

**Floating-Point Parameters** (in order):
1. %xmm0
2. %xmm1
3. %xmm2
4. %xmm3
5. %xmm4
6. %xmm5
7. %xmm6
8. %xmm7
9. Stack (8-byte aligned)

**Mixed Parameters**: Parameters are assigned to registers based on their class (INTEGER, SSE, MEMORY). See System V AMD64 ABI for detailed classification rules.

### Return Values

**Integer/Pointer Returns**:
- %eax (32-bit values, pointers)
- %eax:%edx (64-bit values split across two registers)

**Floating-Point Returns**:
- %xmm0 (float, double)
- %xmm0:%xmm1 (long double, complex types)

**Structures**:
- Small structures (≤ 16 bytes): Returned in %rax:%rdx or %xmm0:%xmm1 based on field types
- Large structures: Caller passes pointer in %rdi, function returns pointer in %rax

### Stack Alignment

- **Stack must be 16-byte aligned** at function entry (after `call` pushes return address)
- Before `call` instruction: %rsp must be 16-byte aligned
- After `call` instruction: %rsp is (16n + 8) aligned
- Function prologue must ensure 16-byte alignment for local variables
- Variadic functions require proper alignment of stack arguments

Example:
```asm
function_entry:
    # %rsp = 16n + 8 (return address pushed by call)
    subq $8, %rsp           # Align to 16 bytes
    # ... function body ...
    addq $8, %rsp
    ret
```

### Red Zone

**x32 does NOT have a red zone** (unlike x86-64 System V which has a 128-byte red zone). Signal handlers and interrupts can clobber data below %rsp.

## System Calls

### Syscall Instruction

x32 uses the `syscall` instruction with special conventions:

- Syscall number in %eax with **bit 30 set** (i.e., syscall_number | 0x40000000)
- Arguments passed in registers (same as function calls)
- Destroys %rcx and %r11
- Return value in %rax (negative values indicate errors)

**Syscall Register Mapping**:
1. %edi - arg1
2. %esi - arg2
3. %edx - arg3
4. %r10d - arg4 (NOT %ecx like function calls)
5. %r8d - arg5
6. %r9d - arg6

Example:
```asm
    mov $(__NR_write | 0x40000000), %eax  # Syscall number with bit 30
    mov $1, %edi                           # fd = 1 (stdout)
    lea message(%rip), %esi                # buf pointer
    mov $14, %edx                          # count
    syscall                                # Invoke kernel
```

## Compiler Detection

Detect x32 ABI in preprocessor:

```c
#if defined(__ILP32__) && defined(__x86_64__)
    /* x32 ABI code */
#elif defined(__x86_64__)
    /* x86-64 LP64 ABI code */
#elif defined(__i386__)
    /* 32-bit x86 code */
#endif
```

**Do NOT use** `__LP64__` to detect x32 - it will be undefined!

## Compilation

### GCC/Clang

```bash
# Compile for x32
gcc -mx32 source.c -o binary

# Cross-compile x32 from x86-64
gcc -mx32 -static source.c -o binary
```

### Assembler

```asm
    .code64                    # Use 64-bit instruction encodings

my_function:
    # Use 32-bit register names for pointers
    mov %edi, %eax             # Copy pointer (32-bit)
    mov (%rdi), %ecx           # Load from pointer (64-bit base, 32-bit value)

    # Stack operations use 64-bit
    pushq %rbx                 # Must push full 64-bit register
    popq %rbx                  # Must pop full 64-bit register

    # Arithmetic on pointers uses 32-bit
    add $16, %edi              # Advance pointer by 16 bytes
```

## ABI Differences from x86-64 LP64

| Aspect | x32 (ILP32) | x86-64 (LP64) |
|--------|-------------|---------------|
| Pointer size | 4 bytes | 8 bytes |
| `long` size | 4 bytes | 8 bytes |
| `size_t` size | 4 bytes | 8 bytes |
| Red zone | **None** | 128 bytes |
| Syscall numbers | Bit 30 set | Regular numbers |
| Syscall arg4 | %r10d | %r10 |
| Address space | 4 GB | 16 EB (theoretical) |

## Performance Considerations

### Advantages

1. **Smaller memory footprint**: Pointers are 50% smaller
2. **Better cache utilization**: More pointers fit in cache lines
3. **Reduced memory bandwidth**: Less data movement
4. **Full x86-64 ISA**: All performance features available

### Disadvantages

1. **4 GB address space limit**: Cannot address > 4 GB RAM per process
2. **Zero-extension overhead**: Pointers must be zero-extended when loaded
3. **Limited ecosystem**: Less common than x86-64 or i686

## References

- System V Application Binary Interface - AMD64 Architecture Processor Supplement
- x32 ABI Project: https://sites.google.com/site/x32abi/
- Linux x32 ABI Documentation: https://wiki.debian.org/X32Port
- GCC x32 Support: https://gcc.gnu.org/wiki/X32Port

## See Also

- `ananke/libs/ntrtl/arch/amd64_32/*.S` - Assembly implementations
- `ananke/libs/ccrt/sources/startup/gnu/arch/amd64_32/` - CRT startup code
- `ananke/libs/cxxcrt/` - C++ runtime support
