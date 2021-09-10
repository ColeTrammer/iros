set(SOURCES
    application.cpp
    frame.cpp
    label.cpp
    panel.cpp
    root_window.cpp
    terminal_panel.cpp
)

add_os_library(libtui tui true)

target_link_libraries(libtui libtinput libeventloop libterminal)
