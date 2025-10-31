cmake_minimum_required(VERSION 3.15)

# NUX - Kernel Framework
# Depends on Ananke foundation library

project(NUX
    LANGUAGES C CXX ASM
    VERSION 0.1.0
    DESCRIPTION "NUX kernel framework")

# Options
option(NUX_BUILD_LIBS "Build NUX libraries" ON)
option(NUX_BUILD_HAL "Build Hardware Abstraction Layer" ON)
option(NUX_BUILD_PLATFORM "Build Platform Layer" ON)
option(NUX_BUILD_ECRT "Build embedded C runtime" ON)
option(NUX_BUILD_EXAMPLE "Build example kernel" ON)
option(NUX_BUILD_TOOLS "Build NUX tools" ON)

# Set module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake/modules")

# Include compiler setup
if(EXISTS "${CMAKE_SOURCE_DIR}/../cmake/modules/NuxCompilerSetup.cmake")
    include(NuxCompilerSetup)
    nux_detect_compiler()
    nux_setup_compiler_flags()
endif()

# Target architectures
if(NOT DEFINED NUX_TARGETS)
    set(NUX_TARGETS "i386" CACHE STRING "Target architectures (semicolon-separated)")
endif()

# Image format
if(NOT DEFINED NUX_IMAGE_FORMAT)
    set(NUX_IMAGE_FORMAT "elf" CACHE STRING "Image format: elf, macho, or pecoff")
endif()

# Find Ananke dependency
find_package(Ananke REQUIRED
    HINTS "${CMAKE_SOURCE_DIR}/../ananke"
          "${CMAKE_SOURCE_DIR}/../ananke/build"
    NO_DEFAULT_PATH)

if(NOT Ananke_FOUND)
    message(FATAL_ERROR
        "Ananke not found. Please build Ananke first:\n"
        "  cd ${CMAKE_SOURCE_DIR}/../ananke\n"
        "  cmake -B build && cmake --build build")
endif()

# Validate multi-target configuration
list(LENGTH NUX_TARGETS target_count)
if(target_count GREATER 1)
    if(NUX_IMAGE_FORMAT STREQUAL "pecoff")
        message(FATAL_ERROR
            "PE/COFF format does not support multiple targets (universal binaries).\n"
            "Only ELF and Mach-O formats support universal binaries.\n"
            "Please specify a single target in NUX_TARGETS or use 'elf' or 'macho' format.")
    endif()
    set(NUX_UNIVERSAL_BINARY ON)
else()
    set(NUX_UNIVERSAL_BINARY OFF)
endif()

message(STATUS "=== NUX Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Compiler: ${NUX_COMPILER_VENDOR}")
message(STATUS "Targets: ${NUX_TARGETS}")
message(STATUS "Image format: ${NUX_IMAGE_FORMAT}")
message(STATUS "Universal binary: ${NUX_UNIVERSAL_BINARY}")
message(STATUS "Ananke: ${ANANKE_VERSION}")
message(STATUS "========================")

# Common include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_BINARY_DIR}/include
)

# Build tools first (needed for code generation)
if(NUX_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

# Build libraries
if(NUX_BUILD_LIBS)
    if(NUX_BUILD_ECRT)
        add_subdirectory(libs/ecrt)
    endif()
    if(NUX_BUILD_HAL)
        add_subdirectory(libs/hal)
    endif()
    if(NUX_BUILD_PLATFORM)
        add_subdirectory(libs/platform)
    endif()
    add_subdirectory(libs/nux)
    add_subdirectory(libs/unux)

    # Contributed libraries
    if(EXISTS "${CMAKE_SOURCE_DIR}/libs/contrib/fdt")
        add_subdirectory(libs/contrib/fdt)
    endif()
endif()

# Build example kernel
if(NUX_BUILD_EXAMPLE)
    if(EXISTS "${CMAKE_SOURCE_DIR}/example")
        add_subdirectory(example)
    endif()
endif()

# Installation
include(GNUInstallDirs)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/../include/nux
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h")

# Export configuration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Generate build configuration header
configure_file(
    ${CMAKE_SOURCE_DIR}/../cmake/config.h.in
    ${CMAKE_BINARY_DIR}/include/nux/build_config.h
)
