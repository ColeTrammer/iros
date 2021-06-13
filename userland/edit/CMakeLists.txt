set(SOURCES
    main.cpp
    terminal_panel.cpp
)

if (NOT ${NATIVE_BUILD})
    set(SOURCES ${SOURCES} 
        app_panel.cpp
    )
endif()

add_os_executable(edit bin)
target_link_libraries(edit libapp libedit libclipboard)
