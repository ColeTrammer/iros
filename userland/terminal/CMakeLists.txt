set(SOURCES
    main.cpp
    pseudo_terminal.cpp
    terminal_widget.cpp
    tty.cpp
    vga_buffer.cpp
    vga_terminal.cpp
)

add_os_executable(terminal bin)
target_link_libraries(terminal libapp)
