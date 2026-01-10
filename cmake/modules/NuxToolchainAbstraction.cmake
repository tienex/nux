# NuxToolchainAbstraction.cmake
# Provides compiler-agnostic macros for section attributes, alignment, etc.

# Generate compiler-specific attribute header
function(nux_generate_compiler_attributes)
    set(ATTR_HEADER "${CMAKE_BINARY_DIR}/include/nux/compiler_attrs.h")

    file(WRITE ${ATTR_HEADER} "/* Auto-generated compiler attributes */\n")
    file(APPEND ${ATTR_HEADER} "#ifndef NUX_COMPILER_ATTRS_H\n")
    file(APPEND ${ATTR_HEADER} "#define NUX_COMPILER_ATTRS_H\n\n")

    # Section attribute
    if(NUX_COMPILER_VENDOR MATCHES "gcc|clang|intel")
        file(APPEND ${ATTR_HEADER} "#define NUX_SECTION(name) __attribute__((section(name)))\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_ALIGNED(n) __attribute__((aligned(n)))\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_PACKED __attribute__((packed))\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_USED __attribute__((used))\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_NORETURN __attribute__((noreturn))\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_INLINE inline __attribute__((always_inline))\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_NOINLINE __attribute__((noinline))\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_WEAK __attribute__((weak))\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_CONSTRUCTOR __attribute__((constructor))\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_DESTRUCTOR __attribute__((destructor))\n")

    elseif(NUX_COMPILER_VENDOR MATCHES "msvc|clang-cl")
        file(APPEND ${ATTR_HEADER} "#define NUX_SECTION(name) __declspec(allocate(name))\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_ALIGNED(n) __declspec(align(n))\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_PACKED\n")  # Use #pragma pack instead
        file(APPEND ${ATTR_HEADER} "#define NUX_USED\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_NORETURN __declspec(noreturn)\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_INLINE __forceinline\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_NOINLINE __declspec(noinline)\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_WEAK __declspec(selectany)\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_CONSTRUCTOR\n")  # Use .CRT$XI* sections
        file(APPEND ${ATTR_HEADER} "#define NUX_DESTRUCTOR\n")

    elseif(NUX_COMPILER_VENDOR STREQUAL "sunstudio")
        file(APPEND ${ATTR_HEADER} "#define NUX_SECTION(name)\n")  # Limited support
        file(APPEND ${ATTR_HEADER} "#define NUX_ALIGNED(n) _Pragma(\"align n\")\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_PACKED _Pragma(\"pack(1)\")\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_USED\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_NORETURN\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_INLINE inline\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_NOINLINE\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_WEAK _Pragma(\"weak\")\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_CONSTRUCTOR\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_DESTRUCTOR\n")

    else()
        # Generic/minimal support
        file(APPEND ${ATTR_HEADER} "#define NUX_SECTION(name)\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_ALIGNED(n)\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_PACKED\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_USED\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_NORETURN\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_INLINE inline\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_NOINLINE\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_WEAK\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_CONSTRUCTOR\n")
        file(APPEND ${ATTR_HEADER} "#define NUX_DESTRUCTOR\n")
    endif()

    # Assembly syntax helper
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]86|x86_64|amd64")
        if(NUX_COMPILER_VENDOR MATCHES "gcc|clang")
            file(APPEND ${ATTR_HEADER} "#define NUX_ASM_SYNTAX_ATT 1\n")
        elseif(NUX_COMPILER_VENDOR MATCHES "msvc|intel|watcom")
            file(APPEND ${ATTR_HEADER} "#define NUX_ASM_SYNTAX_INTEL 1\n")
        endif()
    endif()

    file(APPEND ${ATTR_HEADER} "\n#endif /* NUX_COMPILER_ATTRS_H */\n")

    message(STATUS "Generated compiler attributes header: ${ATTR_HEADER}")
endfunction()

# Create section marker variables programmatically (no linker script needed)
function(nux_generate_section_markers target_name sections)
    set(MARKER_SOURCE "${CMAKE_BINARY_DIR}/${target_name}_section_markers.c")

    file(WRITE ${MARKER_SOURCE} "/* Auto-generated section markers */\n")
    file(APPEND ${MARKER_SOURCE} "#include <stdint.h>\n\n")

    foreach(section ${sections})
        # Extract section name and size
        string(REPLACE ":" ";" section_parts ${section})
        list(GET section_parts 0 section_name)
        list(LENGTH section_parts parts_count)
        if(parts_count GREATER 1)
            list(GET section_parts 1 section_size)
        else()
            set(section_size 0)
        endif()

        # Generate start/end markers
        file(APPEND ${MARKER_SOURCE} "extern char _${section_name}_start[];\n")
        file(APPEND ${MARKER_SOURCE} "extern char _${section_name}_end[];\n")

        if(NUX_USE_LINKER_SCRIPTS)
            # Linker script will define these
            file(APPEND ${MARKER_SOURCE} "/* Defined by linker script */\n\n")
        else()
            # Define using section attributes
            if(NUX_COMPILER_VENDOR MATCHES "gcc|clang|intel")
                file(APPEND ${MARKER_SOURCE}
                    "__attribute__((section(\".${section_name}\"), used))\n"
                    "char _${section_name}_start[1] = {0};\n")
                file(APPEND ${MARKER_SOURCE}
                    "__attribute__((section(\".${section_name}_end\"), used))\n"
                    "char _${section_name}_end[1] = {0};\n\n")
            elseif(NUX_COMPILER_VENDOR MATCHES "msvc")
                file(APPEND ${MARKER_SOURCE}
                    "#pragma section(\".${section_name}\", read, write)\n"
                    "__declspec(allocate(\".${section_name}\"))\n"
                    "char _${section_name}_start[1] = {0};\n")
                file(APPEND ${MARKER_SOURCE}
                    "#pragma section(\".${section_name}_end\", read, write)\n"
                    "__declspec(allocate(\".${section_name}_end\"))\n"
                    "char _${section_name}_end[1] = {0};\n\n")
            endif()
        endif()
    endforeach()

    # Add source to target
    target_sources(${target_name} PRIVATE ${MARKER_SOURCE})
endfunction()

# Call this once to generate headers
nux_generate_compiler_attributes()
