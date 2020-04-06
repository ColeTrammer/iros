cmake_minimum_required(VERSION 3.10)
project(os_2 C CXX ASM)

set(ARCH $ENV{ARCH} CACHE STRING "architecture for the os")
set(TARGET_OS $ENV{TARGET_OS} CACHE STRING "target operating system for build")

add_compile_definitions("ARCH=${ARCH}")
add_compile_definitions("_OS_2_SOURCE")
add_compile_definitions("KERNEL_NO_GRAPHICS")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -fdiagnostics-color=always -fno-omit-frame-pointer -std=gnu2x")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -fconcepts -fno-exceptions -fno-rtti -fdiagnostics-color=always -fno-omit-frame-pointer")

set(CMAKE_CXX_STANDARD 20)


set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(ISODIR ${ROOT}/isodir)
set(SYSROOT ${ROOT}/sysroot)

if (${TARGET_OS} STREQUAL os_2)
    set(CMAKE_INSTALL_PREFIX ${SYSROOT})
    set(NATIVE_DIR $ENV{OS_2_NATIVE_DIR} CACHE STRING "directory of native cmake build")
    set(NATIVE_INITRD ${NATIVE_DIR}/gen/initrd/initrd)
    set(NATIVE_PARSER ${NATIVE_DIR}/gen/parser/parser)
    include_directories(libs/libc/include)
else()
    set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/native)
    add_compile_definitions("_GNU_SOURCE")
    add_compile_definitions("_POSIX_C_SOURCE=201900L")
endif()

function(install_headers)
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
        DESTINATION "usr/include"
        FILES_MATCHING PATTERN "*.h"
    )
endfunction()

function(add_os_headers name)
    add_library(${name} INTERFACE)
    target_include_directories(${name} INTERFACE include)
    install_headers()
endfunction()

function(add_os_library name has_headers)
    add_library(${name} ${SOURCES})
    install(TARGETS ${name} ARCHIVE DESTINATION usr/lib)
    if (${has_headers})
        target_include_directories(${name} PUBLIC include)
        install_headers()
    endif()
endfunction()

add_custom_target(
    generate-initrd
    BYPRODUCTS ${SYSROOT}/boot/initrd.bin
    COMMAND ${NATIVE_INITRD} -e cmake_install.cmake initrd ${SYSROOT}/boot/initrd.bin > /dev/null
)

add_custom_target(
    os_2.iso
    BYPRODUCTS ${ROOT}/os_2.iso
    COMMAND /bin/mkdir -p ${ISODIR}/boot/grub
	COMMAND /bin/mkdir -p ${ISODIR}/modules
	COMMAND objcopy -S ${SYSROOT}/boot/boot ${ISODIR}/boot/boot
	COMMAND objcopy -S ${SYSROOT}/boot/kernel ${ISODIR}/modules/kernel
	COMMAND /bin/cp --preserve=timestamps ${SYSROOT}/boot/initrd.bin ${ISODIR}/modules/initrd.bin
	COMMAND /bin/cp --preserve=timestamps ${ROOT}/grub.cfg ${ISODIR}/boot/grub
	COMMAND grub-file --is-x86-multiboot2 ${ISODIR}/boot/boot
    COMMAND grub-mkrescue -o ${ROOT}/os_2.iso ${ISODIR}
    DEPENDS generate-initrd kernel boot
)

add_subdirectory(libs)
add_subdirectory(gen)
add_subdirectory(userland)
add_subdirectory(initrd)
add_subdirectory(boot)
add_subdirectory(kernel)