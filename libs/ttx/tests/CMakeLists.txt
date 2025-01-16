file(GLOB sources CONFIGURE_DEPENDS *.cpp)
add_executable(test_libttx ${sources})

find_package(Libttx REQUIRED)

target_link_libraries(test_libttx PRIVATE Libttx::libttx Dius::dius_test_main)

add_test(NAME test_libttx COMMAND test_libttx)
