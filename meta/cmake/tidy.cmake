add_custom_target(
    check_tidy
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMAND /bin/sh -c "'${CMAKE_SOURCE_DIR}/meta/run-clang-tidy.sh' check '$$IROS_TIDY_ARGS'"
    USES_TERMINAL
)

add_custom_target(
    tidy
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMAND /bin/sh -c "'${CMAKE_SOURCE_DIR}/meta/run-clang-tidy.sh' tidy '$$IROS_TIDY_ARGS'"
    USES_TERMINAL
)

add_custom_target(
    clang_analyze
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMAND /bin/sh -c "'${CMAKE_SOURCE_DIR}/meta/run-clang-tidy.sh' analyze '$$IROS_TIDY_ARGS'"
    USES_TERMINAL
)
