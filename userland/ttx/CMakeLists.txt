cmake_minimum_required(VERSION 3.25.2)

project(Ttx CXX)

find_package(Dius REQUIRED)

file(GLOB headers CONFIGURE_DEPENDS "*.h")
add_library(ttx_headers INTERFACE)
add_library(Ttx::ttx_headers ALIAS ttx_headers)

target_sources(ttx_headers INTERFACE FILE_SET headers TYPE HEADERS FILES ${headers})
target_link_libraries(ttx_headers INTERFACE Dius::dius Di::di)

file(GLOB sources CONFIGURE_DEPENDS "*.cpp")

add_executable(ttx ${sources})
target_link_libraries(ttx PUBLIC Dius::dius Di::di)

install(TARGETS ttx RUNTIME DESTINATION bin)
