cmake_minimum_required(VERSION 3.25.2)

project(iros CXX)

option(IROS_BuildIris "Build the iris kernel." OFF)
option(IROS_BuildCcpp "Build the c++ libc." ON)

set(IROS_DiagnosticFlags ""
    CACHE STRING "Extra compilation flags for diagnostics purposes."
)
set(IROS_WarningFlags ""
    CACHE STRING "Extra compilation flags to set warnings."
)
set(IROS_SanitizerFlags ""
    CACHE STRING "Extra compilation flags to enable sanitizers."
)

separate_arguments(IROS_DiagnosticsFlags)
separate_arguments(IROS_WarningFlags)
separate_arguments(IROS_SanitizerFlags)

set(as_subproject Di Dius Iris)

macro(find_package)
    if (NOT "${ARGV0}" IN_LIST as_subproject)
        _find_package(${ARGV})
    endif()
endmacro()

set(not_buildable_library ALIAS INTERFACE IMPORTED)

macro(add_library)
    _add_library(${ARGV})
    if (NOT "${ARGV1}" IN_LIST not_buildable_library)
        target_compile_options(${ARGV0}
          PRIVATE
            ${IROS_DiagnosticFlags}
            ${IROS_WarningFlags}
            ${IROS_SanitizerFlags}
        )
        target_link_options(${ARGV0}
          PRIVATE
            ${IROS_DiagnosticFlags}
            ${IROS_WarningFlags}
            ${IROS_SanitizerFlags}
        )
    endif()
endmacro()

macro(add_executable)
    _add_executable(${ARGV})
    if (NOT "${ARGV1}" IN_LIST not_buildable_library)
        target_compile_options(${ARGV0}
          PRIVATE
            ${IROS_DiagnosticFlags}
            ${IROS_WarningFlags}
            ${IROS_SanitizerFlags}
        )
        target_link_options(
            ${ARGV0}
          PRIVATE
            ${IROS_DiagnosticFlags}
            ${IROS_WarningFlags}
            ${IROS_SanitizerFlags}
        )
    endif()
endmacro()

enable_testing()

add_subdirectory(libs/di)
add_subdirectory(libs/dius)

if(IROS_BuildCcpp)
    add_subdirectory(libs/ccpp)
endif()

if(IROS_BuildIris)
    add_subdirectory(iris)
    include(meta/cmake/iris_commands.cmake)
endif()