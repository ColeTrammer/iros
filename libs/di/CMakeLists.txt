cmake_minimum_required(VERSION 3.25.2)

project(Di CXX)

add_library(di INTERFACE)
add_library(Di::di ALIAS di)

target_include_directories(di
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(di
  INTERFACE
    cxx_std_23
)

target_compile_options(di
  INTERFACE
    -fno-exceptions
    -fno-rtti
)

install(
    TARGETS di
    EXPORT DiConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(
    EXPORT DiConfig
    FILE DiConfig.cmake
    NAMESPACE Di::
    DESTINATION lib/cmake/Di
)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "include"
    FILES_MATCHING PATTERN "*.h"
)

add_subdirectory(tests)