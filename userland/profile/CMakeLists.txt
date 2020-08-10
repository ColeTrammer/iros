set(SOURCES
    main.cpp
)

add_os_executable(profile bin)
target_link_libraries(profile libapp)
