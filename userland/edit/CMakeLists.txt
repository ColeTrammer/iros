set(SOURCES
    app_display.cpp
    main.cpp
    terminal_display.cpp
)

add_os_executable(edit bin)
target_link_libraries(edit libapp libedit libclipboard libtinput)
