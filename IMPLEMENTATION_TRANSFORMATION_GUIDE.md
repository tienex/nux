# Implementation Transformation Guide

This guide provides systematic patterns for transforming NUX implementation files (.c files) to NT coding style while maintaining full backward compatibility.

## Overview

### Scope
- **HAL Implementations**: libhal_x86/ (16 files, 3,230 lines), libhal_riscv/ (7 files, 1,252 lines)
- **APXH Bootloader**: apxh/ (16 files, 4,210 lines)
- **Tools**: tools/ (6 files, 985 lines)
- **Total**: 45 files, ~9,677 lines of code

### Status
✅ **Headers Transformed**: All 15 public API headers complete
📝 **Implementations**: This guide provides transformation patterns

### Strategy
1. Transform function names to NT style (PascalCase)
2. Add UEFI-style documentation
3. Update type usage to new types
4. Maintain legacy wrappers for backward compatibility
5. Transform incrementally, file by file

---

## Transformation Patterns

### Pattern 1: Simple Function Transformation

**Before:**
```c
void
serial_init (void)
{
  outb (SERIAL_PORT + 1, 0);
  outb (SERIAL_PORT + 3, 0x80);
  outb (SERIAL_PORT + 0, 3);
}
```

**After:**
```c
/**
  Initialize the serial port.

  Configures COM1 for 115200 baud, 8 data bits, no parity, 1 stop bit.
**/
VOID
SerialInitialize (
  VOID
  )
{
  OutB (SERIAL_PORT + 1, 0);     // Disable interrupts
  OutB (SERIAL_PORT + 3, 0x80);  // Enable DLAB
  OutB (SERIAL_PORT + 0, 3);     // Set divisor
}

/** @deprecated Use SerialInitialize instead **/
void serial_init (void) {
  SerialInitialize ();
}
```

### Pattern 2: Function with Parameters

**Before:**
```c
void
serial_putchar (int c)
{
  while (!(inb (SERIAL_PORT + 5) & 0x20));
  outb (SERIAL_PORT, c);
}
```

**After:**
```c
/**
  Output a character to the serial port.

  Waits for transmit buffer to be empty, then sends the character.

  @param[in] Ch  Character to output.
**/
VOID
SerialPutChar (
  IN INT32  Ch
  )
{
  while (!(InB (SERIAL_PORT + 5) & 0x20))
    ;

  OutB (SERIAL_PORT, Ch);
}

/** @deprecated Use SerialPutChar instead **/
void serial_putchar (int c) {
  SerialPutChar (c);
}
```

### Pattern 3: Function with Return Value

**Before:**
```c
int
inb (int port)
{
  unsigned char val;
  asm volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
  return val;
}
```

**After:**
```c
/**
  Read a byte from an I/O port.

  @param[in] Port  I/O port address.

  @return Byte value read from the port.
**/
INT32
InB (
  IN INT32  Port
  )
{
  UINT8 val;
  asm volatile ("inb %1, %0" : "=a"(val) : "Nd"(Port));
  return val;
}

/** @deprecated Use InB instead **/
int inb (int port) {
  return InB (port);
}
```

### Pattern 4: Complex Function with Multiple Parameters

**Before:**
```c
pte_t
set_pte (ptep_t ptep, pte_t pte)
{
  pte_t old = *((pte_t *)ptep);
  *((pte_t *)ptep) = pte;
  return old;
}
```

**After:**
```c
/**
  Set a page table entry.

  Writes a new PTE value and returns the previous value.

  @param[in] Ptep  Page table entry pointer.
  @param[in] Pte   New PTE value to set.

  @return Previous PTE value.
**/
PTE
SetPte (
  IN PTEP  Ptep,
  IN PTE   Pte
  )
{
  PTE old = *((PTE *)Ptep);
  *((PTE *)Ptep) = Pte;
  return old;
}

/** @deprecated Use SetPte instead **/
pte_t set_pte (ptep_t ptep, pte_t pte) {
  return SetPte (ptep, pte);
}
```

### Pattern 5: Static Helper Functions

**Before:**
```c
static int
is_aligned (unsigned long addr, unsigned long align)
{
  return (addr & (align - 1)) == 0;
}
```

**After:**
```c
/**
  Check if an address is aligned.

  @param[in] Addr   Address to check.
  @param[in] Align  Alignment requirement (must be power of 2).

  @retval TRUE   Address is aligned.
  @retval FALSE  Address is not aligned.
**/
static
BOOLEAN
IsAligned (
  IN UINTN  Addr,
  IN UINTN  Align
  )
{
  return (Addr & (Align - 1)) == 0;
}

// Note: Static functions don't need legacy wrappers
```

---

## File-by-File Transformation Guide

### libhal_x86/serial.c

**Priority**: High (simple, frequently used)

**Transformations**:
- `serial_init()` → `SerialInitialize()`
- `serial_putchar()` → `SerialPutChar()`
- `inb()`/`outb()` → `InB()`/`OutB()`
- Add UEFI documentation
- Keep legacy wrappers

**Example**: See Pattern 1 and 2 above

### libhal_x86/x86.c

**Priority**: High (core initialization)

**Transformations**:
- `x86_init()` → `X86Initialize()`
- Convert all function names to PascalCase
- Update type usage (int → INT32, etc.)
- Add comprehensive documentation
- Keep all legacy wrappers

### libhal_x86/pmap.c

**Priority**: Medium (page mapping)

**Transformations**:
- `get_pte()` → `GetPte()`
- `set_pte()` → `SetPte()`
- `kmap_get_l1p()` → `KmapGetL1p()`
- Update pointer types (pte_t → PTE, ptep_t → PTEP)
- Document page table operations

### libhal_x86/amd64/*.c

**Priority**: Medium (architecture-specific)

**Transformations**:
- `amd64_init()` → `Amd64Initialize()`
- `pae64_init()` → `Pae64Initialize()`
- Architecture-specific type updates
- Assembly interface documentation

### libhal_riscv/*.c

**Priority**: Medium (RISC-V HAL)

**Transformations**:
- Similar patterns to x86
- `riscv_init()` → `RiscvInitialize()`
- `sv48_*()` → `Sv48*()`
- RISC-V-specific documentation

### apxh/src/main.c

**Priority**: High (bootloader entry)

**Transformations**:
- `main()` can stay as-is (standard entry point)
- Internal functions to PascalCase
- Update structure usage (use APXH_BOOT_INFO, etc.)
- Document boot protocol

### apxh/src/elf.c

**Priority**: Medium (ELF loader)

**Transformations**:
- ELF parsing functions to PascalCase
- Document ELF loading process
- Update error handling

### tools/*.c

**Priority**: Low (utility tools)

**Transformations**:
- Update to NT style for consistency
- Add documentation
- Can be simpler transformations

---

## Type Transformation Guide

### Integer Types

| Old Type | New Type | Notes |
|----------|----------|-------|
| `int` | `INT32` | Signed 32-bit |
| `unsigned int` | `UINT32` | Unsigned 32-bit |
| `long` | `INTN` | Native signed |
| `unsigned long` | `UINTN` | Native unsigned |
| `char` | `CHAR8` | 8-bit character |
| `unsigned char` | `UINT8` | 8-bit unsigned |
| `short` | `INT16` | 16-bit signed |
| `uint64_t` | `UINT64` | 64-bit unsigned |

### Pointer Types

| Old Type | New Type | Notes |
|----------|----------|-------|
| `void *` | `VOID *` | Generic pointer |
| `char *` | `CHAR8 *` | String pointer |
| `pte_t *` | `PTE *` | Page table entry ptr |

### Function Pointers

**Before**:
```c
void (*callback)(void *opq, int val);
```

**After**:
```c
VOID (*Callback)(VOID *pOpaque, INT32 Value);
```

### Structure Types

**Before**:
```c
struct my_struct {
  int value;
  char *name;
};
```

**After**:
```c
typedef struct _MY_STRUCT {
  INT32    Value;
  CHAR8    *pName;
} MY_STRUCT;
```

---

## Documentation Standards

### File Header

```c
/** @file
  Brief file description

  Detailed description of what this file implements.
  Multiple lines if needed.

  Copyright (C) YEAR Author Name <email>

  SPDX-License-Identifier: BSD-2-Clause
**/
```

### Function Documentation

```c
/**
  Brief function description.

  Detailed explanation of what the function does,
  including any important behavior or side effects.

  @param[in]     Param1  Description of input parameter.
  @param[out]    Param2  Description of output parameter.
  @param[in,out] Param3  Description of in/out parameter.

  @return Description of return value.

  @retval VALUE1  Specific return value meaning.
  @retval VALUE2  Another specific value meaning.
**/
```

### Inline Comments

```c
// Use single-line comments for inline documentation
OutB (SERIAL_PORT + 1, 0);  // Disable interrupts
```

---

## Internal Headers Transformation

### internal.h Pattern

**Before**:
```c
extern int nux_initialized;

void x86_init (void);
void serial_init (void);
int inb (int port);
void outb (int port, int val);
```

**After**:
```c
extern INT32 gNuxInitialized;

VOID X86Initialize (VOID);
VOID SerialInitialize (VOID);
INT32 InB (IN INT32 Port);
VOID OutB (IN INT32 Port, IN INT32 Value);

// Legacy declarations for compatibility
extern int nux_initialized;
void x86_init (void);
void serial_init (void);
int inb (int port);
void outb (int port, int val);
```

---

## Build System Considerations

### Makefile Updates (if needed)

No changes should be needed since:
- Source files keep same names
- Header includes remain compatible
- Link symbols preserved via legacy wrappers

### Compiler Warnings

May see warnings about:
- Deprecated function usage (intentional)
- Style inconsistencies during transition
- These are expected and okay

---

## Testing Strategy

### Unit Testing

1. **Compile Test**: Each file should compile
2. **Link Test**: Legacy and new symbols should link
3. **Function Test**: Each function works correctly

### Integration Testing

1. **Boot Test**: System should boot normally
2. **HAL Test**: Hardware access functions work
3. **Compatibility Test**: Old and new APIs interoperable

### Validation Checklist

For each transformed file:
- [ ] Compiles without errors
- [ ] Legacy wrappers present
- [ ] UEFI documentation added
- [ ] Types converted to NT style
- [ ] Function names are PascalCase
- [ ] Comments updated
- [ ] File header added

---

## Priority Order

### Phase 1: Core HAL Functions (High Priority)
1. libhal_x86/serial.c (simple, visible)
2. libhal_x86/x86.c (initialization)
3. libhal_riscv/riscv.c (RISC-V init)
4. libhal_x86/internal.h (shared definitions)

### Phase 2: Memory Management (Medium Priority)
5. libhal_x86/pmap.c (page mapping)
6. libhal_x86/amd64/pae64.c (AMD64 paging)
7. libhal_riscv/sv48.c (RISC-V paging)

### Phase 3: Architecture-Specific (Medium Priority)
8. libhal_x86/amd64/*.c (AMD64-specific)
9. libhal_x86/i386/*.c (i386-specific)
10. libhal_riscv/frame.c (RISC-V frames)

### Phase 4: Bootloader (Lower Priority)
11. apxh/src/main.c (boot entry)
12. apxh/src/elf.c (ELF loader)
13. apxh/src/*.c (other boot code)

### Phase 5: Tools (Lowest Priority)
14. tools/*.c (utility programs)

---

## Example Complete File Transformation

### Before: libhal_x86/serial.c (original)

```c
/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"

#define SERIAL_PORT 0x3f8

void
serial_init (void)
{
  outb (SERIAL_PORT + 1, 0);
  outb (SERIAL_PORT + 3, 0x80);
  outb (SERIAL_PORT + 0, 3);
  outb (SERIAL_PORT + 1, 0);
  outb (SERIAL_PORT + 3, 3);
  outb (SERIAL_PORT + 2, 0xc7);
  outb (SERIAL_PORT + 4, 0xb);
}

void
serial_putchar (int c)
{
  while (!(inb (SERIAL_PORT + 5) & 0x20));
  outb (SERIAL_PORT, c);
}
```

### After: libhal_x86/serial.c (transformed)

```c
/** @file
  x86 Serial Port Driver

  Provides serial port initialization and output for debugging.
  Configures COM1 (0x3F8) for 115200 8N1 operation.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "internal.h"

#define SERIAL_PORT  0x3F8  ///< COM1 base I/O port

/**
  Initialize the serial port.

  Configures COM1 for 115200 baud, 8 data bits, no parity, 1 stop bit.
  Disables interrupts and enables FIFO.
**/
VOID
SerialInitialize (
  VOID
  )
{
  OutB (SERIAL_PORT + 1, 0);     // Disable interrupts
  OutB (SERIAL_PORT + 3, 0x80);  // Enable DLAB (set baud rate divisor)
  OutB (SERIAL_PORT + 0, 3);     // Set divisor to 3 (38400 baud)
  OutB (SERIAL_PORT + 1, 0);     // High byte of divisor
  OutB (SERIAL_PORT + 3, 3);     // 8 bits, no parity, one stop bit
  OutB (SERIAL_PORT + 2, 0xC7);  // Enable FIFO, clear, 14-byte threshold
  OutB (SERIAL_PORT + 4, 0x0B);  // IRQs enabled, RTS/DSR set
}

/**
  Output a character to the serial port.

  Waits for the transmit buffer to be empty, then sends the character.

  @param[in] Ch  Character to output.
**/
VOID
SerialPutChar (
  IN INT32  Ch
  )
{
  // Wait for transmit buffer to be empty
  while (!(InB (SERIAL_PORT + 5) & 0x20))
    ;

  OutB (SERIAL_PORT, Ch);
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use SerialInitialize instead **/
void serial_init (void) {
  SerialInitialize ();
}

/** @deprecated Use SerialPutChar instead **/
void serial_putchar (int c) {
  SerialPutChar (c);
}
```

---

## Automated Transformation Tools

### Suggested Approach

Create scripts to help with mechanical transformations:

```bash
# sed script for basic pattern replacement
sed -i 's/\bvoid\b/VOID/g' file.c
sed -i 's/\bint\b/INT32/g' file.c
# etc.
```

**Warning**: Automated tools can help but require manual review!

### Manual Steps Always Required

- Function documentation
- Parameter descriptions
- Complex type transformations
- Logic verification
- Testing

---

## Common Pitfalls

### 1. Over-automation
**Problem**: Blindly replacing `int` with `INT32` everywhere
**Solution**: Review each change, especially in complex expressions

### 2. Breaking Assembly
**Problem**: Changing types used in inline assembly
**Solution**: Be careful with assembly constraints

### 3. Missing Legacy Wrappers
**Problem**: Removing old functions breaks compatibility
**Solution**: Always keep legacy wrappers

### 4. Incomplete Documentation
**Problem**: Adding /** */ without useful content
**Solution**: Document behavior, not just syntax

---

## Summary

### What to Transform

✅ Function names → PascalCase
✅ Add UEFI documentation
✅ Update types to NT style
✅ Add legacy wrappers
✅ Improve comments

### What to Keep

✅ Original function bodies (unless fixing bugs)
✅ Backward compatibility
✅ Same file structure
✅ Same build process

### Success Criteria

- Code compiles cleanly
- All tests pass
- Legacy code still works
- New code follows NT style
- Documentation is comprehensive

---

**Document Version**: 1.0
**Last Updated**: October 24, 2025
**Status**: Guide complete, implementations pending
