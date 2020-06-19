set(SOURCES
    main.cpp
)

add_os_executable(mkfifo bin)
target_link_libraries(mkfifo libext)