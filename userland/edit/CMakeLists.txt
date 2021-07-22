set(SOURCES
    app_panel.cpp
    main.cpp
    terminal_panel.cpp
)

add_os_executable(edit bin)
target_link_libraries(edit libapp libedit libclipboard libtinput)
