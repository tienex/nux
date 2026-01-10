cmake_minimum_required(VERSION 3.15)

# Multi-Project Build Coordinator
# This repository contains three independent projects:
#   1. Ananke - Foundation library (no dependencies)
#   2. APXH   - Bootloader (depends on Ananke)
#   3. NUX    - Kernel framework (depends on Ananke)

project(NUX-Suite
    LANGUAGES C CXX ASM
    VERSION 0.1.0
    DESCRIPTION "NUX Kernel Suite: Ananke + APXH + NUX")

# Set module path for custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# Options for which projects to build
option(BUILD_ANANKE "Build Ananke foundation library" ON)
option(BUILD_APXH "Build APXH bootloader" ON)
option(BUILD_NUX "Build NUX kernel framework" ON)
option(BUILD_ALL_PROJECTS "Build all projects in dependency order" ON)

# Global options
option(NUX_ENABLE_TESTS "Enable testing for all projects" OFF)
option(NUX_USE_LINKER_SCRIPTS "Use traditional linker scripts (requires GNU LD)" OFF)

# Target architectures (can specify multiple for universal binaries)
set(NUX_TARGETS "i386" CACHE STRING "Target architectures (semicolon-separated: i386;amd64;riscv64)")
set(NUX_IMAGE_FORMAT "elf" CACHE STRING "Image format: elf, macho, or pecoff")
set_property(CACHE NUX_IMAGE_FORMAT PROPERTY STRINGS elf macho pecoff)

# Validate image format and target combination
list(LENGTH NUX_TARGETS target_count)
if(target_count GREATER 1)
    if(NUX_IMAGE_FORMAT STREQUAL "pecoff")
        message(FATAL_ERROR
            "PE/COFF format does not support multiple targets (universal binaries).\n"
            "Only ELF and Mach-O formats support universal binaries.\n"
            "Please specify a single target in NUX_TARGETS or use 'elf' or 'macho' format.")
    endif()
    set(NUX_UNIVERSAL_BINARY ON)
    message(STATUS "Building universal binary for targets: ${NUX_TARGETS}")
else()
    set(NUX_UNIVERSAL_BINARY OFF)
    message(STATUS "Building single-target binary for: ${NUX_TARGETS}")
endif()

# Include compiler detection and setup
include(NuxCompilerSetup)
include(NuxToolchainAbstraction)
include(NuxSectionGeneration)

# Detect and configure compiler
nux_detect_compiler()
nux_setup_compiler_flags()

# Print configuration summary
message(STATUS "")
message(STATUS "╔═══════════════════════════════════════════════════════╗")
message(STATUS "║       NUX Suite Multi-Project Build Configuration     ║")
message(STATUS "╠═══════════════════════════════════════════════════════╣")
message(STATUS "║ Compiler vendor:   ${NUX_COMPILER_VENDOR}")
message(STATUS "║ Compiler ID:       ${CMAKE_C_COMPILER_ID}")
message(STATUS "║ Compiler version:  ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "║ Target archs:      ${NUX_TARGETS}")
message(STATUS "║ Image format:      ${NUX_IMAGE_FORMAT}")
message(STATUS "║ Universal binary:  ${NUX_UNIVERSAL_BINARY}")
message(STATUS "║ Use linker scripts: ${NUX_USE_LINKER_SCRIPTS}")
message(STATUS "╠═══════════════════════════════════════════════════════╣")
message(STATUS "║ Projects to build:                                    ║")
message(STATUS "║   • Ananke:        ${BUILD_ANANKE}")
message(STATUS "║   • APXH:          ${BUILD_APXH}")
message(STATUS "║   • NUX:           ${BUILD_NUX}")
message(STATUS "╚═══════════════════════════════════════════════════════╝")
message(STATUS "")

# Build projects in dependency order

# 1. Build Ananke first (no dependencies)
if(BUILD_ANANKE)
    message(STATUS "")
    message(STATUS ">>> Building Ananke (Foundation Library)")
    add_subdirectory(ananke)
    message(STATUS "<<< Ananke configured")
endif()

# 2. Build APXH (depends on Ananke)
if(BUILD_APXH)
    if(NOT BUILD_ANANKE AND NOT TARGET Ananke::Headers)
        message(WARNING
            "APXH requires Ananke, but BUILD_ANANKE is OFF.\n"
            "Attempting to find installed Ananke...")
        find_package(Ananke REQUIRED)
    endif()
    message(STATUS "")
    message(STATUS ">>> Building APXH (Bootloader)")
    set(APXH_TARGETS ${NUX_TARGETS})
    set(APXH_IMAGE_FORMAT ${NUX_IMAGE_FORMAT})
    add_subdirectory(apxh)
    message(STATUS "<<< APXH configured")
endif()

# 3. Build NUX (depends on Ananke)
if(BUILD_NUX)
    if(NOT BUILD_ANANKE AND NOT TARGET Ananke::Headers)
        message(WARNING
            "NUX requires Ananke, but BUILD_ANANKE is OFF.\n"
            "Attempting to find installed Ananke...")
        find_package(Ananke REQUIRED)
    endif()
    message(STATUS "")
    message(STATUS ">>> Building NUX (Kernel Framework)")
    add_subdirectory(nux)
    message(STATUS "<<< NUX configured")
endif()

# Testing
if(NUX_ENABLE_TESTS)
    enable_testing()
    if(EXISTS "${CMAKE_SOURCE_DIR}/tests")
        add_subdirectory(tests)
    endif()
endif()

# Installation
install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h")

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "")
message(STATUS "Configuration complete! Build with:")
message(STATUS "  cmake --build .")
message(STATUS "")
