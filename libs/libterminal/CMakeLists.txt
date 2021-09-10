set(SOURCES
    pseudo_terminal.cpp
    tty_parser.cpp
    tty.cpp
)

add_os_library(libterminal terminal TRUE)

target_link_libraries(libterminal PUBLIC libeventloop libgraphics)
