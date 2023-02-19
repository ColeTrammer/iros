cmake_minimum_required(VERSION 3.25.2)

project(iros CXX)

option(IROS_BuildIris "Build the iris kernel." OFF)
option(IROS_BuildUserspace "Build userspace libaries and programs." ON)
option(IROS_BuildTests "Build test programs." ON)
option(IROS_BuildTools "Build tools required native tools." ON)
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
set(IROS_ExtraFlags ""
    CACHE STRING "Extra compilation flags for any purpose."
)

separate_arguments(IROS_DiagnosticsFlags)
separate_arguments(IROS_WarningFlags)
separate_arguments(IROS_SanitizerFlags)
separate_arguments(IROS_ExtraFlags)

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
            ${IROS_ExtraFlags}
        )
        target_link_options(${ARGV0}
          PRIVATE
            ${IROS_DiagnosticFlags}
            ${IROS_WarningFlags}
            ${IROS_SanitizerFlags}
            ${IROS_ExtraFlags}
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
            ${IROS_ExtraFlags}
        )
        target_link_options(
            ${ARGV0}
          PRIVATE
            ${IROS_DiagnosticFlags}
            ${IROS_WarningFlags}
            ${IROS_SanitizerFlags}
            ${IROS_ExtraFlags}
        )
    endif()
endmacro()

if (IROS_BuildTests)
    enable_testing()
endif()

add_subdirectory(libs/di)

if(IROS_BuildUserspace OR IROS_BuildTools OR IROS_BuildCcpp)
    add_subdirectory(libs/dius)
endif()

if(IROS_BuildUserspace)
    add_subdirectory(userland/core)
endif()

if(IROS_BuildTools)
    add_subdirectory(userland/tools)
endif()

if(IROS_BuildCcpp)
    add_subdirectory(libs/ccpp)
endif()

add_subdirectory(iris)
if(IROS_BuildIris)
    include(meta/cmake/iris_commands.cmake)
endif()