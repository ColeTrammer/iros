add_executable(test_dius test_tls.cpp)

find_package(Dius REQUIRED)

target_link_libraries(test_dius PRIVATE Dius::dius Dius::dius_test_main)

add_test(NAME test_dius COMMAND test_dius)
