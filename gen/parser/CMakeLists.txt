add_os_headers(parser_headers)
target_link_libraries(parser_headers INTERFACE liim)

set(SOURCES
    extended_grammar.cpp
    generator.cpp
    item_set.cpp
    lexer.cpp
    main.cpp
    state_table.cpp
)

add_executable(parser ${SOURCES})
target_link_libraries(parser liim)

install(TARGETS parser RUNTIME DESTINATION bin)