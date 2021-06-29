set(SOURCES
    command.cpp
    document_type.cpp
    document.cpp
    line.cpp
    panel.cpp
)

add_os_library(libedit edit TRUE)

target_link_libraries(libedit PUBLIC libliim libgraphics libeventloop libclanguage libsh)
