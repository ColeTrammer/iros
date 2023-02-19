cmake_minimum_required(VERSION 3.25.2)

project(Ccpp CXX)

add_library(ccpp
    string/memcpy.cpp
    string/memmove.cpp
    string/memset.cpp
    string/strchr.cpp
    string/strlen.cpp
    string/strstr.cpp
)
add_library(Ccpp::ccpp ALIAS ccpp)

target_include_directories(ccpp
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

find_package(Dius REQUIRED)

target_link_libraries(ccpp
  PRIVATE
    Dius::dius
)

target_compile_options(ccpp
  PRIVATE
    -ffreestanding
    -nostdinc++
)

install(
    TARGETS ccpp
    EXPORT CcppConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(
    EXPORT CcppConfig
    FILE CcppConfig.cmake
    NAMESPACE Ccpp::
    DESTINATION lib/cmake/Ccpp
)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "include"
    FILES_MATCHING PATTERN "*.h"
)

if (IROS_BuildTests)
    add_subdirectory(tests)
endif()