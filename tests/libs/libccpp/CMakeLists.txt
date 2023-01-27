set(TEST_FILES
    test_string_h.cpp
)

add_dius_tests(libccpp ${TEST_FILES})
target_link_libraries(test_libccpp PRIVATE libccpp libdius libdius_test_main)
