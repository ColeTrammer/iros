set(TEST_FILES
    test_point.cpp
    test_rect.cpp
)

add_os_tests(libgraphics ${TEST_FILES})
target_link_libraries(test_libgraphics PRIVATE libgraphics)
