# VINIL Coding Style Guide

This document describes the coding style used in the VINIL library to match
the NUX/Ananke codebase conventions.

## File Headers

Use Doxygen-style file documentation:

```c
/** @file
  Brief module description

  Detailed description of the module's purpose and functionality.

  Copyright (C) 2003-2007 Hans-Martin Will
  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/
```

## Header Guards

Use double-underscore format with full path:

```c
#ifndef __vinil_modulename_h__
#define __vinil_modulename_h__ 1
...
#endif // __vinil_modulename_h__
```

## Section Comments

Use simple double-slash style:

```c
//
// Section Name
//
```

## Function Documentation

Use Doxygen format with proper parameter documentation:

```c
/**
  Brief function description.

  Detailed description of what the function does, any important notes
  about usage or behavior.

  @param[in]   param1  Description of input parameter.
  @param[out]  param2  Description of output parameter.
  @param[in,out] param3  Description of in/out parameter.

  @return  Description of return value.
**/
RETURN_TYPE
FunctionName (
  TYPE1  Param1,
  TYPE2  *Param2,
  TYPE3  *Param3
  );
```

## Naming Conventions

### Types
- Library types use `vinil_` prefix with lowercase and underscores
- Examples: `vinil_context`, `vinil_uint32`, `vinil_error`
- This avoids namespace pollution unlike bare UPPERCASE types

### Functions
- Public API: `vinil_module_action` (all lowercase with underscores)
- Internal/static: Same pattern but not exported
- Examples: `vinil_context_create`, `vinil_program_compile`

### Constants and Macros
- UPPERCASE with underscores
- Examples: `VINIL_VERSION_MAJOR`, `VINIL_DEFAULT_PAGE_SIZE`

### Variables
- Local variables: lowercase with underscores
- Global variables: `g` prefix (internal only, avoid in public API)

## Indentation and Formatting

- **Indentation**: 2 spaces (no tabs)
- **Line length**: 80 characters preferred, 100 maximum
- **Braces**: Opening brace on same line for blocks, new line for functions
- **Spaces**: Space after keywords (`if`, `for`, `while`), no space after function names

## Function Declarations

Multi-line format with parameters aligned:

```c
vinil_error
vinil_program_compile (
  vinil_context  *ctx,
  vinil_program  *program,
  vinil_bool     use_jit
  );
```

## Comments

- Use `//` for single-line comments
- Use `/* ... */` for inline comments
- Use `/** ... **/` for documentation blocks
- Align inline comments when in groups

## Error Handling

- Return `vinil_error` enum for functions that can fail
- Use error codes defined in public API
- Set context error message for detailed diagnostics

## Memory Management

- All allocations through vinil_memory_pool
- No raw malloc/free in IL construction code
- JIT and interpreter may use standard allocation

## License

All files include SPDX license identifier:
```
SPDX-License-Identifier:    CDDL-1.0
```

## Rationale

The VINIL library uses a hybrid style:
- **File/function documentation**: Doxygen format (matches NUX/Ananke)
- **Type naming**: Prefixed lowercase (standard C library practice)
- **Section comments**: `//` style (matches Ananke framebuffer)
- **Header guards**: Double-underscore format (matches NUX HAL)

This balances consistency with the NUX codebase while maintaining
portability and compatibility with external APIs (OpenCL, CUDA, HIP).
