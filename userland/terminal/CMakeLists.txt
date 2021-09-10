set(SOURCES
    base_terminal_widget.cpp
    main.cpp
    terminal_panel.cpp
    terminal_widget.cpp
    vga_buffer.cpp
    vga_terminal.cpp
)

add_os_executable(terminal bin)
target_link_libraries(terminal libapp libclipboard libtui libterminal)
