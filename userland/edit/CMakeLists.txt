set(SOURCES
    app_display.cpp
    main.cpp
    terminal_display.cpp
    terminal_prompt.cpp
    terminal_search.cpp
    terminal_status_bar.cpp
)

add_os_executable(edit bin)
target_link_libraries(edit libapp libedit libclipboard libtui libgui)
