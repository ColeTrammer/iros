cmake_minimum_required(VERSION 3.25.2)

project(Shell CXX)

find_package(Dius REQUIRED)

file(GLOB sources CONFIGURE_DEPENDS "*.cpp")
add_executable(sh ${sources})

target_link_libraries(sh PRIVATE Dius::dius)

install(TARGETS sh RUNTIME DESTINATION bin)
