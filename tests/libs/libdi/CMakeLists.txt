set(TEST_FILES
    test_util_concepts.cpp
    test_util_invoke.cpp
    test_util_meta.cpp
    test_util_swap.cpp
    test_util_tag_invoke.cpp
    test_vocab_optional.cpp
)

add_os_tests(libdi ${TEST_FILES})
target_link_libraries(test_libdi PRIVATE libdi)
target_compile_definitions(test_libdi PRIVATE DI_USE_STD)
