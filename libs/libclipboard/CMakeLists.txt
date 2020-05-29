set(SOURCES
    connection.cpp
)

add_os_library(libclipboard clipboard TRUE)
target_link_libraries(libclipboard PRIVATE clipboard_server_headers)
target_link_libraries(libclipboard PUBLIC libliim)
