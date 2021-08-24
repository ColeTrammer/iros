set(SOURCES
    base/application.cpp
    base/widget.cpp
    base/window.cpp
    application_os_2.cpp
    application_sdl.cpp
    application.cpp
    box_layout.cpp
    button.cpp
    context_menu.cpp
    icon_view.cpp
    layout.cpp
    menubar.cpp
    model.cpp
    tab_widget.cpp
    table_view.cpp
    text_label.cpp
    view.cpp
    widget.cpp
    window_os_2.cpp
    window_sdl.cpp
    window.cpp
)

add_os_library(libapp app TRUE)

if(${NATIVE_BUILD})
    find_package(SDL2)
    if (${SDL2_FOUND})
        target_link_libraries(libapp PUBLIC ${SDL2_LIBRARIES})
        target_include_directories(libapp PUBLIC ${SDL2_INCLUDE_DIRS})
        target_compile_definitions(libapp PRIVATE "USE_SDL2=1")
    endif()
endif()

target_link_libraries(libapp PUBLIC libeventloop libgraphics window_server_headers)
