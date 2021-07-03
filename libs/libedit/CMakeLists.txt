set(SOURCES
    command.cpp
    cursor.cpp
    document_type.cpp
    document.cpp
    line.cpp
    panel.cpp
)

add_os_library(libedit edit TRUE)

target_link_libraries(libedit PUBLIC libliim libext libgraphics libeventloop libclanguage libsh)
