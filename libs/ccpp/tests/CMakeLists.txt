file(GLOB sources CONFIGURE_DEPENDS *.cpp)
list(REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/test_bits_all_h.cpp)
add_executable(test_ccpp ${sources})

find_package(Dius REQUIRED)

target_link_libraries(test_ccpp PRIVATE Dius::dius Dius::dius_test_main)

if(IROS_UseDiusRuntime)
    target_sources(test_ccpp PRIVATE test_bits_all_h.cpp)
    target_link_libraries(test_ccpp PRIVATE Ccpp::ccpp)
endif()

add_test(NAME test_ccpp COMMAND test_ccpp)
