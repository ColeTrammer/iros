set(TEST_FILES
    test_concepts.cpp
    test_container_concepts.cpp
    test_container_string.cpp
    test_container_vector.cpp
    test_container_view.cpp
    test_function_bind.cpp
    test_function_invoke.cpp
    test_meta.cpp
    test_util_swap.cpp
    test_vocab_array.cpp
    test_vocab_expected.cpp
    test_vocab_optional.cpp
    test_vocab_span.cpp
    test_vocab_tuple.cpp
)

add_os_tests(libdi ${TEST_FILES})
target_link_libraries(test_libdi PRIVATE libdi)
target_compile_definitions(test_libdi PRIVATE DI_USE_STD)
