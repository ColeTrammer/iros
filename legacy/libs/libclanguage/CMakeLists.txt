set(SOURCES
    clexer.cpp
    cpplexer.cpp
)

add_os_library(libclanguage clanguage TRUE)
target_link_libraries(libclanguage PUBLIC libliim)
