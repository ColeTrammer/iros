set(TEST_FILES
    test_format.cpp
    test_maybe.cpp
    test_preprocessor.cpp
    test_result.cpp
    test_string_view.cpp
    test_string.cpp
    test_task.cpp
    test_utf8_view.cpp
    test_variant.cpp
)

add_os_tests(libliim ${TEST_FILES})
target_link_libraries(test_libliim PRIVATE libliim)
