# ANXCONFIG - Ananke Configuration System

## Overview

ANXCONFIG is a portable, cross-platform configuration system similar to Linux's kconfig/menuconfig, designed for use with Ananke, APXH, and NUX projects. It uses YAML files to define configuration options and provides an interactive TUI (Text User Interface) for configuration, generating output in multiple formats.

## Features

### Core Capabilities

- **YAML-based Configuration**: Define configuration options in human-readable YAML files
- **Interactive TUI**: menuconfig-like interface (portable across all platforms)
- **Multiple Output Formats**: Generate CMake cache, C headers, Makefiles, and autoconf files
- **Dependency Resolution**: Automatic evaluation of conditional dependencies
- **COM Architecture**: Uses Ananke's COM interfaces for extensibility
- **Cross-Platform**: Works on Windows, Linux, macOS, BSD, and embedded systems
- **Compiler-Agnostic**: Builds with GCC, Clang, MSVC, TCC, PCC, and others

### Configuration Item Types

| Type | Description | Example |
|------|-------------|---------|
| `boolean` | ON/OFF toggle | Enable feature (y/n) |
| `tristate` | YES/NO/MODULE | Kernel module options |
| `choice` | Radio button selection | Scheduler type |
| `string` | Text input | Installation path |
| `integer` | Numeric input | Maximum CPUs |
| `hex` | Hexadecimal input | Memory address |
| `menu` | Submenu container | Group related options |
| `separator` | Visual divider | Section break |
| `comment` | Information text | Help text |

## Usage

### Command Line Interface

```bash
# Interactive configuration menu
anxconfig anxconfig.yaml

# Load existing configuration
anxconfig -l .config anxconfig.yaml

# Save configuration
anxconfig -s .config anxconfig.yaml

# Generate CMake cache file
anxconfig -c CMakeCache.txt anxconfig.yaml

# Generate C header
anxconfig -H config.h anxconfig.yaml

# Generate Makefile fragment
anxconfig -M config.mk anxconfig.yaml

# Generate autoconf fragment
anxconfig -a config.ac anxconfig.yaml

# Load defaults
anxconfig -d anxconfig.yaml

# Combined workflow
anxconfig -l .config -c CMakeCache.txt -H config.h anxconfig.yaml
```

### Options

| Option | Description |
|--------|-------------|
| `-m, --menuconfig` | Run interactive menu (default) |
| `-l, --load <file>` | Load saved configuration |
| `-s, --save <file>` | Save configuration |
| `-c, --cmake <file>` | Generate CMake cache |
| `-H, --header <file>` | Generate C header |
| `-M, --makefile <file>` | Generate Makefile |
| `-a, --autoconf <file>` | Generate autoconf |
| `-d, --defconfig` | Load default configuration |
| `-h, --help` | Show help message |

## YAML Configuration Format

### Basic Structure

```yaml
mainmenu: "Project Configuration"

config:
  - name: FEATURE_NAME
    type: boolean
    prompt: "Enable feature"
    default: true
    help: |
      Detailed help text for this feature.
      Can span multiple lines.

menu:
  title: "Submenu Title"
  items:
    - name: SUBMENU_OPTION
      type: integer
      prompt: "Option value"
      default: 100
      range: [0, 1000]
      help: "Help text"
```

### Configuration Options

#### Boolean Option

```yaml
- name: CONFIG_FEATURE_X
  type: boolean
  prompt: "Enable feature X"
  default: true
  depends: CONFIG_SOME_OTHER_FEATURE
  help: |
    Enable or disable feature X.
    Requires CONFIG_SOME_OTHER_FEATURE to be enabled.
```

#### Choice Option

```yaml
- name: CONFIG_SCHEDULER
  type: choice
  prompt: "Scheduler type"
  default: SCHED_CFS
  choices:
    - name: SCHED_SIMPLE
      prompt: "Simple round-robin"
    - name: SCHED_CFS
      prompt: "Completely Fair Scheduler"
    - name: SCHED_REALTIME
      prompt: "Real-time scheduler"
  help: "Select the process scheduling algorithm"
```

#### Integer Option

```yaml
- name: CONFIG_MAX_CPUS
  type: integer
  prompt: "Maximum number of CPUs"
  default: 64
  range: [1, 256]
  depends: CONFIG_SMP
  help: "Maximum number of CPUs the kernel can handle"
```

#### Hexadecimal Option

```yaml
- name: CONFIG_KERNEL_BASE
  type: hex
  prompt: "Kernel base address"
  default: 0xC0000000
  help: "Virtual address where kernel is mapped"
```

#### String Option

```yaml
- name: CONFIG_VERSION_STRING
  type: string
  prompt: "Version string"
  default: "1.0.0"
  help: "Project version string"
```

#### Menu

```yaml
menu:
  title: "Advanced Options"
  depends: CONFIG_EXPERT_MODE
  items:
    - name: CONFIG_ADVANCED_FEATURE
      type: boolean
      prompt: "Advanced feature"
      default: false
      help: "Enable advanced feature"
```

#### Separator and Comment

```yaml
separator: "Section Title"

comment: |
  This is informational text that appears in the menu.
  It can explain complex options or provide warnings.
```

### Dependencies

Dependencies use simple expressions:

```yaml
# Simple dependency
depends: CONFIG_FOO

# Logical AND
depends: CONFIG_FOO && CONFIG_BAR

# Logical OR
depends: CONFIG_FOO || CONFIG_BAR

# Logical NOT
depends: !CONFIG_FOO

# Comparison
depends: CONFIG_MAX_CPUS > 1

# Complex expression
depends: (CONFIG_FOO || CONFIG_BAR) && !CONFIG_BAZ
```

## Output Formats

### CMake Cache File

Generated with `-c` option:

```cmake
# ANXCONFIG Generated CMake Cache
set(CONFIG_FEATURE_X ON CACHE BOOL "Enable feature X")
set(CONFIG_MAX_CPUS 64 CACHE STRING "Maximum CPUs")
set(CONFIG_KERNEL_BASE 0xC0000000 CACHE STRING "Kernel base address")
```

Usage in CMakeLists.txt:

```cmake
# Load configuration
include(config.cmake)

if(CONFIG_FEATURE_X)
    add_definitions(-DFEATURE_X_ENABLED)
endif()

add_compile_definitions(MAX_CPUS=${CONFIG_MAX_CPUS})
```

### C Header File

Generated with `-H` option:

```c
/* ANXCONFIG Generated Configuration Header */
#ifndef __CONFIG_H__
#define __CONFIG_H__

#define CONFIG_FEATURE_X 1
#define CONFIG_MAX_CPUS 64
#define CONFIG_KERNEL_BASE 0xC0000000UL

#endif /* __CONFIG_H__ */
```

Usage in C code:

```c
#include "config.h"

#ifdef CONFIG_FEATURE_X
    /* Feature X code */
#endif

int max_cpus = CONFIG_MAX_CPUS;
```

### Makefile Fragment

Generated with `-M` option:

```make
# ANXCONFIG Generated Makefile Fragment
CONFIG_FEATURE_X := y
CONFIG_MAX_CPUS := 64
CONFIG_KERNEL_BASE := 0xC0000000
```

Usage in Makefile:

```make
include config.mk

ifeq ($(CONFIG_FEATURE_X),y)
    CFLAGS += -DFEATURE_X_ENABLED
endif

CFLAGS += -DMAX_CPUS=$(CONFIG_MAX_CPUS)
```

### Autoconf Fragment

Generated with `-a` option:

```m4
# ANXCONFIG Generated Autoconf Fragment
AC_DEFINE([CONFIG_FEATURE_X], [1], [Enable feature X])
AC_DEFINE([CONFIG_MAX_CPUS], [64], [Maximum CPUs])
AC_DEFINE([CONFIG_KERNEL_BASE], [0xC0000000], [Kernel base address])
```

## Integration

### With CMake Projects

```cmake
# In CMakeLists.txt
find_program(ANXCONFIG anxconfig)

if(ANXCONFIG)
    # Run interactive configuration
    add_custom_target(menuconfig
        COMMAND ${ANXCONFIG} ${CMAKE_SOURCE_DIR}/anxconfig.yaml
        COMMENT "Running ANXCONFIG menuconfig"
    )

    # Generate CMake cache
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/config.cmake
        COMMAND ${ANXCONFIG} -s .config -c config.cmake ${CMAKE_SOURCE_DIR}/anxconfig.yaml
        DEPENDS ${CMAKE_SOURCE_DIR}/anxconfig.yaml
        COMMENT "Generating configuration"
    )

    # Include generated configuration
    if(EXISTS ${CMAKE_BINARY_DIR}/config.cmake)
        include(${CMAKE_BINARY_DIR}/config.cmake)
    endif()
endif()

# User can now run: cmake --build . --target menuconfig
```

### With Autoconf Projects

```bash
#!/bin/bash
# bootstrap.sh

# Run configuration
anxconfig -s .config anxconfig.yaml

# Generate autoconf fragment
anxconfig -a config.ac anxconfig.yaml

# Run autoconf
autoconf
```

In configure.ac:

```m4
AC_INIT([MyProject], [1.0])

# Include ANXCONFIG generated configuration
m4_include([config.ac])

AC_OUTPUT
```

### With Plain Makefiles

```make
# Makefile

# Generate configuration if not exists
.config:
	anxconfig anxconfig.yaml

config.mk: .config
	anxconfig -s .config -M config.mk anxconfig.yaml

# Include configuration
-include config.mk

# Use configuration
all: config.mk
	$(CC) $(CFLAGS) -DMAX_CPUS=$(CONFIG_MAX_CPUS) main.c
```

## Examples

### Ananke Configuration

See [`ananke/anxconfig.yaml`](anxconfig.yaml) for the Ananke foundation library configuration.

Features configured:
- NT Runtime Library building
- Shared/static library selection
- Memory management options
- Debug levels
- Compiler compatibility options

### APXH Configuration

See [`apxh/anxconfig.yaml`](../apxh/anxconfig.yaml) for the APXH bootloader configuration.

Features configured:
- Target architectures (i386/amd64/riscv64)
- Platform support (Multiboot/UEFI/SBI)
- Image format loaders (ELF/Mach-O/PE)
- Boot features (framebuffer, device tree, ACPI)
- Memory configuration
- Debug options

### NUX Configuration

See [`nux/anxconfig.yaml`](../nux/anxconfig.yaml) for the NUX kernel configuration.

Features configured:
- Target architectures
- Build components (HAL/PLT/ECRT)
- Kernel features (SMP, NUMA, preemption)
- Memory management (page size, heap, slab)
- Scheduling algorithms
- Debugging and diagnostics
- Security features (NX, ASLR, stack canaries)

## Architecture

### COM Interface Design

ANXCONFIG uses Ananke's COM architecture for extensibility:

```
IConfigDatabase
├── LoadFromFile() - Parse YAML
├── LoadValues() - Load .config
├── SaveValues() - Save .config
├── GetRootItem() - Get root menu
├── FindItem() - Find by name
└── EvaluateDependencies() - Resolve deps

IConfigItem
├── GetType() - Item type
├── GetName() - Symbol name
├── GetPrompt() - Display text
├── GetValue() - Current value
├── SetValue() - Set value
└── IsVisible() - Dependency check

IConfigGenerator
├── GenerateCMakeCache()
├── GenerateCHeader()
├── GenerateMakefile()
└── GenerateAutoconf()

ITuiScreen (TUI abstraction)
ITuiWindow (TUI windows)
ITuiMenu (TUI menus)
```

### Portable TUI Layer

The TUI is abstracted to work across all platforms:

- **Unix/Linux**: VT100/ANSI escape codes
- **Windows**: Windows Console API
- **Embedded**: Simple text mode

All accessed through COM interfaces for portability.

## Building

### From Source

```bash
cd ananke
cmake -B build -DANANKE_BUILD_TOOLS=ON
cmake --build build
./build/bin/anxconfig --help
```

### Installation

```bash
cd ananke/build
sudo cmake --install .

# Now anxconfig is in PATH
anxconfig --help
```

## Development Status

### Current Implementation

✅ **Completed**:
- COM interface definitions
- TUI API design
- YAML configuration format specification
- Command-line interface
- Example configuration files (Ananke, APXH, NUX)
- Stub implementations
- Generator placeholders

⚠️ **In Progress** (Stubs):
- YAML parser implementation
- Full TUI implementation
- Dependency expression evaluator
- Interactive menu rendering

### Future Enhancements

- [ ] Full YAML parser (lightweight, no external deps)
- [ ] Complete TUI implementation with color support
- [ ] Advanced dependency expressions
- [ ] Configuration validation
- [ ] Import/export to other formats (JSON, TOML)
- [ ] Search functionality in menu
- [ ] Configuration diff tool
- [ ] Web-based configuration UI
- [ ] Integration with IDE plugins

## Contributing

When adding configuration options:

1. Use clear, descriptive prompts
2. Provide comprehensive help text
3. Set sensible defaults
4. Use dependencies to hide irrelevant options
5. Group related options in menus
6. Test on multiple platforms

## See Also

- [Ananke Foundation Library](README.md)
- [Multi-Project Guide](../MULTI_PROJECT_GUIDE.md)
- [CMake Build System](../CMAKE_BUILD_SYSTEM.md)
- [Linux Kconfig](https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html) - Inspiration
