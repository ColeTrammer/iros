cmake_minimum_required(VERSION 3.25.2)

project(Dius CXX)

add_library(dius
    test/test_manager.cpp
    di_assert_impl.cpp
    memory_region.cpp
    sync_file.cpp
)
add_library(Dius::dius ALIAS dius)

target_include_directories(dius
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

find_package(Di REQUIRED)

target_link_libraries(dius
  PUBLIC
    Di::di
)
target_compile_definitions(dius PUBLIC DI_CUSTOM_ASSERT_HANDLER DI_CUSTOM_PLATFORM=<dius/platform.h>)

if(LINUX)
    target_sources(dius
      PRIVATE
        linux/filesystem/directory_iterator.cpp
        linux/io_uring.cpp
    )
endif()

install(
    TARGETS dius
    EXPORT DiusConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(
    EXPORT DiusConfig
    FILE DiusConfig.cmake
    NAMESPACE Dius::
    DESTINATION lib/cmake/Dius
)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "include"
    FILES_MATCHING PATTERN "*.h"
)

add_library(dius_test_main STATIC
    test/test_main.cpp
)
add_library(DiusTestMain::dius_test_main ALIAS dius_test_main)
target_link_libraries(dius_test_main
  PRIVATE
    dius
)

install(
    TARGETS dius_test_main
    EXPORT DiusTestMainConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(
    EXPORT DiusTestMainConfig
    FILE DiusTestMainConfig.cmake
    NAMESPACE DiusTestMain::
    DESTINATION lib/cmake/DiusTestMain
)