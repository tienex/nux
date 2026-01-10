# NuxSectionGeneration.cmake
# Tools for generating section layouts without linker scripts

# Generate memory layout C file
function(nux_generate_memory_layout target_name arch)
    set(LAYOUT_SOURCE "${CMAKE_BINARY_DIR}/${target_name}_layout.c")
    set(LAYOUT_HEADER "${CMAKE_BINARY_DIR}/include/nux/${target_name}_layout.h")

    # Architecture-specific memory layouts
    if(arch STREQUAL "i386")
        set(TEXT_VADDR "0xc0100000")
        set(TEXT_PADDR "0x00100000")
        set(PHYSMAP_SIZE "1048576")    # 1 MB
        set(PFNCACHE_SIZE "1048576")   # 1 MB
        set(STREE_SIZE "16777216")     # 16 MB
        set(KMEM_SIZE "536870912")     # 512 MB
        set(KVA_SIZE "268435456")      # 256 MB
        set(FBUF_SIZE "16777216")      # 16 MB
        set(LINEAR_SIZE "8388608")     # 8 MB
    elseif(arch STREQUAL "amd64")
        set(TEXT_VADDR "0xffffffff80100000")
        set(TEXT_PADDR "0x00100000")
        set(PHYSMAP_SIZE "2097152")    # 2 MB
        set(PFNCACHE_SIZE "2097152")   # 2 MB
        set(STREE_SIZE "33554432")     # 32 MB
        set(KMEM_SIZE "1073741824")    # 1 GB
        set(KVA_SIZE "536870912")      # 512 MB
        set(FBUF_SIZE "33554432")      # 32 MB
        set(LINEAR_SIZE "16777216")    # 16 MB
    elseif(arch STREQUAL "riscv64")
        set(TEXT_VADDR "0xffffffc080000000")
        set(TEXT_PADDR "0x80000000")
        set(PHYSMAP_SIZE "2097152")    # 2 MB
        set(PFNCACHE_SIZE "2097152")   # 2 MB
        set(STREE_SIZE "33554432")     # 32 MB
        set(KMEM_SIZE "1073741824")    # 1 GB
        set(KVA_SIZE "536870912")      # 512 MB
        set(FBUF_SIZE "33554432")      # 32 MB
        set(LINEAR_SIZE "16777216")    # 16 MB
    endif()

    # Generate header file
    file(WRITE ${LAYOUT_HEADER} "/* Auto-generated memory layout for ${arch} */\n")
    file(APPEND ${LAYOUT_HEADER} "#ifndef NUX_${arch}_LAYOUT_H\n")
    file(APPEND ${LAYOUT_HEADER} "#define NUX_${arch}_LAYOUT_H\n\n")
    file(APPEND ${LAYOUT_HEADER} "#include <stdint.h>\n\n")
    file(APPEND ${LAYOUT_HEADER} "#define NUX_TEXT_VADDR ${TEXT_VADDR}UL\n")
    file(APPEND ${LAYOUT_HEADER} "#define NUX_TEXT_PADDR ${TEXT_PADDR}UL\n")
    file(APPEND ${LAYOUT_HEADER} "#define NUX_PHYSMAP_SIZE ${PHYSMAP_SIZE}UL\n")
    file(APPEND ${LAYOUT_HEADER} "#define NUX_PFNCACHE_SIZE ${PFNCACHE_SIZE}UL\n")
    file(APPEND ${LAYOUT_HEADER} "#define NUX_STREE_SIZE ${STREE_SIZE}UL\n")
    file(APPEND ${LAYOUT_HEADER} "#define NUX_KMEM_SIZE ${KMEM_SIZE}UL\n")
    file(APPEND ${LAYOUT_HEADER} "#define NUX_KVA_SIZE ${KVA_SIZE}UL\n")
    file(APPEND ${LAYOUT_HEADER} "#define NUX_FBUF_SIZE ${FBUF_SIZE}UL\n")
    file(APPEND ${LAYOUT_HEADER} "#define NUX_LINEAR_SIZE ${LINEAR_SIZE}UL\n\n")

    # Extern declarations for section boundaries
    file(APPEND ${LAYOUT_HEADER} "/* Section boundaries (defined by compiler/linker) */\n")
    file(APPEND ${LAYOUT_HEADER} "extern char _nuxperf_start[], _nuxperf_end[];\n")
    file(APPEND ${LAYOUT_HEADER} "extern char _nuxmeasure_start[], _nuxmeasure_end[];\n")
    file(APPEND ${LAYOUT_HEADER} "extern char _data_ext0_start[], _data_ext0_end[];\n")
    file(APPEND ${LAYOUT_HEADER} "extern char _data_ext1_start[], _data_ext1_end[];\n")
    file(APPEND ${LAYOUT_HEADER} "extern char _sbss[], _ebss[];\n")
    file(APPEND ${LAYOUT_HEADER} "extern char _physmap_start[], _physmap_end[];\n")
    file(APPEND ${LAYOUT_HEADER} "extern char _pfncache_start[], _pfncache_end[];\n")
    file(APPEND ${LAYOUT_HEADER} "extern char _info_start[], _info_end[];\n")
    file(APPEND ${LAYOUT_HEADER} "extern char _memregs_start[], _memregs_end[];\n")
    file(APPEND ${LAYOUT_HEADER} "extern char _stree_start[], _stree_end[];\n")
    file(APPEND ${LAYOUT_HEADER} "extern char _kmem_start[], _kmem_end[];\n")
    file(APPEND ${LAYOUT_HEADER} "extern char _kva_start[], _kva_end[];\n")
    file(APPEND ${LAYOUT_HEADER} "extern char _fbuf_start[], _fbuf_end[];\n")
    file(APPEND ${LAYOUT_HEADER} "extern char _ksym_start[], _ksym_end[];\n")
    file(APPEND ${LAYOUT_HEADER} "extern char _linear_start[], _linear_end[];\n")
    file(APPEND ${LAYOUT_HEADER} "extern char _end[];\n\n")

    file(APPEND ${LAYOUT_HEADER} "#endif /* NUX_${arch}_LAYOUT_H */\n")

    # Generate source file with layout runtime functions
    file(WRITE ${LAYOUT_SOURCE} "/* Auto-generated memory layout */\n")
    file(APPEND ${LAYOUT_SOURCE} "#include \"nux/${target_name}_layout.h\"\n\n")

    # Define getter functions for layout info
    file(APPEND ${LAYOUT_SOURCE} "uintptr_t nux_get_text_vaddr(void) {\n")
    file(APPEND ${LAYOUT_SOURCE} "    return ${TEXT_VADDR}UL;\n")
    file(APPEND ${LAYOUT_SOURCE} "}\n\n")

    file(APPEND ${LAYOUT_SOURCE} "uintptr_t nux_get_text_paddr(void) {\n")
    file(APPEND ${LAYOUT_SOURCE} "    return ${TEXT_PADDR}UL;\n")
    file(APPEND ${LAYOUT_SOURCE} "}\n\n")

    # Add source to target
    target_sources(${target_name} PRIVATE ${LAYOUT_SOURCE})
    target_include_directories(${target_name} PRIVATE ${CMAKE_BINARY_DIR}/include)

    message(STATUS "Generated memory layout for ${target_name} (${arch})")
endfunction()

# Generate custom ELF/PE/Mach-O program headers without linker scripts
function(nux_generate_custom_phdrs target_name arch format)
    if(NUX_USE_LINKER_SCRIPTS)
        return()  # Use traditional linker scripts
    endif()

    set(PHDR_TOOL "${CMAKE_BINARY_DIR}/tools/phdr_generator")
    set(PHDR_SOURCE "${CMAKE_BINARY_DIR}/${target_name}_phdrs.c")

    # Generate PHDR insertion code based on format
    if(format STREQUAL "elf")
        file(WRITE ${PHDR_SOURCE} "/* Custom ELF program headers */\n")
        file(APPEND ${PHDR_SOURCE} "#include <stdint.h>\n\n")

        # Define APXH-specific program header types
        file(APPEND ${PHDR_SOURCE} "#define PT_APXH_INFO 0xAF100000\n")
        file(APPEND ${PHDR_SOURCE} "#define PT_APXH_PHYSMAP 0xAF100002\n")
        file(APPEND ${PHDR_SOURCE} "#define PT_APXH_BATREE 0xAF100004\n")
        file(APPEND ${PHDR_SOURCE} "#define PT_APXH_PFNCACHE 0xAF100005\n")
        file(APPEND ${PHDR_SOURCE} "#define PT_APXH_FBUF 0xAF100006\n")
        file(APPEND ${PHDR_SOURCE} "#define PT_APXH_REGIONS 0xAF100007\n")
        file(APPEND ${PHDR_SOURCE} "#define PT_APXH_TOPPGTALLOC 0xAF100008\n")
        file(APPEND ${PHDR_SOURCE} "#define PT_APXH_LINEAR 0xAF10FFFF\n\n")

        file(APPEND ${PHDR_SOURCE} "/* These will be processed by post-link tool */\n")

    elseif(format STREQUAL "pecoff")
        file(WRITE ${PHDR_SOURCE} "/* PE/COFF section headers */\n")
        file(APPEND ${PHDR_SOURCE} "/* Custom sections will be added by post-link tool */\n")

    elseif(format STREQUAL "macho")
        file(WRITE ${PHDR_SOURCE} "/* Mach-O load commands */\n")
        file(APPEND ${PHDR_SOURCE} "/* Custom segments will be added by post-link tool */\n")
    endif()

    target_sources(${target_name} PRIVATE ${PHDR_SOURCE})

    message(STATUS "Generated custom program headers for ${target_name} (${format})")
endfunction()
