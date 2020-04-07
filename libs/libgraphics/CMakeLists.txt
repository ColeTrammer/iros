set(SOURCES
    color.cpp
    pixel_buffer.cpp
    renderer.cpp
)

add_os_library(libgraphics graphics TRUE)

target_link_libraries(libgraphics PUBLIC libliim kernel_headers)