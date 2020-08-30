set(SOURCES
    main.cpp
    process_model.cpp
)

add_os_executable(system_monitor bin)
target_link_libraries(system_monitor libapp libprocinfo)
