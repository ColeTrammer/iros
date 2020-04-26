set(SOURCES
    application.cpp
    main.cpp
    raw_frame_buffer_wrapper.cpp
    terminal.cpp
    tty.cpp
    vga_buffer.cpp
    window_wrapper.cpp
)

add_os_executable(xterm bin)
target_link_libraries(xterm libgraphics libliim libws)

add_os_executable(xterm-vga bin)
target_link_libraries(xterm-vga libgraphics libliim libws)
target_compile_definitions(xterm-vga PRIVATE KERNEL_NO_GRAPHICS)