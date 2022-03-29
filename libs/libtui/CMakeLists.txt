set(SOURCES
    application.cpp
    frame.cpp
    label.cpp
    panel.cpp
    root_window.cpp
    scroll_component.cpp
    table_view.cpp
    terminal_panel.cpp
    view.cpp
)

add_os_library(libtui tui true)

target_link_libraries(libtui libtinput libeventloop libterminal)
