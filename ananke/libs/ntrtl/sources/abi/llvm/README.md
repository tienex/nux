# LLVM C++ Demanglers

This directory contains the LLVM C++ demangler implementations extracted from demumble/LLVM.

## Files

- **ItaniumDemangle.cpp** - Itanium C++ ABI demangler (GCC/Clang/ICC)
- **MicrosoftDemangle.cpp** - MSVC C++ ABI demangler
- **MicrosoftDemangleNodes.cpp** - MSVC demangler node types
- **Demangle.cpp** - Entry point wrappers

## Building

These files are **C++17 code** and require a C++ compiler:

```cmake
# CMake example
add_library(ntrtl_demangle_llvm STATIC
    ItaniumDemangle.cpp
    MicrosoftDemangle.cpp
    MicrosoftDemangleNodes.cpp
    Demangle.cpp
)

target_compile_features(ntrtl_demangle_llvm PRIVATE cxx_std_17)
target_include_directories(ntrtl_demangle_llvm PRIVATE
    ${ANANKE_ROOT}/libs/ntrtl/include/private
)
```

## C Wrapper

The C code in `../itanium/demangle.c` and `../msvc/demangle.c` provides simplified
pure-C implementations for environments without C++ support.

For full-featured demangling with C++ support, use these LLVM demanglers:

```c
// C++ code
extern "C" {
    #include <ananke/ntrtl.h>
}

#include "llvm/Demangle/Demangle.h"

extern "C" UINTN RtlDemangleNameItaniumLLVM(
    const CHAR8 *MangledName,
    CHAR8 *DemangledName,
    UINTN BufferSize
) {
    char *result = llvm::itaniumDemangle(MangledName);
    if (!result) return 0;

    UINTN len = strlen(result);
    if (len < BufferSize) {
        memcpy(DemangledName, result, len + 1);
    }
    free(result);
    return len < BufferSize ? len : 0;
}
```

## License

Apache-2.0 WITH LLVM-exception (see LICENSE file in LLVM source)

## Source

Extracted from: https://github.com/nico/demumble
Original source: LLVM Project
