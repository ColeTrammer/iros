target_sources(iris
  PRIVATE
    tests/unit/allocation.cpp
)

add_test(
  NAME
    test_iris_unit
  COMMAND
    "${CMAKE_CURRENT_SOURCE_DIR}/../meta/run-command-on-iris.sh" "-run=kernel_unit_test"
)
