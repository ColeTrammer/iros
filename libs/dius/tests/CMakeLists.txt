add_executable(test_dius test_process.cpp test_tls.cpp)

find_package(Dius REQUIRED)

target_link_libraries(test_dius PRIVATE Dius::dius Dius::dius_test_main)

if(CMAKE_CROSSCOMPILING)
    target_compile_definitions(test_dius PRIVATE FIXTURE_PATH="/")
else()
    target_compile_definitions(test_dius PRIVATE FIXTURE_PATH="${CMAKE_CURRENT_BINARY_DIR}/")
endif()

add_executable(test_dius_process_arg_passing_fixture test_process_arg_passing_fixture.cpp)

target_link_libraries(test_dius_process_arg_passing_fixture PRIVATE Dius::dius)

add_test(NAME test_dius COMMAND test_dius)
