set(GENERATED_FILES
    generic_sh_parser_impl.cpp
    generic_sh_parser.h
    sh_token_type.h
    sh.info
)

add_custom_target(
    generate-sh-parser
    BYPRODUCTS ${GENERATED_FILES}
    COMMAND ${NATIVE_PARSER} -s -p . ${CMAKE_CURRENT_SOURCE_DIR}/sh.y 2>sh.info || [ $$? = 2 ]
    DEPENDS sh.y
)

set(SOURCES
    generic_sh_parser.cpp
    sh_lexer.cpp
)

add_os_library(libsh sh TRUE)
target_link_libraries(libsh PUBLIC parser_headers libliim)
target_include_directories(libsh PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

add_dependencies(libsh generate-sh-parser)

if (${NATIVE_BUILD})
    target_compile_definitions(libsh PRIVATE USERLAND_NATIVE)
    target_link_libraries(libsh PRIVATE libwordexp)
endif()

