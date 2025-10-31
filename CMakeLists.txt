cmake_minimum_required(VERSION 3.15)

project(NUX
    LANGUAGES C CXX ASM
    VERSION 0.1.0
    DESCRIPTION "NUX: A kernel framework for modern hardware")

# Set module path for custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# Options for build configuration
option(NUX_BUILD_APXH "Build APXH bootloader" ON)
option(NUX_BUILD_LIBS "Build NUX libraries" ON)
option(NUX_BUILD_EXAMPLE "Build example kernel" ON)
option(NUX_BUILD_TOOLS "Build NUX tools" ON)
option(NUX_ENABLE_TESTS "Enable testing" OFF)
option(NUX_USE_LINKER_SCRIPTS "Use traditional linker scripts (disable for maximum compiler compatibility)" OFF)

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
message(STATUS "=== NUX Build Configuration ===")
message(STATUS "Compiler vendor: ${NUX_COMPILER_VENDOR}")
message(STATUS "Compiler ID: ${CMAKE_C_COMPILER_ID}")
message(STATUS "Compiler version: ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "Target architectures: ${NUX_TARGETS}")
message(STATUS "Image format: ${NUX_IMAGE_FORMAT}")
message(STATUS "Universal binary: ${NUX_UNIVERSAL_BINARY}")
message(STATUS "Use linker scripts: ${NUX_USE_LINKER_SCRIPTS}")
message(STATUS "==============================")

# Set up include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/ananke/include
    ${CMAKE_BINARY_DIR}/include
)

# Build tools first (needed for code generation)
if(NUX_BUILD_TOOLS)
    add_subdirectory(nux/tools)
endif()

# Build libraries
if(NUX_BUILD_LIBS)
    add_subdirectory(nux/libs/ecrt)
    add_subdirectory(nux/libs/hal)
    add_subdirectory(nux/libs/nux)
    add_subdirectory(nux/libs/platform)
    add_subdirectory(nux/libs/unux)
    add_subdirectory(nux/libs/contrib/fdt)
endif()

# Build APXH bootloader
if(NUX_BUILD_APXH)
    add_subdirectory(apxh)
endif()

# Build example kernel
if(NUX_BUILD_EXAMPLE)
    add_subdirectory(nux/example)
endif()

# Testing
if(NUX_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h")

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Generate build configuration header
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/config.h.in
    ${CMAKE_BINARY_DIR}/include/nux/build_config.h
)
