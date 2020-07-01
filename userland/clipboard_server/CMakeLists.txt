add_os_headers(clipboard_server_headers)

set(SOURCES
    main.cpp
)

add_os_executable(clipboard_server bin)
target_link_libraries(clipboard_server clipboard_server_headers libapp)
