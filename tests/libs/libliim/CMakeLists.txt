set(TEST_FILES
    test_format.cpp
    test_maybe.cpp
    test_string_view.cpp
    test_string.cpp
)

add_os_tests(libliim ${TEST_FILES})
target_link_libraries(test_libliim libliim)
