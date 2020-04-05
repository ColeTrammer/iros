add_library(parser_headers INTERFACE)
target_include_directories(parser_headers INTERFACE include)
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