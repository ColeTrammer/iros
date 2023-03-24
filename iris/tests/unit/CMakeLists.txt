target_sources(iris
  PRIVATE
    tests/unit/allocation.cpp
)

get_filename_component(BUILD_BASE "${CMAKE_CURRENT_BINARY_DIR}/.." REALPATH)

add_test(
  NAME
    test_iris_unit
  COMMAND
    "${CMAKE_CURRENT_SOURCE_DIR}/../meta/run-command-on-iris.sh" "${BUILD_BASE}" "-run=kernel_unit_test"
)
