set(SOURCES
    app.cpp
    box_layout.cpp
    event_loop.cpp
    layout.cpp
    object.cpp
    selectable.cpp
    text_label.cpp
    widget.cpp
    window.cpp
)

add_os_library(libapp app TRUE)

target_link_libraries(libapp PUBLIC libliim libgraphics libws)
