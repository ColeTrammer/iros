set(SOURCES
    application.cpp
    frame.cpp
    label.cpp
    panel.cpp
    scroll_component.cpp
    splitter.cpp
    table_view.cpp
    terminal_panel.cpp
    tree_view.cpp
    view.cpp
    window.cpp
)

add_os_library(libtui tui true)

target_link_libraries(libtui libtinput libeventloop libterminal)
