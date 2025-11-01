# NTRTL C++ ABI Name Demangling

This directory contains C++ symbol name demanglers for different ABI schemes.

## Current Implementation

Simple recursive descent parsers that handle common mangling patterns:

- **itanium/demangle.c** - Itanium C++ ABI (GCC, Clang, ICC)
  - Handles basic types, namespaces, templates, function parameters
  - Substitution table for compression
  - Recognizes mangled names starting with `_Z`

- **msvc/demangle.c** - MSVC C++ ABI
  - Handles basic types, scope qualifiers, function parameters
  - Recognizes mangled names starting with `?`

- **watcom/demangle.c** - Watcom/Borland C++ ABI
  - Handles basic class members, parameters
  - Recognizes mangled names starting with `@` or `_`

- **demangle.c** - Unified demangler that tries all ABIs

## Limitations

These are simplified implementations that handle common cases. For complete
demangling of complex symbols (advanced templates, lambdas, expression encodings),
consider integrating LLVM's demanglers from https://github.com/nico/demumble

## Future Work

Full LLVM/demumble integration would require either:
1. Porting LLVM's C++ demangler code to C (significant effort)
2. Building LLVM demanglers as separate C++ library (requires C++ support)
3. Using the simplified C implementations (current approach)

The current implementations are sufficient for debugging, stack traces, and
basic symbol name resolution in embedded/kernel environments.

## Usage

```c
#include <ananke/ntrtl.h>

CHAR8 Demangled[1024];
UINTN Length;

// Auto-detect and demangle
Length = RtlDemangleName(
    "_ZN3std6vectorIiE4pushEi",
    Demangled,
    sizeof(Demangled)
);

// Or use specific ABI
Length = RtlDemangleNameItanium(MangledName, Demangled, sizeof(Demangled));
Length = RtlDemangleNameMSVC(MangledName, Demangled, sizeof(Demangled));
Length = RtlDemangleNameWatcom(MangledName, Demangled, sizeof(Demangled));
```

## cxxCRT Wrappers

cxxCRT provides compiler-specific wrappers:

- `__cxa_demangle()` - Itanium C++ ABI standard function (GCC/Clang)
- `__unDName()` / `UnDecorateSymbolName()` - MSVC functions

See `ananke/libs/cxxcrt/include/public/cxxcrt/demangle.h`
