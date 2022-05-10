set(SOURCES
    main.cpp
    port.cpp
)

add_os_executable(port_manager bin)
target_link_libraries(port_manager libliim libext)
