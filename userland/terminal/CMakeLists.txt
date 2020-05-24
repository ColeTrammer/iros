set(SOURCES
    application.cpp
    main.cpp
    raw_frame_buffer_wrapper.cpp
    terminal.cpp
    tty.cpp
    vga_buffer.cpp
    window_wrapper.cpp
)

add_os_executable(terminal bin)
target_link_libraries(terminal libgraphics libliim libws)

add_os_executable(terminal-vga bin)
target_link_libraries(terminal-vga libgraphics libliim libws)
target_compile_definitions(terminal-vga PRIVATE KERNEL_NO_GRAPHICS)
