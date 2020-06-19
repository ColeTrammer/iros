set(GENERATED_FILES
    generic_mode_parser_impl.cpp
    generic_mode_parser.h
    mode_token_type.h
    mode.info
)

add_custom_target(
    generate-mode-parser
    BYPRODUCTS ${GENERATED_FILES}
    COMMAND ${NATIVE_PARSER} -s -p . ${CMAKE_CURRENT_SOURCE_DIR}/mode.y 2>mode.info || [ $$? = 2 ]
    DEPENDS mode.y
)

set(SOURCES
    generic_mode_parser.cpp
    parse_mode.cpp
)

add_os_library(libext ext TRUE)
target_link_libraries(libext PUBLIC parser_headers libliim)
target_include_directories(libext PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

add_dependencies(libext generate-mode-parser)
