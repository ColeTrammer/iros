cmake_minimum_required(VERSION 3.25.2)

project(Tools CXX)

find_package(Dius REQUIRED)
find_package(Iris REQUIRED)

add_executable(initrd initrd.cpp)

target_link_libraries(initrd PRIVATE Dius::dius Iris::iris_headers)

install(TARGETS initrd RUNTIME DESTINATION bin)
