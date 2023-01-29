cmake_minimum_required(VERSION 3.25.2)

project(iros CXX)

set(as_subproject Di Dius)

macro(find_package)
    if (NOT "${ARGV0}" IN_LIST as_subproject)
        _find_package(${ARGV})
    endif()
endmacro()

enable_testing()

add_subdirectory(iris)
add_subdirectory(libs/ccpp)
add_subdirectory(libs/di)
add_subdirectory(libs/dius)

include(meta/cmake/iris_commands.cmake)