set(GENERATED_FILES
    generic_sh_parser_impl.cpp
    generic_sh_parser.h
    sh_token_type.h
    sh.info
)

add_custom_command(
    OUTPUT ${GENERATED_FILES}
    COMMAND [ -e ${CMAKE_CURRENT_SOURCE_DIR}/generic_sh_parser_impl.cpp ] || ${NATIVE_PARSER} ${CMAKE_CURRENT_SOURCE_DIR}/sh.y 2>sh.info
)

add_custom_target(
    generate-sh-parser
    DEPENDS ${GENERATED_FILES}
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

add_executable(sh ${SOURCES})
target_link_libraries(sh parser_headers liim)

add_dependencies(sh generate-sh-parser)

install(TARGETS sh RUNTIME DESTINATION bin)