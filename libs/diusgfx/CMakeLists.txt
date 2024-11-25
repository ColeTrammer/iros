cmake_minimum_required(VERSION 3.25.2)

project(Diusgfx CXX)

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

add_library(diusgfx ${sources})
add_library(Diusgfx::diusgfx ALIAS diusgfx)

target_sources(
    diusgfx
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
    diusgfx PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
)

find_package(Dius REQUIRED)

target_link_libraries(diusgfx PUBLIC Dius::dius)

install(
    TARGETS diusgfx
    EXPORT DiusgfxConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES
    DESTINATION include
    FILE_SET HEADERS
)
install(
    EXPORT DiusgfxConfig
    FILE DiusgfxConfig.cmake
    NAMESPACE Diusgfx::
    DESTINATION lib/cmake/Diusgfx
)

if(IROS_BuildTests)
    add_subdirectory(tests)
endif()
