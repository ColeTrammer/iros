set(SOURCES
    color.cpp
    font.cpp
    bitmap.cpp
    png.cpp
    rect_set.cpp
    renderer.cpp
)

add_os_library(libgraphics graphics TRUE)

target_link_libraries(libgraphics PUBLIC libliim libext kernel_headers)
