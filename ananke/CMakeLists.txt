cmake_minimum_required(VERSION 3.15)

# ANANKE - Common Foundation Library
# Provides portable, compiler-agnostic foundation layer for NUX and APXH

project(Ananke
    LANGUAGES C
    VERSION 1.0.0
    DESCRIPTION "Common foundation library for NUX kernel and APXH bootloader")

# Options
option(ANANKE_BUILD_SHARED "Build shared library" OFF)
option(ANANKE_BUILD_STATIC "Build static library" ON)
option(ANANKE_BUILD_NTRTL "Build NT Runtime Library components" ON)
option(ANANKE_INSTALL "Install Ananke headers and library" ON)

# Set module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake/modules")

# Detect compiler if modules are available
if(EXISTS "${CMAKE_SOURCE_DIR}/../cmake/modules/NuxCompilerSetup.cmake")
    include(NuxCompilerSetup)
    nux_detect_compiler()
else()
    set(NUX_COMPILER_VENDOR ${CMAKE_C_COMPILER_ID})
endif()

# Print configuration
message(STATUS "=== ANANKE Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Compiler: ${NUX_COMPILER_VENDOR}")
message(STATUS "Build NTRTL: ${ANANKE_BUILD_NTRTL}")
message(STATUS "Build shared: ${ANANKE_BUILD_SHARED}")
message(STATUS "Build static: ${ANANKE_BUILD_STATIC}")
message(STATUS "===========================")

# Ananke is primarily a header-only library
add_library(ananke_headers INTERFACE)
target_include_directories(ananke_headers INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Alias for consistent naming
add_library(Ananke::Headers ALIAS ananke_headers)

# NT Runtime Library (NTRTL) - optional compiled components
if(ANANKE_BUILD_NTRTL)
    set(NTRTL_SOURCES
        sources/ntrtl/bitmap.c
        sources/ntrtl/memory.c
        sources/ntrtl/utility.c
        sources/ntrtl/tree.c
        sources/ntrtl/string.c
        sources/ntrtl/list.c
    )

    # Static library
    if(ANANKE_BUILD_STATIC)
        add_library(ananke_ntrtl_static STATIC ${NTRTL_SOURCES})
        target_include_directories(ananke_ntrtl_static PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        )
        set_target_properties(ananke_ntrtl_static PROPERTIES
            OUTPUT_NAME ananke_ntrtl
            POSITION_INDEPENDENT_CODE ON
        )
        add_library(Ananke::NTRTL ALIAS ananke_ntrtl_static)
    endif()

    # Shared library
    if(ANANKE_BUILD_SHARED)
        add_library(ananke_ntrtl_shared SHARED ${NTRTL_SOURCES})
        target_include_directories(ananke_ntrtl_shared PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        )
        set_target_properties(ananke_ntrtl_shared PROPERTIES
            OUTPUT_NAME ananke_ntrtl
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}
        )
        if(NOT TARGET Ananke::NTRTL)
            add_library(Ananke::NTRTL ALIAS ananke_ntrtl_shared)
        endif()
    endif()
endif()

# Installation
if(ANANKE_INSTALL)
    include(GNUInstallDirs)

    # Install headers
    install(DIRECTORY include/ananke
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h")

    # Install libraries
    if(ANANKE_BUILD_NTRTL)
        if(ANANKE_BUILD_STATIC)
            install(TARGETS ananke_ntrtl_static
                EXPORT AnankeTargets
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
        endif()
        if(ANANKE_BUILD_SHARED)
            install(TARGETS ananke_ntrtl_shared
                EXPORT AnankeTargets
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
        endif()
    endif()

    # Install header-only interface
    install(TARGETS ananke_headers
        EXPORT AnankeTargets)

    # Create and install CMake package files
    include(CMakePackageConfigHelpers)

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/AnankeConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/AnankeConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/AnankeConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Ananke
    )

    install(EXPORT AnankeTargets
        FILE AnankeTargets.cmake
        NAMESPACE Ananke::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Ananke)

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/AnankeConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/AnankeConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Ananke)
endif()

# Export for use from build tree
export(EXPORT AnankeTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/AnankeTargets.cmake"
    NAMESPACE Ananke::)

# Package configuration
set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)
export(PACKAGE Ananke)
