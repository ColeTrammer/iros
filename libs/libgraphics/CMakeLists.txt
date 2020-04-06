set(SOURCES
    color.cpp
    pixel_buffer.cpp
    renderer.cpp
)

add_os_library(graphics TRUE)

target_link_libraries(graphics PUBLIC liim kernel_headers)