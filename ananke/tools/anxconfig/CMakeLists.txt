cmake_minimum_required(VERSION 3.15)

# ANXCONFIG - Ananke Configuration Tool
project(anxconfig C)

add_executable(anxconfig
    main.c
    config_db.c
    config_generator.c
    tui_simple.c
    yaml_parser.c
)

target_include_directories(anxconfig PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link against Ananke headers
target_link_libraries(anxconfig PRIVATE Ananke::Headers)

# Link against Ananke NTRTL if available
if(TARGET Ananke::NTRTL)
    target_link_libraries(anxconfig PRIVATE Ananke::NTRTL)
endif()

# Set output name
set_target_properties(anxconfig PROPERTIES
    OUTPUT_NAME anxconfig
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install
install(TARGETS anxconfig
    RUNTIME DESTINATION bin)

# Install example configuration files
install(FILES
    ${CMAKE_SOURCE_DIR}/anxconfig.yaml
    DESTINATION share/ananke/examples)
