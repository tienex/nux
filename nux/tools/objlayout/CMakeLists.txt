cmake_minimum_required(VERSION 3.15)

# Object Layout Tool - manipulates ELF/PE/Mach-O files without linker scripts
project(objlayout C)

add_executable(objlayout
    objlayout.c
    elf_handler.c
    pe_handler.c
    macho_handler.c
)

target_include_directories(objlayout PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/ananke/include
)

install(TARGETS objlayout
    RUNTIME DESTINATION bin)
