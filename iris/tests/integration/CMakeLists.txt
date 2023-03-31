add_executable(test_iris test_syscall.cpp)

find_package(Dius REQUIRED)

target_link_libraries(test_iris PRIVATE Dius::dius Dius::dius_test_main)

add_test(NAME test_iris COMMAND test_iris)
