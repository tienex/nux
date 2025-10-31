cmake_minimum_required(VERSION 3.15)

# ANXCONFIG - Ananke Configuration Tool
project(anxconfig C)

# Detect platform and select appropriate TUI backend
set(TUI_BACKEND "")
set(TUI_LIBS "")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR
   CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR
   CMAKE_SYSTEM_NAME STREQUAL "NetBSD" OR
   CMAKE_SYSTEM_NAME STREQUAL "OpenBSD" OR
   CMAKE_SYSTEM_NAME STREQUAL "DragonFly" OR
   CMAKE_SYSTEM_NAME STREQUAL "SunOS" OR
   CMAKE_SYSTEM_NAME STREQUAL "AIX" OR
   CMAKE_SYSTEM_NAME STREQUAL "HP-UX" OR
   CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Unix-like systems: use ncurses
    set(TUI_BACKEND "tui_ncurses.c")
    find_package(Curses REQUIRED)
    set(TUI_LIBS ${CURSES_LIBRARIES})
    message(STATUS "ANXCONFIG: Using ncurses TUI backend")

elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Windows: use Win32 console API
    set(TUI_BACKEND "tui_win32.c")
    message(STATUS "ANXCONFIG: Using Win32 TUI backend")

elseif(CMAKE_SYSTEM_NAME STREQUAL "OS2")
    # OS/2: use VIO API
    set(TUI_BACKEND "tui_os2.c")
    message(STATUS "ANXCONFIG: Using OS/2 VIO TUI backend")

elseif(DEFINED ENV{OPENVMS} OR CMAKE_SYSTEM_NAME MATCHES "VMS")
    # OpenVMS: use SMG$ facility
    set(TUI_BACKEND "tui_vms.c")
    message(STATUS "ANXCONFIG: Using OpenVMS SMG TUI backend")

elseif(CMAKE_SYSTEM_NAME STREQUAL "DOS" OR DEFINED ENV{DJGPP})
    # DOS: use BIOS/direct VGA
    set(TUI_BACKEND "tui_dos.c")
    message(STATUS "ANXCONFIG: Using DOS TUI backend")

elseif(CMAKE_SYSTEM_NAME STREQUAL "NUX" OR DEFINED NUX_KERNEL_MODE)
    # NUX kernel: use kernel console
    set(TUI_BACKEND "tui_nux.c")
    message(STATUS "ANXCONFIG: Using NUX kernel TUI backend")

else()
    # Fallback: use simple stub implementation
    set(TUI_BACKEND "tui_simple.c")
    message(WARNING "ANXCONFIG: Unknown platform, using simple TUI stub")
endif()

# Common source files
set(ANXCONFIG_SOURCES
    main.c
    config_db.c
    config_generator.c
    yaml_parser.c
    expr_eval.c
    ui_composer.c
    # Widget implementations
    widget_button.c
    widget_checkbox.c
    widget_input.c
    widget_label.c
    widget_hyperlink.c
    widget_groupbox.c
    widget_elastic.c
    widget_listbox.c
    widget_scrollbar.c
    widget_menubar.c
    widget_statusbar.c
    widget_combobox.c
    widget_messagebox.c
    widget_menu.c
    widget_slider.c
    widget_spinner.c
    widget_progressbar.c
    widget_fkeybar.c
    widget_markdown_viewer.c
    widget_terminal.c
    widget_treeview.c
    focus_manager.c
    # Common dialogs
    dialog_fileopensave.c
    dialog_about.c
    dialog_color.c
    dialog_find.c
    dialog_print.c
    dialog_longop.c
    dialog_directory.c
    # Markdown parser (COM component)
    markdown_parser.c
    # Desk accessories
    deskapp_calculator.c
    ${TUI_BACKEND}
)

add_executable(anxconfig ${ANXCONFIG_SOURCES})

target_include_directories(anxconfig PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link against Ananke headers
target_link_libraries(anxconfig PRIVATE Ananke::Headers)

# Link against Ananke NTRTL if available
if(TARGET Ananke::NTRTL)
    target_link_libraries(anxconfig PRIVATE Ananke::NTRTL)
endif()

# Link against platform-specific TUI libraries
if(TUI_LIBS)
    target_link_libraries(anxconfig PRIVATE ${TUI_LIBS})
    target_include_directories(anxconfig PRIVATE ${CURSES_INCLUDE_DIRS})
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
