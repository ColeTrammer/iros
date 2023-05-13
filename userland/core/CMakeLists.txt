cmake_minimum_required(VERSION 3.25.2)

project(Core CXX)

find_package(Dius REQUIRED)

add_executable(ls ls.cpp)
add_executable(cp cp.cpp)

target_link_libraries(ls PRIVATE Dius::dius)
target_link_libraries(cp PRIVATE Dius::dius)

install(TARGETS ls RUNTIME DESTINATION bin)
install(TARGETS cp RUNTIME DESTINATION bin)
