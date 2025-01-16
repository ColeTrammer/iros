cmake_minimum_required(VERSION 3.25.2)

project(Ttx CXX)

find_package(Libttx REQUIRED)

file(GLOB headers CONFIGURE_DEPENDS "*.h")
add_library(ttx_headers INTERFACE)
add_library(Ttx::ttx_headers ALIAS ttx_headers)

target_sources(ttx_headers INTERFACE FILE_SET headers TYPE HEADERS FILES ${headers})
target_link_libraries(ttx_headers INTERFACE Libttx::libttx)

file(GLOB sources CONFIGURE_DEPENDS "*.cpp")

add_executable(ttx ${sources})
target_link_libraries(ttx PUBLIC Libttx::libttx)

install(TARGETS ttx RUNTIME DESTINATION bin)
