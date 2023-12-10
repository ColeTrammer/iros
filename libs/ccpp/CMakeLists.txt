cmake_minimum_required(VERSION 3.25.2)

project(Ccpp CXX)

set(to_filter x86_64 Linux Iros tests)
list(REMOVE_ITEM to_filter ${CMAKE_HOST_SYSTEM_PROCESSOR} ${CMAKE_SYSTEM_NAME})

function(filter input exclude_list output)
    foreach(exclude ${exclude_list})
        string(TOLOWER ${exclude} exclude)
        list(FILTER input EXCLUDE REGEX ${exclude})
    endforeach()

    set(${output}
        ${input}
        PARENT_SCOPE
    )
endfunction()

file(
    GLOB_RECURSE sources CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "*.cpp"
)
file(
    GLOB_RECURSE headers CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "include/*.h"
)

filter("${sources}" "${to_filter}" sources)
filter("${headers}" "${to_filter}" headers)

add_library(ccpp ${sources})
add_library(Ccpp::ccpp ALIAS ccpp)

target_sources(
    ccpp
    INTERFACE FILE_SET
              HEADERS
              TYPE
              HEADERS
              BASE_DIRS
              include
              FILES
              ${headers}
)

target_include_directories(
    ccpp SYSTEM PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
)

find_package(Dius REQUIRED)

target_link_libraries(ccpp PUBLIC Dius::dius)

target_compile_options(ccpp PRIVATE -ffreestanding -nostdinc++)

if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
    include(arch/x86_64/CMakeLists.txt)
else()
    message(FATAL_ERROR "The Iris kernel does not support the \"" ${CMAKE_HOST_SYSTEM_PROCESSOR} "\" architecture.")
endif()

install(
    TARGETS ccpp
    EXPORT CcppConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES
    DESTINATION include
    FILE_SET HEADERS
)
install(
    EXPORT CcppConfig
    FILE CcppConfig.cmake
    NAMESPACE Ccpp::
    DESTINATION lib/cmake/Ccpp
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/libpthread.a" "INPUT(libc.a)")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/libdl.a" "INPUT(libc.a)")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/librt.a" "INPUT(librt.a)")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/libm.a" "INPUT(libc.a)")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/libc.a" "INPUT(libccpp.a libdius.a)")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libpthread.a" DESTINATION lib)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libdl.a" DESTINATION lib)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/librt.a" DESTINATION lib)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libm.a" DESTINATION lib)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libc.a" DESTINATION lib)

if(IROS_BuildTests)
    add_subdirectory(tests)
endif()
