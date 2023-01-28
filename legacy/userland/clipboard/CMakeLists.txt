set(SOURCES
    main.cpp
)

add_os_executable(clipboard bin)
target_link_libraries(clipboard libclipboard libliim)
