set(SOURCES
    file_input_source.cpp
    history.cpp
    input_source.cpp
    repl_base.cpp
    repl_display.cpp
    string_input_source.cpp
    suggestions_panel.cpp
    terminal_input_source.cpp
)

add_os_library(librepl repl TRUE)

target_link_libraries(librepl PUBLIC libtui libedit libclipboard)
