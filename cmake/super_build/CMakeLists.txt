cmake_minimum_required(VERSION 3.16.3)
project(iros_super_build
    LANGUAGES NONE
)

get_filename_component(
    IROS_PROJECT_ROOT "${PROJECT_SOURCE_DIR}/../.."
    ABSOLUTE CACHE
)

get_filename_component(
    NATIVE_TOOLS_PREFIX "${PROJECT_BINARY_DIR}/native-dist"
    ABSOLUTE CACHE
)

get_filename_component(
    IROS_TOOLCHAIN "${IROS_PROJECT_ROOT}/cmake/CMakeToolchain_$ENV{IROS_ARCH}.txt"
    ABSOLUTE CACHE
)

include(ExternalProject)

ExternalProject_Add(
    native
    SOURCE_DIR "${IROS_PROJECT_ROOT}"
    BINARY_DIR "${PROJECT_BINARY_DIR}/native"
    INSTALL_DIR "${NATIVE_TOOLS_PREFIX}"
    CMAKE_CACHE_DEFAULT_ARGS
        "-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>"
    BUILD_ALWAYS YES
    STEP_TARGETS install
)

ExternalProject_Add(
    iros
    SOURCE_DIR "${IROS_PROJECT_ROOT}"
    BINARY_DIR "${PROJECT_BINARY_DIR}/iros"
    INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/iros/sysroot"
    CMAKE_CACHE_DEFAULT_ARGS
        "-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>"
        "-DCMAKE_TOOLCHAIN_FILE:PATH=${IROS_TOOLCHAIN}"
        "-DNATIVE_TOOLS_PREFIX:PATH=${NATIVE_TOOLS_PREFIX}"
        "-DCMAKE_C_COMPILER_WORKS:BOOL=TRUE"
        "-DCMAKE_CXX_COMPILER_WORKS:BOOL=TRUE"
    BUILD_ALWAYS YES
    DEPENDS native-install
    STEP_TARGETS bootstrap-core-libs
)
ExternalProject_Add_Step(
    iros
    bootstrap-core-libs
    COMMAND "${CMAKE_COMMAND}" --build "${PROJECT_BINARY_DIR}/iros" -t bootstrap-core-libs
    DEPENDEES configure
    EXCLUDE_FROM_MAIN
    bootstrap-core-libs
)
