set(TEST_FILES
    test_bit_bitset.cpp
    test_bit_bitstruct.cpp
    test_concepts.cpp
    test_container_algorithm.cpp
    test_container_concepts.cpp
    test_container_priority_queue.cpp
    test_container_string.cpp
    test_container_tree_set.cpp
    test_container_vector.cpp
    test_container_view.cpp
    test_format.cpp
    test_function_bind.cpp
    test_function_invoke.cpp
    test_meta.cpp
    test_parser.cpp
    test_util_swap.cpp
    test_util.cpp
    test_vocab_array.cpp
    test_vocab_error.cpp
    test_vocab_expected.cpp
    test_vocab_optional.cpp
    test_vocab_pointer.cpp
    test_vocab_span.cpp
    test_vocab_tuple.cpp
    test_vocab_variant.cpp
)

add_os_tests(libdi ${TEST_FILES})
target_link_libraries(test_libdi PRIVATE libdi)
target_compile_definitions(test_libdi PRIVATE DI_USE_STD)
