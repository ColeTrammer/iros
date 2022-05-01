set(TEST_FILES
    test_barrier.cpp
    test_cancel.cpp
    test_cleanup.cpp
)

add_os_tests(libpthread ${TEST_FILES})
target_link_libraries(test_libpthread PRIVATE ${PTHREAD_LIB})
