# NUX Kernel Coding Style

This document describes the coding style for the NUX kernel, based on Windows NT kernel conventions and UEFI/EDK2 coding standards.

## Automated Tools

The NUX kernel provides automated tools to enforce and check coding style:

### clang-format

Automatically formats code according to NT kernel style.

**Configuration:** `.clang-format` in repository root

**Usage:**
```bash
# Format a single file
clang-format -i path/to/file.c

# Format all files in a directory
find nux/libs -name "*.c" -o -name "*.h" | xargs clang-format -i

# Check formatting without modifying files
clang-format --dry-run --Werror path/to/file.c
```

**Key formatting rules:**
- 4-space indentation (no tabs)
- Allman/BSD brace style (opening brace on new line)
- Return type on separate line for functions
- Space before opening parenthesis: `FunctionName (`
- Parameters on separate lines for long declarations
- Closing parenthesis on new line

### clang-tidy

Performs static analysis and enforces naming conventions.

**Configuration:** `.clang-tidy` in repository root

**Usage:**
```bash
# Check a single file
clang-tidy path/to/file.c -- -I. -I./include

# Check and auto-fix a single file
clang-tidy path/to/file.c --fix --fix-errors -- -I. -I./include

# Check entire codebase
./scripts/run-clang-tidy.sh

# Check and auto-fix entire codebase
./scripts/run-clang-tidy.sh . --fix

# Quick check of modified files only
./scripts/quick-tidy.sh

# Quick fix modified files
./scripts/quick-tidy.sh --fix
```

**Checks enabled:**
- Naming convention enforcement
- Bug detection (assignment in if, null dereferences, etc.)
- Performance issues
- Portability problems
- Code readability

## Naming Conventions

### Functions
- **Style:** PascalCase
- **Examples:** `CpuInitialize`, `KmemBrkGrow`, `PfnGet`
- **Exception:** Standard C library functions (printf, memcpy, etc.)

### Macros
- **Style:** UPPER_SNAKE_CASE
- **Examples:** `MAX_VALUE`, `FOREACH_CPUMASK`, `PAGE_SIZE`
- **Exception:** Attribute macros like `IN`, `OUT`, `OPTIONAL`

### Types (struct, union, enum, typedef)
- **Style:** PascalCase or UPPER_CASE
- **Examples:** `CPU_INFO`, `UINT32`, `VIRTUAL_ADDRESS`
- **Struct tags:** Often prefixed with underscore: `struct _CPU_INFO`

### Variables

#### Global variables
- **Style:** gPascalCase or UPPER_CASE
- **Examples:** `gNumberCpus`, `gCpuPhysToId`, `PAGE_SIZE`

#### Static variables
- **Style:** gPascalCase or sPascalCase
- **Examples:** `gCpusActive`, `sTlbGeneration`

#### Local variables
- **Style:** PascalCase or camelCase
- **Examples:** `CpuInfo`, `Id`, `pfn`

#### Parameters
- **Style:** PascalCase (UEFI convention)
- **Examples:** `PhysId`, `Umap`, `Va`

### Constants
- **Style:** UPPER_CASE or kPascalCase
- **Examples:** `HAL_MAXCPUS`, `PLT_PCPU_INVALID`

## Code Structure

### Function Declarations

NT kernel style with UEFI documentation:

```c
/**
  Add a CPU to the system.

  Requires NUXST_OKPLT status.

  @param[in] PhysId  Physical CPU ID.

  @return Logical CPU ID, or -1 on error.
**/
static INT32
CpuAdd (
  IN UINT16  PhysId
  )
{
  // Function body
}
```

Key elements:
1. Doxygen comment block with `@param` and `@return`
2. Return type on separate line
3. Space before opening parenthesis
4. Parameters with SAL-lite annotations (`IN`, `OUT`, `OPTIONAL`)
5. Parameters aligned in columns (type and name)
6. Closing parenthesis on new line, indented
7. Opening brace on new line

### Control Structures

```c
if (Condition)
{
    Statement;
}
else
{
    OtherStatement;
}

for (Index = 0; Index < Count; Index++)
{
    Statement;
}

while (Condition)
{
    Statement;
}
```

### Switch Statements

```c
switch (Value)
{
case 1:
    Statement;
    break;
case 2:
    OtherStatement;
    break;
default:
    DefaultStatement;
    break;
}
```

### Struct Definitions

```c
///
/// CPU information structure.
///
typedef struct _CPU_INFO
{
    ///
    /// Logical CPU ID.
    ///
    INT32    CpuId;

    ///
    /// Physical CPU ID.
    ///
    UINT16   PhysId;

    ///
    /// Self-pointer for easy access.
    ///
    struct _CPU_INFO *self;
} CPU_INFO;
```

## SAL-lite Annotations

Use SAL-lite annotations for function parameters:

- `IN` - Input parameter (read-only)
- `OUT` - Output parameter (write-only, must be written before return)
- `IN OUT` - Parameter is both read and written
- `OPTIONAL` - Parameter may be NULL

Example:
```c
NTSTATUS
DoSomething (
  IN     UINT32   Input,
  OUT    UINT32   *Result,
  IN OUT UINT32   *Counter OPTIONAL
  )
```

## UEFI Types

Use UEFI integer types instead of standard C types:

| C Type              | UEFI Type    |
|---------------------|--------------|
| unsigned char       | UINT8        |
| unsigned short      | UINT16       |
| unsigned int        | UINT32       |
| unsigned long long  | UINT64       |
| size_t              | UINTN        |
| signed char         | INT8         |
| signed short        | INT16        |
| signed int          | INT32        |
| signed long long    | INT64        |
| ssize_t             | INTN         |
| char                | CHAR8        |
| wchar_t             | CHAR16       |
| void                | VOID         |
| bool                | BOOLEAN      |

## Common Mistakes to Avoid

1. **Using tabs instead of spaces** - Always use 4 spaces
2. **Incorrect function naming** - Use PascalCase, not snake_case
3. **Missing SAL annotations** - Always annotate parameters
4. **Wrong brace style** - Opening brace must be on new line
5. **Assignment in if condition** - Move assignment outside: `Value = Get(); if (Value != 0)`
6. **Missing NULL checks** - Always validate pointer parameters
7. **Magic numbers** - Use named constants or macros

## Pre-commit Workflow

Before committing code:

1. **Format your code:**
   ```bash
   clang-format -i path/to/modified/file.c
   ```

2. **Run static analysis:**
   ```bash
   ./scripts/quick-tidy.sh
   ```

3. **Fix any warnings:**
   ```bash
   ./scripts/quick-tidy.sh --fix
   ```

4. **Review changes** and ensure auto-fixes are correct

5. **Commit** with descriptive message

## Resources

- [UEFI EDK II C Coding Standards](https://github.com/tianocore/tianocore.github.io/wiki/Code-Style-C)
- [Windows NT kernel source](https://github.com/microsoft/Windows-Driver-Frameworks)
- clang-format documentation: https://clang.llvm.org/docs/ClangFormat.html
- clang-tidy documentation: https://clang.llvm.org/extra/clang-tidy/
