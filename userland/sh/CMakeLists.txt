set(GENERATED_FILES
    generic_sh_parser_impl.cpp
    generic_sh_parser.h
    sh_token_type.h
    sh.info
)

add_custom_target(
    generate-sh-parser
    BYPRODUCTS ${GENERATED_FILES}
    COMMAND ${NATIVE_PARSER} -p . ${CMAKE_CURRENT_SOURCE_DIR}/sh.y 2>sh.info
    DEPENDS sh.y
)

set(SOURCES
    builtin.cpp
    command.cpp
    generic_sh_parser.cpp
    input.cpp
    job.cpp
    main.cpp
    sh_lexer.cpp
    sh_state.cpp
)

add_os_executable(sh bin)
target_link_libraries(sh parser_headers libliim)
target_include_directories(sh PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

if (${NATIVE_BUILD})
    target_compile_definitions(sh PRIVATE USERLAND_NATIVE)
    target_link_libraries(sh libwordexp)
endif()

add_dependencies(sh generate-sh-parser)