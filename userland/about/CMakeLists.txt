set(SOURCES
    main.cpp
)

add_os_executable(about bin)

target_link_libraries(about libgui)
