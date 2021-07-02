set(SOURCES
    application.cpp
    box_layout.cpp
    button.cpp
    context_menu.cpp
    icon_view.cpp
    layout.cpp
    menubar.cpp
    model.cpp
    tab_widget.cpp
    table_view.cpp
    text_label.cpp
    view.cpp
    widget.cpp
    window.cpp
)

add_os_library(libapp app TRUE)

target_link_libraries(libapp PUBLIC libeventloop libgraphics window_server_headers)
