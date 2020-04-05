cmake_minimum_required(VERSION 3.10)
project(os_2 C CXX ASM)

set(ARCH x86_64)
set(TARGET_OS os_2)

add_compile_definitions("ARCH=${ARCH}")
add_compile_definitions("_OS_2_SOURCE")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -fdiagnostics-color=always")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -fconcepts -fno-exceptions -fno-rtti -fdiagnostics-color=always")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

set(NATIVE_PARSER ${CMAKE_CURRENT_SOURCE_DIR}/native/bin/parser)

if (${TARGET_OS} STREQUAL os_2)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/sysroot)
    include_directories(libs/libc/include)
else()
    set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/native)
    add_compile_definitions("_GNU_SOURCE")
    add_compile_definitions("_POSIX_C_SOURCE=201900L")
endif()


add_subdirectory(libs)
add_subdirectory(gen)
add_subdirectory(userland)
add_subdirectory(initrd)
add_subdirectory(boot)
add_subdirectory(kernel)