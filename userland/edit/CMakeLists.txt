set(SOURCES
    editor.cpp
    main.cpp
)

add_os_executable(edit bin)
target_link_libraries(edit libliim)