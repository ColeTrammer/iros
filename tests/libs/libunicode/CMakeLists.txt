set(TEST_FILES
    test_terminal_width.cpp
)

add_os_tests(libunicode ${TEST_FILES})
target_link_libraries(test_libunicode PRIVATE libunicode)
