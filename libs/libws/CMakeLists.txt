set(SOURCES
    connection.cpp
    window.cpp
)

add_library(ws ${SOURCES})

target_include_directories(ws PUBLIC include)
target_link_libraries(ws PUBLIC liim graphics window_server_headers)