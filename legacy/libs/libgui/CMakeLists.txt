set(SOURCES
    application.cpp
    application_iros.cpp
    application_sdl.cpp
    button.cpp
    context_menu.cpp
    icon_view.cpp
    menubar.cpp
    scroll_component.cpp
    table_view.cpp
    tab_widget.cpp
    terminal_widget.cpp
    text_label.cpp
    tree_view.cpp
    view.cpp
    widget.cpp
    window.cpp
    window_iros.cpp
    window_sdl.cpp
)

add_os_library(libgui gui TRUE)

if(${NATIVE_BUILD})
    find_package(SDL2)
    if (${SDL2_FOUND})
        target_link_libraries(libgui PUBLIC ${SDL2_LIBRARIES})
        target_include_directories(libgui PUBLIC ${SDL2_INCLUDE_DIRS})
        target_compile_definitions(libgui PRIVATE "USE_SDL2=1")
    endif()
endif()

target_link_libraries(libgui PUBLIC libgraphics libapp)
