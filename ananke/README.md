# ANANKE - Common Foundation Library

ANANKE is the shared foundation library for NUX kernel and APXH bootloader components.

## Overview

ANANKE provides a portable, compiler-agnostic foundation layer that abstracts:

- **UEFI-width base types** (UINT8, UINT16, UINT32, UINT64, UINTN, INTN)
- **COM/IUnknown ABI** (HRESULT, GUID, interface definitions)
- **Atomic operations** (interlocked increment, compare-exchange, etc.)
- **Compiler attributes** (inline, noreturn, deprecated, alignment, etc.)
- **Calling conventions** (STDMETHODCALLTYPE, STDAPICALLTYPE)
- **Structure packing** (portable pragma push/pop)
- **Platform detection** (architecture, endianness, object format)

## Design Goals

1. **Wide compatibility**: C89..C23 and C++98..C++23
2. **Compiler-agnostic**: MSVC, Clang, GCC, Watcom
3. **Cross-platform**: 16/32/64-bit, x86/ARM/RISC-V/PPC/MIPS/etc.
4. **Strict COM ordering**: QueryInterface, AddRef, Release always first
5. **NT-era style**: Windows/UEFI naming conventions and coding standards
6. **Zero dependencies**: Only relies on stdint.h and stdbool.h

## Usage

Include the main header in your code:

```c
#include <ananke/common.h>
```

### Type Definitions

```c
UINT32 value = 0x12345678;              // 32-bit unsigned
UINTN  pointer = (UINTN)&value;         // Native pointer width
CHAR16 wideChar = L'A';                 // UTF-16 code unit
BOOLEAN flag = TRUE;                     // Boolean type
```

### COM Interface Usage

```c
// C++ view
IUnknown* pUnk = ...;
pUnk->QueryInterface(IID_IFoo, &pFoo);
pUnk->AddRef();
pUnk->Release();

// C view
IUnknown* pUnk = ...;
IUnknown_QueryInterface(pUnk, IID_IFoo, &pFoo);
IUnknown_AddRef(pUnk);
IUnknown_Release(pUnk);
```

### Atomic Operations

```c
VOLATILE INT32 refCount = 0;
ANX_INTERLOCKED_INCREMENT(&refCount);
ANX_INTERLOCKED_DECREMENT(&refCount);
ANX_INTERLOCKED_CMPXCHG(&refCount, newVal, oldVal);
```

### Structure Packing

```c
ANX_PACK_PUSH(1)
typedef struct _MY_PACKED_STRUCT {
    UINT8  Byte;
    UINT32 Dword;
} MY_PACKED_STRUCT ANX_PACKED;
ANX_PACK_POP()
```

### Compiler Attributes

```c
ANX_ATTR_NOINLINE VOID MyFunction(VOID) { ... }
ANX_ATTR_DEPRECATED("Use NewFunction") VOID OldFunction(VOID) { ... }
ANX_ATTR_NORETURN VOID Panic(VOID) { ... }
```

## Directory Structure

```
ananke/
├── include/
│   └── ananke/
│       └── common.h           # Main header
└── README.md                  # This file
```

## Integration

### APXH Bootloader

APXH can use ANANKE types and interfaces for consistent platform abstraction.

### NUX Kernel

NUX kernel components can use ANANKE as the foundational type system and COM infrastructure.

## License

SPDX-License-Identifier: BSD-2-Clause

Copyright (C) 2024 ANANKE Project Contributors
