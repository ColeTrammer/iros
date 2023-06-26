cmake_minimum_required(VERSION 3.25.2)

project(PackageManager CXX)

find_package(Dius REQUIRED)

add_executable(package_manager config.cpp package.cpp package_database.cpp package_manager.cpp)

target_link_libraries(package_manager PRIVATE Dius::dius)

install(TARGETS package_manager RUNTIME DESTINATION bin)
