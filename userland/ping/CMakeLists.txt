set(SOURCES
    main.c
)

add_os_executable(ping bin)
target_link_libraries(ping libext)
