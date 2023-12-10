cmake_minimum_required(VERSION 3.25.2)

project(PackageManager CXX)

find_package(Dius REQUIRED)

file(GLOB headers CONFIGURE_DEPENDS "*.h")
add_library(package_manager_headers INTERFACE)
add_library(PackageManager::package_manager_headers ALIAS package_manager_headers)

target_sources(package_manager_headers INTERFACE FILE_SET headers TYPE HEADERS FILES ${headers})
target_link_libraries(package_manager_headers INTERFACE Dius::dius Di::di)

file(GLOB sources CONFIGURE_DEPENDS "*.cpp")

add_executable(package_manager ${sources})
target_link_libraries(package_manager PUBLIC Dius::dius Di::di)

install(TARGETS package_manager RUNTIME DESTINATION bin)
