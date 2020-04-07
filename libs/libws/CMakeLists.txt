set(SOURCES
    connection.cpp
    window.cpp
)

add_os_library(libws ws TRUE)

target_link_libraries(libws PUBLIC libliim libgraphics window_server_headers)