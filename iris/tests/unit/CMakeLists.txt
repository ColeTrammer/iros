target_sources(iris PRIVATE tests/unit/allocation.cpp)

file(RELATIVE_PATH BUILD_BASE "${CMAKE_SOURCE_DIR}/build" "${CMAKE_BINARY_DIR}")

add_test(NAME test_iris_unit COMMAND "${CMAKE_SOURCE_DIR}/meta/run-command-on-iris.sh" "${BUILD_BASE}"
                                     "-run=kernel_unit_test"
)
