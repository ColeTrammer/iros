cmake_minimum_required(VERSION 3.25.2)

project(Libttx CXX)

set(to_filter tests)

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

add_library(libttx ${sources})
add_library(Libttx::libttx ALIAS libttx)

target_sources(
    libttx
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
    libttx PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
)

find_package(Dius REQUIRED)

target_link_libraries(libttx PUBLIC Dius::dius)

install(
    TARGETS libttx
    EXPORT LibttxConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES
    DESTINATION include
    FILE_SET HEADERS
)
install(
    EXPORT LibttxConfig
    FILE LibttxConfig.cmake
    NAMESPACE Libttx::
    DESTINATION lib/cmake/Libttx
)

if(IROS_BuildTests)
    add_subdirectory(tests)
endif()
