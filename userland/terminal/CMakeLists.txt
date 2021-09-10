set(SOURCES
    main.cpp
    vga_buffer.cpp
    vga_terminal.cpp
)

add_os_executable(terminal bin)
target_link_libraries(terminal libapp libclipboard libtui libterminal)
