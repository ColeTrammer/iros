set(SOURCES
    connection.cpp
    window.cpp
)

add_os_library(ws TRUE)

target_link_libraries(ws PUBLIC liim graphics window_server_headers)