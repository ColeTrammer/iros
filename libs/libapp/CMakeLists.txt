set(SOURCES
    object.cpp
    widget.cpp
    window.cpp
)

add_os_library(libapp app TRUE)

target_link_libraries(libapp PUBLIC libliim libgraphics libws)
