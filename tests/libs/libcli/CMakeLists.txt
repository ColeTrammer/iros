set(TEST_FILES
    test_cli.cpp
)

add_os_tests(libcli ${TEST_FILES})
target_link_libraries(test_libcli PRIVATE libcli)
