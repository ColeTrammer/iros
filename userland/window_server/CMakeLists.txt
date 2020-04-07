add_os_headers(window_server_headers)
target_link_libraries(window_server_headers INTERFACE kernel_headers)

if (NOT ${NATIVE_BUILD})
    set(SOURCES
        main.cpp
        server.cpp
        window_manager.cpp
        window.cpp
    )

    add_os_executable(window_server bin)

    target_link_libraries(window_server window_server_headers libliim libgraphics)
endif()