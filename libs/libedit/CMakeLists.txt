set(SOURCES
    actions.cpp
    command.cpp
    cursor.cpp
    document_type.cpp
    document.cpp
    keyboard_action.cpp
    line_renderer.cpp
    line.cpp
    multicursor.cpp
    display.cpp
    selection.cpp
    suggestions.cpp
    text_range_collection.cpp
    text_range.cpp
)

add_os_library(libedit edit TRUE)

target_link_libraries(libedit PUBLIC libliim libext libgraphics libeventloop libclanguage libsh)
