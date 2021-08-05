set(SOURCES
    io_terminal.cpp
    terminal_glyph.cpp
    terminal_input_parser.cpp
    terminal_renderer.cpp
)

add_os_library(libtinput tinput TRUE)

target_link_libraries(libtinput PUBLIC libapp libedit libclipboard)
