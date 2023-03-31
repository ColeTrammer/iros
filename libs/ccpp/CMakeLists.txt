cmake_minimum_required(VERSION 3.25.2)

project(Ccpp CXX)

add_library(
    ccpp
    ctype/isalnum.cpp
    ctype/isalpha.cpp
    ctype/isascii.cpp
    ctype/isblank.cpp
    ctype/iscntrl.cpp
    ctype/isdigit.cpp
    ctype/isgraph.cpp
    ctype/islower.cpp
    ctype/isprint.cpp
    ctype/ispunct.cpp
    ctype/isspace.cpp
    ctype/isupper.cpp
    ctype/isxdigit.cpp
    ctype/tolower.cpp
    ctype/toupper.cpp
    errno/errno.cpp
    stdio/fclose.cpp
    stdio/fopen.cpp
    stdio/fseek.cpp
    stdio/ftell.cpp
    stdlib/aligned_alloc.cpp
    stdlib/atoi.cpp
    stdlib/calloc.cpp
    stdlib/free.cpp
    stdlib/malloc.cpp
    stdlib/realloc.cpp
    string/memcmp.cpp
    string/memcpy.cpp
    string/memmove.cpp
    string/memset.cpp
    string/strcat.cpp
    string/strchr.cpp
    string/strcmp.cpp
    string/strcpy.cpp
    string/strlen.cpp
    string/strncat.cpp
    string/strncmp.cpp
    string/strncpy.cpp
    string/strrchr.cpp
    string/strstr.cpp
    unistd/lseek.cpp
)
add_library(Ccpp::ccpp ALIAS ccpp)

set_target_properties(ccpp PROPERTIES OUTPUT_NAME "c")

target_include_directories(
    ccpp PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
)

find_package(Dius REQUIRED)

target_link_libraries(ccpp PUBLIC Dius::dius)

target_compile_options(ccpp PRIVATE -ffreestanding -nostdinc++)

install(
    TARGETS ccpp
    EXPORT CcppConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES
    DESTINATION include
)
install(
    EXPORT CcppConfig
    FILE CcppConfig.cmake
    NAMESPACE Ccpp::
    DESTINATION lib/cmake/Ccpp
)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "include"
    FILES_MATCHING
    PATTERN "*.h"
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/libpthread.a" "INPUT(libc.a)")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/libdl.a" "INPUT(libc.a)")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/librt.a" "INPUT(librt.a)")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/libm.a" "INPUT(libc.a)")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libpthread.a" DESTINATION lib)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libdl.a" DESTINATION lib)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/librt.a" DESTINATION lib)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libm.a" DESTINATION lib)

if(IROS_BuildTests)
    add_subdirectory(tests)
endif()
