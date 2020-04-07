set(SOURCES
    main.c
)

add_os_executable(top bin)
target_link_libraries(top procinfo)