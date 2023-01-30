add_executable(test_di
    test_bit_bitset.cpp
    test_bit_bitstruct.cpp
    test_cli.cpp
    test_chrono_duration.cpp
    test_concepts.cpp
    test_container_algorithm.cpp
    test_container_concepts.cpp
    test_container_intrusive.cpp
    test_container_linked_list.cpp
    test_container_path.cpp
    test_container_queue.cpp
    test_container_string.cpp
    test_container_tree_map.cpp
    test_container_tree_set.cpp
    test_container_vector.cpp
    test_container_view.cpp
    test_execution.cpp
    test_format.cpp
    test_function_bind.cpp
    test_function_generator.cpp
    test_function_invoke.cpp
    test_math_rational.cpp
    test_math_integer.cpp
    test_meta.cpp
    test_parser.cpp
    test_sync_in_place_stop_source.cpp
    test_util_swap.cpp
    test_util.cpp
    test_vocab_array.cpp
    test_vocab_error.cpp
    test_vocab_expected.cpp
    test_vocab_md.cpp
    test_vocab_optional.cpp
    test_vocab_pointer.cpp
    test_vocab_span.cpp
    test_vocab_tuple.cpp
    test_vocab_variant.cpp
)

find_package(Dius REQUIRED)

target_link_libraries(test_di
  PRIVATE
    di
    dius
    dius_test_main
)

add_test(NAME test_di
    COMMAND test_di
)