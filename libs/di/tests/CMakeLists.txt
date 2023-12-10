file(GLOB sources CONFIGURE_DEPENDS *.cpp)
add_executable(test_di ${sources})

find_package(Dius REQUIRED)

target_link_libraries(test_di PRIVATE Di::di Dius::dius Dius::dius_test_main)

add_test(NAME test_di COMMAND test_di)
