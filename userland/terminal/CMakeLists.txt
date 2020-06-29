set(SOURCES
    main.cpp
    pseudo_terminal.cpp
    terminal_widget.cpp
    tty.cpp
)

add_os_executable(terminal bin)
target_link_libraries(terminal libapp)
