set(TEST_FILES
    test_maybe.cpp
    test_string_view.cpp
    test_string.cpp
)

add_os_library_tests(libliim ${TEST_FILES})
