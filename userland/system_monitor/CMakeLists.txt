set(SOURCES
    main.cpp
    process_model.cpp
    process_tab.cpp
    resource_usage_tab.cpp
)

add_os_executable(system_monitor bin)
target_link_libraries(system_monitor libgui libprocinfo)
