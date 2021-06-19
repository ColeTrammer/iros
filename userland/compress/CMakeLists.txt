set(SOURCES
    main.cpp
)

add_os_executable(compress bin)
target_link_libraries(compress libext)
