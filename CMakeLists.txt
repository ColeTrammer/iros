cmake_minimum_required(VERSION 3.25.2)

project(iros CXX)

option(IROS_BuildIris "Build the iris kernel." OFF)

set(as_subproject Di Dius)

macro(find_package)
    if (NOT "${ARGV0}" IN_LIST as_subproject)
        _find_package(${ARGV})
    endif()
endmacro()

enable_testing()

add_subdirectory(libs/ccpp)
add_subdirectory(libs/di)
add_subdirectory(libs/dius)

if(IROS_BuildIris)
    add_subdirectory(iris)
    include(meta/cmake/iris_commands.cmake)
endif()