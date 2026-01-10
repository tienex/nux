cmake_minimum_required(VERSION 3.15)

# APXH - Boot Protocol Handler
# Bootloader for NUX kernel, depends on Ananke

project(APXH
    LANGUAGES C ASM
    VERSION 0.1.0
    DESCRIPTION "APXH bootloader for NUX kernel")

# Options
option(APXH_BUILD_MULTIBOOT "Build Multiboot platform" ON)
option(APXH_BUILD_UEFI "Build UEFI platform" ON)
option(APXH_BUILD_SBI "Build SBI platform (RISC-V)" ON)

# Set module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake/modules")

# Include compiler setup
if(EXISTS "${CMAKE_SOURCE_DIR}/../cmake/modules/NuxCompilerSetup.cmake")
    include(NuxCompilerSetup)
    nux_detect_compiler()
    nux_setup_compiler_flags()
endif()

# Target architectures
if(NOT DEFINED APXH_TARGETS)
    set(APXH_TARGETS "i386" CACHE STRING "Target architectures (semicolon-separated)")
endif()

# Image format
if(NOT DEFINED APXH_IMAGE_FORMAT)
    set(APXH_IMAGE_FORMAT "elf" CACHE STRING "Image format: elf, macho, or pecoff")
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

message(STATUS "=== APXH Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Compiler: ${NUX_COMPILER_VENDOR}")
message(STATUS "Targets: ${APXH_TARGETS}")
message(STATUS "Image format: ${APXH_IMAGE_FORMAT}")
message(STATUS "Ananke: ${ANANKE_VERSION}")
message(STATUS "=========================")

# Common include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include/public
    ${CMAKE_SOURCE_DIR}/include/private
    ${CMAKE_BINARY_DIR}/include
)

# Common sources
set(APXH_COMMON_SOURCES
    sources/common/main.c
    sources/common/elf.c
    sources/loader/elf.c
    sources/loader/fatelf.c
    sources/loader/macho.c
    sources/loader/pe.c
)

# Create APXH common library
add_library(apxh_common STATIC ${APXH_COMMON_SOURCES})
target_link_libraries(apxh_common PUBLIC Ananke::Headers)
target_include_directories(apxh_common PUBLIC
    ${CMAKE_SOURCE_DIR}/include/public
    ${CMAKE_SOURCE_DIR}/include/private
)

# Platform-specific builds
foreach(target ${APXH_TARGETS})
    message(STATUS "Configuring APXH for target: ${target}")

    # Determine which platforms to build for this target
    if(target STREQUAL "i386")
        if(APXH_BUILD_MULTIBOOT)
            add_subdirectory(sources/platform/multiboot)
        endif()
    elseif(target STREQUAL "amd64")
        if(APXH_BUILD_MULTIBOOT)
            add_subdirectory(sources/platform/multiboot)
        endif()
        if(APXH_BUILD_UEFI)
            add_subdirectory(sources/platform/uefi)
        endif()
    elseif(target STREQUAL "riscv64")
        if(APXH_BUILD_SBI)
            add_subdirectory(sources/platform/sbi)
        endif()
        if(APXH_BUILD_UEFI)
            add_subdirectory(sources/platform/uefi)
        endif()
    endif()
endforeach()

# Installation
include(GNUInstallDirs)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h")

# Export configuration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
