add_os_headers(window_server_headers)
target_link_libraries(window_server_headers INTERFACE kernel_headers)

if (${TARGET_OS} STREQUAL os_2)
    set(SOURCES
        main.cpp
        server.cpp
        window_manager.cpp
        window.cpp
    )

    add_executable(window_server ${SOURCES})

    target_link_libraries(window_server PRIVATE window_server_headers liim graphics)

    install(TARGETS window_server RUNTIME DESTINATION bin)
endif()