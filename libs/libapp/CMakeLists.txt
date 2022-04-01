set(SOURCES
    application.cpp
    file_system_model.cpp
    flex_layout_engine.cpp
    layout_engine.cpp
    model.cpp
    scroll_component.cpp
    terminal_widget.cpp
    tree_view.cpp
    view.cpp
    widget.cpp
    window.cpp
)

add_os_library(libapp app TRUE)

generate_interface(libapp libapp_widget_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/widget.h app/widget_interface.h)
generate_interface(libapp libapp_widget_bridge_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/widget_bridge.h app/widget_bridge_interface.h)

generate_interface(libapp libapp_view_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/view.h app/view_interface.h)
generate_interface(libapp libapp_view_bridge_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/view_bridge.h app/view_bridge_interface.h)

generate_interface(libapp libapp_tree_view_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/tree_view.h app/tree_view_interface.h)
generate_interface(libapp libapp_tree_view_bridge_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/tree_view_bridge.h app/tree_view_bridge_interface.h)

generate_interface(libapp libapp_terminal_widget_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/terminal_widget.h app/terminal_widget_interface.h)
generate_interface(libapp libapp_terminal_widget_bridge_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/terminal_widget_bridge.h app/terminal_widget_bridge_interface.h)

generate_interface(libapp libapp_scroll_component_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/scroll_component.h app/scroll_component_interface.h)
generate_interface(libapp libapp_scroll_component_bridge_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/app/scroll_component_bridge.h app/scroll_component_bridge_interface.h)

target_link_libraries(libapp PUBLIC libeventloop libterminal libclipboard libgraphics window_server_headers)
