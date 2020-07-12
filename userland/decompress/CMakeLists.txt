set(SOURCES
    main.cpp
)

add_os_executable(decompress bin)
target_link_libraries(decompress libext)
