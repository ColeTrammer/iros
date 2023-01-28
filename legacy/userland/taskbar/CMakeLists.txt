set(SOURCES
    main.cpp
    taskbar.cpp
)

add_os_executable(taskbar bin)

target_link_libraries(taskbar libgui)
