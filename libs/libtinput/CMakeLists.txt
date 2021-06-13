set(SOURCES
    repl_panel.cpp
    repl.cpp
)

add_os_library(libtinput tinput TRUE)

target_link_libraries(libtinput PUBLIC libapp libedit libclipboard)
