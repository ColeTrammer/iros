cmake_minimum_required(VERSION 3.25.2)

project(Shell CXX)

find_package(Dius REQUIRED)

add_executable(sh sh.cpp)

target_link_libraries(sh PRIVATE Dius::dius)

install(TARGETS sh RUNTIME DESTINATION bin)
