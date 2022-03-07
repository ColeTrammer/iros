set(SOURCES
    actions.cpp
    command.cpp
    cursor.cpp
    display.cpp
    document_type.cpp
    document.cpp
    keyboard_action.cpp
    line_renderer.cpp
    line.cpp
    multicursor.cpp
    rendered_line.cpp
    suggestions.cpp
    text_range_collection.cpp
    text_range.cpp
)

add_os_library(libedit edit TRUE)

generate_interface(libedit libedit_display_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/edit/display.h edit/display_interface.h)
generate_interface(libedit libedit_display_bridge_interface ${CMAKE_CURRENT_SOURCE_DIR}/include/edit/display_bridge.h edit/display_bridge_interface.h)

target_link_libraries(libedit PUBLIC libapp libliim libext libgraphics libeventloop libthread libclanguage libsh)
