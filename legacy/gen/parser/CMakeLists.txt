add_os_headers(parser_headers)
target_link_libraries(parser_headers INTERFACE libliim)

set(SOURCES
    extended_grammar.cpp
    generator.cpp
    item_set.cpp
    lexer.cpp
    main.cpp
    state_table.cpp
)

add_os_executable(parser bin)
target_link_libraries(parser libliim)
