set(SOURCES
    event_loop.cpp
    object.cpp
    selectable.cpp
    selectable_file.cpp
    timer.cpp
    udp_socket.cpp
    unix_socket.cpp
    unix_socket_server.cpp
)

add_os_library(libeventloop eventloop TRUE)

target_link_libraries(libeventloop PUBLIC libliim ${REALTIME_LIB} kernel_headers)
