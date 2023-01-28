set(SOURCES
    parser_impl.cpp
)

add_os_library(libcli cli TRUE)
target_link_libraries(libcli PUBLIC libliim libext)
