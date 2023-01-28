set(SOURCES
    interface.cpp
    main.cpp
)

add_os_executable(dhcp_client bin)
target_link_libraries(dhcp_client libeventloop)
