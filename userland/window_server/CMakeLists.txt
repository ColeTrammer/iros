add_os_headers(window_server_headers)
target_link_libraries(window_server_headers INTERFACE kernel_headers)

if (${TARGET_OS} STREQUAL os_2)
    set(SOURCES
        main.cpp
        server.cpp
        window_manager.cpp
        window.cpp
    )

    add_os_executable(window_server bin)

    target_link_libraries(window_server window_server_headers liim graphics)
endif()