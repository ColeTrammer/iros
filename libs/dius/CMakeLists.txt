cmake_minimum_required(VERSION 3.25.2)

project(Dius CXX)

set(to_filter x86_64 posix linux iros runtime tests)
list(REMOVE_ITEM to_filter ${CMAKE_HOST_SYSTEM_PROCESSOR})

if(UNIX AND NOT IROS_UseDiusRuntime)
    list(REMOVE_ITEM to_filter posix)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(REMOVE_ITEM to_filter linux)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Iros")
    list(REMOVE_ITEM to_filter iros)
endif()
if(IROS_UseDiusRuntime)
    list(REMOVE_ITEM to_filter runtime)
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

filter("${sources}" "${to_filter}" sources)
filter("${headers}" "${to_filter}" headers)

list(REMOVE_ITEM sources "test/test_main.cpp")

add_library(dius ${sources})
add_library(Dius::dius ALIAS dius)

target_sources(
    dius
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
    dius SYSTEM PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
)

find_package(Di REQUIRED)

target_link_libraries(dius PUBLIC Di::di)
target_compile_definitions(dius PUBLIC DI_CUSTOM_ASSERT_HANDLER DI_CUSTOM_PLATFORM=<dius/platform.h>)

if(CMAKE_SYSTEM_NAME STREQUAL "Iros")
    if(CMAKE_CROSSCOMPILING)
        find_package(Iris REQUIRED)

        target_link_libraries(dius PUBLIC Iris::iris_headers)
    endif()
endif()

install(
    TARGETS dius
    EXPORT DiusConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES
    DESTINATION include
    FILE_SET HEADERS
)
install(
    EXPORT DiusConfig
    FILE DiusConfig.cmake
    NAMESPACE Dius::
    DESTINATION lib/cmake/Dius
)

if(IROS_UseDiusRuntime)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/crt0.o" "INPUT(libdius.a)")

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/crt0.o" DESTINATION lib)

    target_compile_definitions(dius PUBLIC DIUS_USE_RUNTIME DI_NO_USE_STD)

    target_compile_options(
        dius
        PUBLIC "-nostdlib" "-static"
        PRIVATE "-fno-stack-protector" "-ffreestanding"
    )
    target_link_options(
        dius
        PUBLIC
        "-nostdlib"
        "-static"
        PRIVATE
        "-fno-stack-protector"
        "-ffreestanding"
    )
endif()

add_library(dius_test_main STATIC test/test_main.cpp)
add_library(Dius::dius_test_main ALIAS dius_test_main)
target_link_libraries(dius_test_main PRIVATE dius)

install(
    TARGETS dius_test_main
    EXPORT DiusTestMainConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES
    DESTINATION include
)
install(
    EXPORT DiusTestMainConfig
    FILE DiusTestMainConfig.cmake
    NAMESPACE Dius::
    DESTINATION lib/cmake/DiusTestMain
)

if(IROS_BuildTests)
    add_subdirectory(tests)
endif()
