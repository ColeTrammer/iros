cmake_minimum_required(VERSION 3.25.2)

project(Tools CXX)

find_package(Dius REQUIRED)
find_package(Iris REQUIRED)

add_executable(initrd initrd.cpp)
add_executable(generate_presets generate_presets.cpp)

target_link_libraries(initrd PRIVATE Dius::dius Iris::iris_headers)
target_link_libraries(generate_presets PRIVATE Dius::dius)

install(TARGETS initrd RUNTIME DESTINATION bin)
install(TARGETS generate_presets RUNTIME DESTINATION bin)
