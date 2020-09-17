set(SOURCES
    app.cpp
    box_layout.cpp
    button.cpp
    context_menu.cpp
    event_loop.cpp
    icon_view.cpp
    layout.cpp
    model.cpp
    mouse_press_tracker.cpp
    object.cpp
    selectable.cpp
    selectable_file.cpp
    tab_widget.cpp
    table_view.cpp
    text_label.cpp
    timer.cpp
    unix_socket.cpp
    unix_socket_server.cpp
    view.cpp
    widget.cpp
    window.cpp
)

add_os_library(libapp app TRUE)

target_link_libraries(libapp PUBLIC libliim libgraphics libws ${REALTIME_LIB} kernel_headers)
