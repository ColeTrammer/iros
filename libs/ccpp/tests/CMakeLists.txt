add_executable(test_ccpp
    test_errno_h.cpp
    test_stdlib_h_alloc.cpp
    test_stdlib_h_string.cpp
    test_string_h.cpp
)


find_package(Dius REQUIRED)

target_link_libraries(test_ccpp
  PRIVATE
    Dius::dius
    Dius::dius_test_main
)

if(IROS_UseDiusRuntime)
    target_link_libraries(test_ccpp
      PRIVATE
        Ccpp::ccpp
    )
endif()

add_test(NAME test_ccpp
    COMMAND test_ccpp
)
