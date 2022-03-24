set(SOURCES
    base/application.cpp
    base/scroll_component.cpp
    base/terminal_widget.cpp
    base/tree_view.cpp
    base/view.cpp
    base/widget.cpp
    base/window.cpp
    application_os_2.cpp
    application_sdl.cpp
    application.cpp
    button.cpp
    context_menu.cpp
    flex_layout_engine.cpp
    icon_view.cpp
    layout_engine.cpp
    menubar.cpp
    model.cpp
    scroll_component.cpp
    tab_widget.cpp
    table_view.cpp
    terminal_widget.cpp
    text_label.cpp
    tree_view.cpp
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

generate_interface(libapp libapp_base_widget_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/base/widget.h app/base/widget_interface.h)
generate_interface(libapp libapp_base_widget_bridge_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/base/widget_bridge.h app/base/widget_bridge_interface.h)

generate_interface(libapp libapp_base_view_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/base/view.h app/base/view_interface.h)
generate_interface(libapp libapp_base_view_bridge_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/base/view_bridge.h app/base/view_bridge_interface.h)

generate_interface(libapp libapp_base_tree_view_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/base/tree_view.h app/base/tree_view_interface.h)
generate_interface(libapp libapp_base_tree_view_bridge_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/base/tree_view_bridge.h app/base/tree_view_bridge_interface.h)

generate_interface(libapp libapp_base_terminal_widget_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/base/terminal_widget.h app/base/terminal_widget_interface.h)
generate_interface(libapp libapp_base_terminal_widget_bridge_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/base/terminal_widget_bridge.h app/base/terminal_widget_bridge_interface.h)

generate_interface(libapp libapp_base_scroll_component_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/base/scroll_component.h app/base/scroll_component_interface.h)
generate_interface(libapp libapp_base_scroll_component_bridge_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/base/scroll_component_bridge.h app/base/scroll_component_bridge_interface.h)

target_link_libraries(libapp PUBLIC libeventloop libgraphics libterminal libclipboard window_server_headers)
