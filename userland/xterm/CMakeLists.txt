set(SOURCES
    application.cpp
    main.cpp
    raw_frame_buffer_wrapper.cpp
    terminal.cpp
    tty.cpp
    vga_buffer.cpp
    window_wrapper.cpp
)

add_executable(xterm ${SOURCES})
target_link_libraries(xterm graphics liim ws)

install(TARGETS xterm RUNTIME DESTINATION bin)