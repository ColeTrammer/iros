set(SOURCES
    command.cpp
    cursor.cpp
    document_type.cpp
    document.cpp
    line_renderer.cpp
    line.cpp
    multicursor.cpp
    panel.cpp
    selection.cpp
    text_range_collection.cpp
    text_range.cpp
)

add_os_library(libedit edit TRUE)

target_link_libraries(libedit PUBLIC libliim libext libgraphics libeventloop libclanguage libsh)
