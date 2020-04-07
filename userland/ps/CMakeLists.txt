set(SOURCES
    main.c
)

add_os_executable(ps bin)
target_link_libraries(ps libprocinfo)