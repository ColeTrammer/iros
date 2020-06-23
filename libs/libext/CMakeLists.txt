set(GENERATED_FILES
    generic_mode_parser_impl.cpp
    generic_mode_parser.h
    mode_token_type.h
    mode.info
)

add_custom_command(
    OUTPUT ${GENERATED_FILES}
    COMMAND ${NATIVE_PARSER} -p . -n Ext -v <ext/parse_mode.h> ${CMAKE_CURRENT_SOURCE_DIR}/mode.y 2> mode.info
    VERBATIM
    MAIN_DEPENDENCY mode.y
)
add_custom_target(generate-mode-parser DEPENDS ${GENERATED_FILES})

set(SOURCES
    ${GENERATED_FILES}
    generic_mode_parser.cpp
    parse_mode.cpp
)

add_os_library(libext ext TRUE)
target_link_libraries(libext PUBLIC parser_headers libliim)
target_include_directories(libext PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

add_dependencies(libext generate-mode-parser)
