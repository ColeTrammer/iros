set(SOURCES
    command.cpp
    document_type.cpp
    document.cpp
    line.cpp
    main.cpp
    panel.cpp
    terminal_panel.cpp
)

add_os_executable(edit bin)
target_link_libraries(edit libclanguage libclipboard libliim libsh)
