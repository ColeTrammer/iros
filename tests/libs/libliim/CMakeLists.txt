set(TEST_FILES
    test_container.cpp
    test_enum.cpp
    test_error.cpp
    test_format.cpp
    test_function.cpp
    test_hash_map.cpp
    test_hash_set.cpp
    test_new_vector.cpp
    test_option.cpp
    test_pair.cpp
    test_preprocessor.cpp
    test_result.cpp
    test_string_view.cpp
    test_string.cpp
    test_task.cpp
    test_tuple.cpp
    test_utf8_view.cpp
    test_variant.cpp
)

add_os_tests(libliim ${TEST_FILES})
target_link_libraries(test_libliim PRIVATE libliim)
