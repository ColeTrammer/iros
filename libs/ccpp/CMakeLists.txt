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
    getopt/getopt.cpp
    getopt/getopt_implementation.cpp
    getopt/getopt_long.cpp
    getopt/getopt_long_only.cpp
    getopt/optvars.cpp
    locale/setlocale.cpp
    stdio/fclose.cpp
    stdio/fdopen.cpp
    stdio/feof.cpp
    stdio/feof_unlocked.cpp
    stdio/fflush.cpp
    stdio/fflush_unlocked.cpp
    stdio/fgetc.cpp
    stdio/fgetc_unlocked.cpp
    stdio/fgets.cpp
    stdio/fgets_unlocked.cpp
    stdio/fileno.cpp
    stdio/fileno_unlocked.cpp
    stdio/fopen.cpp
    stdio/fprintf.cpp
    stdio/fputc.cpp
    stdio/fputc_unlocked.cpp
    stdio/fputs.cpp
    stdio/fputs_unlocked.cpp
    stdio/fread.cpp
    stdio/fread_unlocked.cpp
    stdio/fscanf.cpp
    stdio/fseek.cpp
    stdio/ftell.cpp
    stdio/fwrite.cpp
    stdio/fwrite_unlocked.cpp
    stdio/getc.cpp
    stdio/getc_unlocked.cpp
    stdio/getchar.cpp
    stdio/getchar_unlocked.cpp
    stdio/gets.cpp
    stdio/printf.cpp
    stdio/printf_implementation.cpp
    stdio/putc.cpp
    stdio/putc.cpp
    stdio/putc_unlocked.cpp
    stdio/putchar.cpp
    stdio/putchar_unlocked.cpp
    stdio/puts.cpp
    stdio/puts.cpp
    stdio/scanf.cpp
    stdio/scanf_implementation.cpp
    stdio/setvbuf.cpp
    stdio/snprintf.cpp
    stdio/sprintf.cpp
    stdio/sscanf.cpp
    stdio/stderr.cpp
    stdio/stdin.cpp
    stdio/stdout.cpp
    stdio/tmpfile.cpp
    stdio/ungetc.cpp
    stdio/vfprintf.cpp
    stdio/vfscanf.cpp
    stdio/vprintf.cpp
    stdio/vscanf.cpp
    stdio/vsnprintf.cpp
    stdio/vsprintf.cpp
    stdio/vsscanf.cpp
    stdlib/abort.cpp
    stdlib/aligned_alloc.cpp
    stdlib/atoi.cpp
    stdlib/atol.cpp
    stdlib/atoll.cpp
    stdlib/calloc.cpp
    stdlib/exit.cpp
    stdlib/free.cpp
    stdlib/getenv.cpp
    stdlib/malloc.cpp
    stdlib/realloc.cpp
    stdlib/strtod.cpp
    stdlib/strtol.cpp
    stdlib/strtoll.cpp
    stdlib/strtoul.cpp
    stdlib/strtoull.cpp
    string/memchr.cpp
    string/memcmp.cpp
    string/memcpy.cpp
    string/memmove.cpp
    string/memset.cpp
    string/strcat.cpp
    string/strchr.cpp
    string/strcmp.cpp
    string/strcoll.cpp
    string/strcpy.cpp
    string/strerror.cpp
    string/strlen.cpp
    string/strncat.cpp
    string/strncmp.cpp
    string/strncpy.cpp
    string/strrchr.cpp
    string/strstr.cpp
    string/strxfrm.cpp
    sys/stat/stat.cpp
    time/strftime.cpp
    unistd/lseek.cpp
    unistd/read.cpp
    unistd/write.cpp
)
add_library(Ccpp::ccpp ALIAS ccpp)

target_include_directories(
    ccpp SYSTEM PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
)

find_package(Dius REQUIRED)

target_link_libraries(ccpp PUBLIC Dius::dius)

target_compile_options(ccpp PRIVATE -ffreestanding -nostdinc++)

if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
    include(arch/x86_64/CMakeLists.txt)
else()
    message(FATAL_ERROR "The Iris kernel does not support the \"" ${CMAKE_HOST_SYSTEM_PROCESSOR} "\" architecture.")
endif()

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
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/libc.a" "INPUT(libccpp.a libdius.a)")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libpthread.a" DESTINATION lib)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libdl.a" DESTINATION lib)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/librt.a" DESTINATION lib)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libm.a" DESTINATION lib)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libc.a" DESTINATION lib)

if(IROS_BuildTests)
    add_subdirectory(tests)
endif()
