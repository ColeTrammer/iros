cmake_minimum_required(VERSION 3.16.3)
project(iros C CXX ASM)

set(ARCH ${CMAKE_SYSTEM_PROCESSOR} CACHE STRING "architecture for the os")
set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(MINIMAL_BUILD FALSE CACHE STRING "")
if (${CMAKE_SYSTEM_NAME} STREQUAL Iros)
    set(NATIVE_BUILD FALSE)
else()
    set(NATIVE_BUILD TRUE)
endif()

if (${NATIVE_BUILD} AND NOT ${MINIMAL_BUILD})
    enable_testing()
endif()

add_compile_definitions("ARCH=${ARCH}")
add_compile_definitions("_IROS_SOURCE")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -Wextra -Werror -ftrack-macro-expansion=1 -fdiagnostics-color=always -fno-omit-frame-pointer -std=gnu2x -U${ARCH}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra -Werror -ftrack-macro-expansion=1 -fconcepts -fcoroutines -fno-exceptions -fno-rtti -fdiagnostics-color=always -fno-omit-frame-pointer -Wno-literal-suffix -U${ARCH}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INSTALL_MESSAGE NEVER)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(ISODIR ${CMAKE_CURRENT_BINARY_DIR}/isodir)
set(SYSROOT ${CMAKE_CURRENT_BINARY_DIR}/sysroot)

if (${NATIVE_BUILD})
    add_compile_definitions("_GNU_SOURCE")
    add_compile_definitions("_POSIX_C_SOURCE=201900L")
    add_compile_definitions("RESOURCE_ROOT=\"${ROOT}/base\"")
    set(DL_LIB dl)
    set(PTHREAD_LIB pthread)
    set(REALTIME_LIB rt)
    set(NATIVE_INITRD initrd)
    set(NATIVE_PARSER parser)
else()
    set(CMAKE_INSTALL_PREFIX ${SYSROOT})
    add_compile_definitions("RESOURCE_ROOT=\"\"")
    set(NATIVE_INITRD ${NATIVE_TOOLS_PREFIX}/bin/initrd)
    set(NATIVE_PARSER ${NATIVE_TOOLS_PREFIX}/bin/parser)
    set(DL_LIB libdl)
    set(PTHREAD_LIB libpthread)
    set(REALTIME_LIB librt)

    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --sysroot=${SYSROOT}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --sysroot=${SYSROOT}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --sysroot=${SYSROOT}")
    include_directories(libs/libc/include)
endif()

include(cmake/functions.cmake)

add_subdirectory(libs)
add_subdirectory(gen)
if (NOT ${MINIMAL_BUILD})
    add_subdirectory(userland)
    add_subdirectory(initrd)
    add_subdirectory(kernel)
    add_subdirectory(tests)
endif()

if (NOT ${NATIVE_BUILD})
    add_custom_target(
        add-gcc-libs-to-sysroot
        COMMAND "ARCH=${ARCH}" "SYSROOT=${SYSROOT}" "${ROOT}/scripts/add-gcc-libs-to-sysroot.sh"
        DEPENDS bootstrap-core-libs
    )

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
        iso
        BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/iros.iso
        COMMAND /bin/mkdir -p ${ISODIR}/boot/grub
        COMMAND /bin/mkdir -p ${ISODIR}/modules
        COMMAND objcopy -S ${SYSROOT}/boot/kernel ${ISODIR}/boot/kernel
        COMMAND /bin/cp --preserve=timestamps ${SYSROOT}/boot/initrd.bin ${ISODIR}/modules/initrd.bin
        COMMAND /bin/echo "set menu_color_normal=white/black" >| ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo "set menu_color_highlight=black/light-gray" >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo "menuentry \"Iros\" {" >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo "    multiboot2 /boot/kernel $$IROS_CMDLINE" >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo "    module2 /modules/initrd.bin initrd" >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo "}" >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo "set timeout_style=menu" >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND /bin/echo "set timeout=0" >> ${ISODIR}/boot/grub/grub.cfg
        COMMAND grub-file --is-x86-multiboot2 ${ISODIR}/boot/kernel
        COMMAND grub-mkrescue -o ${CMAKE_CURRENT_BINARY_DIR}/iros.iso ${ISODIR}
        DEPENDS generate-initrd kernel install-all
    )

    add_custom_target(
        image
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND sudo ROOT="${ROOT}"
                OUTPUT_DIR="${CMAKE_CURRENT_BINARY_DIR}"
                SYSROOT="${SYSROOT}"
                IROS_GRUB_IMAGE="$$IROS_GRUB_IMAGE"
                "${ROOT}/scripts/makeimg.sh"
        DEPENDS install-all
        USES_TERMINAL
    )

    add_custom_target(
        run
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND "IROS_ARCH=${ARCH}" "IROS_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/.." "${ROOT}/scripts/qemu.sh"
        USES_TERMINAL
    )

    add_custom_target(
        brun
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND [ -e bochsrc.txt ] || cp "${ROOT}/scripts/bochsrc.txt" .
        COMMAND "${ROOT}/scripts/bochs.sh"
        USES_TERMINAL
    )

    add_custom_target(
        frun
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target install
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target iso
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target image
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target run
        USES_TERMINAL
    )

    add_custom_target(
        bfrun
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target install
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target iso
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target image
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target brun
        USES_TERMINAL
    )
endif()
