set(SOURCES
    base_terminal_widget.cpp
    main.cpp
    pseudo_terminal.cpp
    terminal_panel.cpp
    terminal_widget.cpp
    tty_parser.cpp
    tty.cpp
    vga_buffer.cpp
    vga_terminal.cpp
)

add_os_executable(terminal bin)
target_link_libraries(terminal libapp libclipboard libtui)
