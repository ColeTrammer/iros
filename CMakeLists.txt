cmake_minimum_required(VERSION 3.25.2)

project(Iros CXX)

option(IROS_BuildIris "Build the iris kernel." OFF)
option(IROS_BuildUserspace "Build userspace libaries and programs." ON)
option(IROS_BuildTests "Build test programs." ON)
option(IROS_BuildTools "Build tools required native tools." ON)
option(IROS_BuildCcpp "Build the c++ libc." OFF)
option(IROS_UseDiusRuntime "Use dius provided runtime instead of system libc." OFF)
option(IROS_NeverBuildDocs "Never enable documentation build target." OFF)

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
set(IROS_IrisFlags ""
    CACHE STRING "Extra compilation flags only used by Iris."
)

separate_arguments(IROS_DiagnosticFlags)
separate_arguments(IROS_WarningFlags)
separate_arguments(IROS_SanitizerFlags)
separate_arguments(IROS_ExtraFlags)
separate_arguments(IROS_IrisFlags)

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
        if("${ARGV0}" STREQUAL iris)
            target_compile_options(${ARGV0}
              PRIVATE
                ${IROS_IrisFlags}
            )
        endif()
        target_link_options(
            ${ARGV0}
          PRIVATE
            ${IROS_DiagnosticFlags}
            ${IROS_WarningFlags}
            ${IROS_SanitizerFlags}
            ${IROS_ExtraFlags}
        )
        if (IROS_UseDiusRuntime AND NOT "${ARGV0}" STREQUAL iris)
            target_link_libraries(
                ${ARGV0}
              PRIVATE
                dius
            )
            target_compile_options(
                ${ARGV0}
              PRIVATE
                "-static"
            )
            target_link_options(
                ${ARGV0}
              PRIVATE
                "-static"
            )
        endif()
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
    add_subdirectory(userland/shell)
endif()

if(IROS_BuildTools)
    add_subdirectory(userland/tools)
endif()

if(IROS_BuildCcpp)
    add_subdirectory(libs/ccpp)
elseif(IROS_BuildTests)
    add_subdirectory(libs/ccpp/tests)
endif()

add_subdirectory(iris)
if(IROS_BuildIris)
    include(meta/cmake/iris_commands.cmake)
endif()

if (NOT IROS_NeverBuildDocs)
    include(meta/cmake/doxygen.cmake)
endif()
