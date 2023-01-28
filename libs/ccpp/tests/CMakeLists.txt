add_executable(test_ccpp
    test_string_h.cpp
)


find_package(Dius REQUIRED)

target_link_libraries(test_ccpp
  PRIVATE
    ccpp
    dius
    dius_test_main
)

add_test(NAME test_ccpp
    COMMAND test_ccpp
)