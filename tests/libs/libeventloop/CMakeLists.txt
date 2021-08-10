set(TEST_FILES
    test_file_watcher.cpp
)

add_os_tests(libeventloop ${TEST_FILES})
target_link_libraries(test_libeventloop libeventloop)
