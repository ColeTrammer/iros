cmake_minimum_required(VERSION 3.25.2)

project(Iris CXX)

set(to_filter amd64 tests)
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    list(REMOVE_ITEM to_filter amd64)
endif()

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
file(
    GLOB_RECURSE uapi_headers CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "include/iris/uapi/*.h"
)

list(REMOVE_ITEM sources test_create_task.cpp test_read.cpp test_userspace.cpp)

filter("${sources}" "${to_filter}" sources)
list(APPEND to_filter "uapi")
filter("${headers}" "${to_filter}" headers)

add_library(iris_headers INTERFACE)
add_library(Iris::iris_headers ALIAS iris_headers)

target_include_directories(
    iris_headers SYSTEM INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
)

target_sources(
    iris_headers
    INTERFACE FILE_SET
              HEADERS
              TYPE
              HEADERS
              BASE_DIRS
              include
              FILES
              ${uapi_headers}
)

find_package(Di REQUIRED)

target_link_libraries(iris_headers INTERFACE Di::di)

install(
    TARGETS iris_headers
    EXPORT IrisHeadersConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES
    DESTINATION include
    FILE_SET HEADERS
)
install(
    EXPORT IrisHeadersConfig
    FILE IrisHeadersConfig.cmake
    NAMESPACE Iris::
    DESTINATION lib/cmake/IrisHeaders
)

if(IROS_BuildIris)
    include(ExternalProject)

    ExternalProject_Add(
        limine
        GIT_REPOSITORY https://github.com/limine-bootloader/limine.git
        GIT_TAG v4.x-branch-binary
        GIT_SHALLOW TRUE
        TMP_DIR "${CMAKE_SOURCE_DIR}/build/host/tools/limine/tmp"
        STAMP_DIR "${CMAKE_SOURCE_DIR}/build/host/tools/limine/stamp"
        DOWNLOAD_DIR "${CMAKE_SOURCE_DIR}/build/host/tools/limine"
        SOURCE_DIR "${CMAKE_SOURCE_DIR}/build/host/tools/limine/src"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND ""
        BUILD_COMMAND "make"
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
    )
    set(LIMINE_DIR
        "${CMAKE_SOURCE_DIR}/build/host/tools/limine/src"
        PARENT_SCOPE
    )
    set(LIMINE_DIR "${CMAKE_SOURCE_DIR}/build/host/tools/limine/src")

    add_library(iris_internal_headers INTERFACE)
    add_library(Iris::iris_internal_headers ALIAS iris_internal_headers)

    target_include_directories(
        iris_internal_headers SYSTEM INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                               $<INSTALL_INTERFACE:include>
    )

    target_sources(
        iris_internal_headers
        INTERFACE FILE_SET
                  HEADERS
                  TYPE
                  HEADERS
                  BASE_DIRS
                  include
                  FILES
                  ${headers}
    )

    add_executable(iris ${sources})

    if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
        include(arch/x86/amd64/CMakeLists.txt)
    else()
        message(FATAL_ERROR "The Iris kernel does not support the \"" ${CMAKE_HOST_SYSTEM_PROCESSOR} "\" architecture.")
    endif()

    target_compile_options(
        iris
        PRIVATE -ffreestanding
                -nostdlib
                -fno-pic
                -fno-pie
                -static
                -nostartfiles
                -fno-stack-protector
    )
    target_link_options(
        iris
        PRIVATE
        -ffreestanding
        -nostdlib
        -fno-pic
        -fno-pie
        -static
        -nostartfiles
    )
    target_link_libraries(iris PRIVATE di iris_headers iris_internal_headers)
    target_include_directories(iris SYSTEM PRIVATE "${LIMINE_DIR}")
    target_compile_definitions(
        iris PRIVATE "DI_CUSTOM_ASSERT_HANDLER" "DI_NO_ASSERT_ALLOCATION" "DI_NO_USE_STD"
                     "DI_CUSTOM_PLATFORM=<iris/core/platform.h>"
    )
    add_dependencies(iris limine)

    install(TARGETS iris RUNTIME DESTINATION boot)

    add_executable(test_userspace test_userspace.cpp)
    target_compile_options(test_userspace PRIVATE -ffreestanding -nostdlib -nostartfiles -fno-pic -fno-pie -static)
    target_link_options(
        test_userspace
        PRIVATE
        -ffreestanding
        -nostdlib
        -nostartfiles
        -fno-pic
        -fno-pie
        -static
    )
    install(TARGETS test_userspace RUNTIME DESTINATION boot)

    add_executable(test_create_task test_create_task.cpp)
    target_compile_options(test_create_task PRIVATE -ffreestanding -nostdlib -nostartfiles -fno-pic -fno-pie -static)
    target_link_options(
        test_create_task
        PRIVATE
        -ffreestanding
        -nostdlib
        -nostartfiles
        -fno-pic
        -fno-pie
        -static
    )
    install(TARGETS test_create_task RUNTIME DESTINATION boot)

    add_executable(test_read test_read.cpp)
    target_compile_options(test_read PRIVATE -ffreestanding -nostdlib -nostartfiles -fno-pic -fno-pie -static)
    target_link_options(
        test_read
        PRIVATE
        -ffreestanding
        -nostdlib
        -nostartfiles
        -fno-pic
        -fno-pie
        -static
    )
    install(TARGETS test_read RUNTIME DESTINATION boot)

    if(IROS_BuildTests)
        add_subdirectory(tests/integration)
        include(tests/unit/CMakeLists.txt)
    endif()
endif()
