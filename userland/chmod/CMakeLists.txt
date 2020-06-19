set(SOURCES
    main.cpp
)

add_os_executable(chmod bin)
target_link_libraries(chmod libext)
