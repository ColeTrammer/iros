cmake_minimum_required(VERSION 3.20.3)
project(os_2 C CXX ASM)

set(ARCH ${CMAKE_SYSTEM_PROCESSOR} CACHE STRING "architecture for the os")
set(TARGET_OS ${CMAKE_SYSTEM_NAME} CACHE STRING "target operating system for build")
set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})
if (${TARGET_OS} STREQUAL OS_2)
    set(NATIVE_BUILD FALSE)
else()
    set(NATIVE_BUILD TRUE)
endif()

add_compile_definitions("ARCH=${ARCH}")
add_compile_definitions("_OS_2_SOURCE")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -Wextra -Werror -fdiagnostics-color=always -fno-omit-frame-pointer -std=gnu2x")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra -Werror -fconcepts -fcoroutines -fno-exceptions -fno-rtti -fdiagnostics-color=always -fno-omit-frame-pointer")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INSTALL_MESSAGE NEVER)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(ISODIR ${ROOT}/isodir)
set(SYSROOT ${ROOT}/sysroot)

if (${NATIVE_BUILD})
    add_compile_definitions("_GNU_SOURCE")
    add_compile_definitions("_POSIX_C_SOURCE=201900L")
    set(DL_LIB dl)
    set(PTHREAD_LIB pthread)
    set(REALTIME_LIB rt)
    set(NATIVE_INITRD initrd)
    set(NATIVE_PARSER parser)
else()
    set(CMAKE_INSTALL_PREFIX ${SYSROOT})
    set(NATIVE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/native CACHE STRING "directory of native cmake build")
    set(NATIVE_INITRD ${NATIVE_DIR}/gen/initrd/initrd)
    set(NATIVE_PARSER ${NATIVE_DIR}/gen/parser/parser)
    set(DL_LIB libdl)
    set(PTHREAD_LIB libpthread)
    set(REALTIME_LIB librt)
    include_directories(libs/libc/include)
endif()

include(cmake/functions.cmake)

add_subdirectory(libs)
add_subdirectory(gen)
add_subdirectory(userland)
add_subdirectory(initrd)
add_subdirectory(kernel)

if (NOT ${NATIVE_BUILD})
    add_custom_target(
        generate-initrd
        BYPRODUCTS ${SYSROOT}/boot/initrd.bin
        COMMAND /bin/mkdir -p ${SYSROOT}/boot
        COMMAND ${NATIVE_INITRD} -e cmake_install.cmake initrd ${SYSROOT}/boot/initrd.bin > /dev/null
        DEPENDS ${INITRD_TARGETS}
    )

    add_custom_target(
        install-all
        COMMAND ${CMAKE_COMMAND} -DBUILD_TYPE=${CMAKE_BUILD_TYPE} -P ${CMAKE_BINARY_DIR}/cmake_install.cmake
        DEPENDS ${ALL_TARGETS}
    )

    add_custom_target(
        os_2.iso
        BYPRODUCTS ${ROOT}/os_2.iso
        COMMAND /bin/mkdir -p ${ISODIR}/boot/grub
        COMMAND /bin/mkdir -p ${ISODIR}/modules
        COMMAND objcopy -S ${SYSROOT}/boot/kernel ${ISODIR}/boot/kernel
        COMMAND /bin/cp --preserve=timestamps ${SYSROOT}/boot/initrd.bin ${ISODIR}/modules/initrd.bin
        COMMAND /bin/echo "set menu_color_normal=white/black" >| ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo "set menu_color_highlight=black/light-gray" >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo "menuentry \"os_2\" {" >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo "    multiboot2 /boot/kernel $$OS_2_CMDLINE" >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo "    module2 /modules/initrd.bin initrd" >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo "}" >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo "set timeout_style=menu" >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo "set timeout=0" >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND grub-file --is-x86-multiboot2 ${ISODIR}/boot/kernel
        COMMAND grub-mkrescue -o ${ROOT}/os_2.iso ${ISODIR}
        DEPENDS generate-initrd kernel
    )

    add_custom_target(
        os_2.img
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND sudo ./makeimg.sh
        USES_TERMINAL
    )

    add_custom_target(
        run
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ./qemu.sh
        USES_TERMINAL
    )

    add_custom_target(
        brun
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ./bochs.sh
        USES_TERMINAL
    )

    add_custom_target(
        frun
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target install
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target os_2.iso
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target os_2.img
        COMMAND ./qemu.sh
        USES_TERMINAL
    )

    add_custom_target(
        bfrun
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target install
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target os_2.iso
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target os_2.img
        COMMAND ./bochs.sh
        USES_TERMINAL
    )
endif()
