cmake_minimum_required(VERSION 3.25.2)

project(GfxTest CXX)

find_package(Dius REQUIRED)

file(GLOB headers CONFIGURE_DEPENDS "*.h")
add_library(gfx_test_headers INTERFACE)
add_library(PackageManager::gfx_test_headers ALIAS gfx_test_headers)

target_sources(gfx_test_headers INTERFACE FILE_SET headers TYPE HEADERS FILES ${headers})
target_link_libraries(gfx_test_headers INTERFACE Dius::dius Di::di)

file(GLOB sources CONFIGURE_DEPENDS "*.cpp")

find_path(WAYLAND_INCLUDE_DIRS wayland-client.h)
find_library(WAYLAND_LIBRARIES NAMES wayland-client)

add_executable(gfx_test ${sources})
target_link_libraries(gfx_test PUBLIC Dius::dius Di::di)

if(NOT WAYLAND_INCLUDE_DIRS OR NOT WAYLAND_LIBRARIES)
    message(FATAL_ERROR "Wayland not found")
endif()

include_directories(${WAYLAND_INCLUDE_DIRS})
target_link_libraries(gfx_test PUBLIC ${WAYLAND_LIBRARIES})

install(TARGETS gfx_test RUNTIME DESTINATION bin)

set_property(SOURCE xdg-shell.cpp PROPERTY SKIP_UNITY_BUILD_INCLUSION ON)
