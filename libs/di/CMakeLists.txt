cmake_minimum_required(VERSION 3.25.2)

project(Di CXX)

add_library(di INTERFACE)
add_library(Di::di ALIAS di)

target_include_directories(
    di SYSTEM INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
)

file(GLOB_RECURSE headers CONFIGURE_DEPENDS *.h)
target_sources(
    di
    INTERFACE FILE_SET
              HEADERS
              TYPE
              HEADERS
              BASE_DIRS
              include
              FILES
              ${headers}
)

target_compile_features(di INTERFACE cxx_std_23)
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(di INTERFACE -fsized-deallocation)
endif()

target_compile_options(di INTERFACE -fno-exceptions -fno-rtti)

install(
    TARGETS di
    EXPORT DiConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES
    DESTINATION include
    FILE_SET HEADERS
)
install(
    EXPORT DiConfig
    FILE DiConfig.cmake
    NAMESPACE Di::
    DESTINATION lib/cmake/Di
)

if(IROS_BuildTests)
    add_subdirectory(tests)
endif()
