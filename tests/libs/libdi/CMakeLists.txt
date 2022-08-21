set(TEST_FILES
    test_util_concepts.cpp
    test_util_meta.cpp
    test_util_tag_invoke.cpp
)

add_os_tests(libdi ${TEST_FILES})
target_link_libraries(test_libdi PRIVATE libdi)
