set(SOURCES
    color.cpp
    pixel_buffer.cpp
    renderer.cpp
)

add_library(graphics ${SOURCES})

target_include_directories(graphics PUBLIC include)
target_link_libraries(graphics PUBLIC liim kernel_headers)