set(SOURCES
    file_input_source.cpp
    history.cpp
    input_source.cpp
    repl_panel.cpp
    repl.cpp
    string_input_source.cpp
    terminal_input_source.cpp
)

add_os_library(libtinput tinput TRUE)

target_link_libraries(libtinput PUBLIC libapp libedit libclipboard)
