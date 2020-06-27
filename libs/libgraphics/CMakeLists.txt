set(SOURCES
    color.cpp
    font.cpp
    pixel_buffer.cpp
    rect_set.cpp
    renderer.cpp
)

add_os_library(libgraphics graphics TRUE)

target_link_libraries(libgraphics PUBLIC libliim kernel_headers)
