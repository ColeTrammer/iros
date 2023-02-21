set(SOURCES
    main.cpp
)

add_os_executable(mkdir bin)
target_link_libraries(mkdir libext)
