set(SOURCES
    main.cpp
)

add_os_executable(mknod bin)
target_link_libraries(mknod libext)