set(SOURCES
    editor.cpp
    main.cpp
    terminal_panel.cpp
)

add_os_executable(edit bin)
target_link_libraries(edit libliim)