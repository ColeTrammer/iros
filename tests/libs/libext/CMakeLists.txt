set(TEST_FILES
    test_parser.cpp
    test_system.cpp
)

add_os_tests(libext ${TEST_FILES})
target_link_libraries(test_libext PRIVATE libext)
