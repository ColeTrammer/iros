set(SOURCES
    app.cpp
    box_layout.cpp
    button.cpp
    event_loop.cpp
    layout.cpp
    object.cpp
    selectable.cpp
    text_label.cpp
    unix_socket.cpp
    unix_socket_server.cpp
    widget.cpp
    window.cpp
)

add_os_static_library(libapp app TRUE)

target_link_libraries(libapp PUBLIC libliim libgraphics libws kernel_headers)
