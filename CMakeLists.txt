cmake_minimum_required(VERSION 3.10)
project(os_2 C CXX ASM)

set(ARCH $ENV{ARCH} CACHE STRING "architecture for the os")
set(TARGET_OS $ENV{TARGET_OS} CACHE STRING "target operating system for build")
set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})
if (${TARGET_OS} STREQUAL os_2)
    set(NATIVE_BUILD FALSE)
else()
    set(NATIVE_BUILD TRUE)
endif()

add_compile_definitions("ARCH=${ARCH}")
add_compile_definitions("_OS_2_SOURCE")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -Wextra -fdiagnostics-color=always -fno-omit-frame-pointer -std=gnu2x")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra -fconcepts -fno-exceptions -fno-rtti -fdiagnostics-color=always -fno-omit-frame-pointer")

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
    set(NATIVE_DIR $ENV{OS_2_NATIVE_DIR} CACHE STRING "directory of native cmake build")
    set(NATIVE_INITRD ${NATIVE_DIR}/gen/initrd/initrd)
    set(NATIVE_PARSER ${NATIVE_DIR}/gen/parser/parser)
    set(DL_LIB libdl)
    set(PTHREAD_LIB libpthread)
    set(REALTIME_LIB librt)
    include_directories(libs/libc/include)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()

function(install_headers)
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
        DESTINATION "usr/include"
        FILES_MATCHING PATTERN "*.h"
    )
endfunction()

function(add_all_targets)
    foreach(TARGET ${TARGETS})
        add_subdirectory(${TARGET})
    endforeach()
endfunction()

function(add_os_headers name)
    add_library(${name} INTERFACE)
    target_include_directories(${name} INTERFACE include)
    install_headers()
endfunction()

function(add_os_static_library target_name short_name has_headers)
    add_library(${target_name} STATIC ${SOURCES})
    install(TARGETS ${target_name} ARCHIVE DESTINATION usr/lib)
    if (${has_headers})
        target_include_directories(${target_name} PUBLIC include)
        install_headers()
    endif()
    target_compile_definitions(${target_name} PRIVATE __is_static)
    set_target_properties(${target_name} PROPERTIES OUTPUT_NAME ${short_name})
endfunction()

function(add_os_library target_name short_name has_headers)
    add_library(${target_name} SHARED ${SOURCES})
    install(TARGETS ${target_name} LIBRARY DESTINATION usr/lib)
    if (${has_headers})
        target_include_directories(${target_name} PUBLIC include)
        install_headers()
    endif()
    target_compile_definitions(${target_name} PRIVATE __is_shared)
    set_target_properties(${target_name} PROPERTIES OUTPUT_NAME ${short_name})
endfunction()

function(add_os_executable name dest_dir)
    add_executable(${name} ${SOURCES})
    if (NOT ${NATIVE_BUILD})
        add_dependencies(${name} install-libc)
        target_link_options(${name} PRIVATE "-Wl,-rpath-link" "${ROOT}/base/usr/lib") # Make the linker stop complaining about libgcc_s.so
    endif()
    install(TARGETS ${name} RUNTIME DESTINATION ${dest_dir})
endfunction()

function(add_os_static_executable name dest_dir)
    add_executable(${name} ${SOURCES})
    if (NOT ${NATIVE_BUILD})
        add_dependencies(${name} install-libc)
        target_link_libraries(${name} PRIVATE libc_static)
    endif()
    target_link_options(${name} PRIVATE "-static")
    install(TARGETS ${name} RUNTIME DESTINATION ${dest_dir})
endfunction()

add_subdirectory(libs)
add_subdirectory(gen)
add_subdirectory(userland)
add_subdirectory(initrd)
add_subdirectory(kernel)
set(ALL_TARGETS ${LIBS_TARGETS} ${GEN_TARGETS} ${USERLAND_TARGETS} ${INITRD_TARGETS} ${BOOT_TARGETS} ${KERNEL_TARGETS})

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
        DEPENDS generate-initrd ${ALL_TARGETS} install-all
    )

    add_custom_target(
        os_2.img
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND sudo ./makeimg.sh
        USES_TERMINAL
        DEPENDS install-all
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
        COMMAND ./qemu.sh
        USES_TERMINAL
        DEPENDS os_2.iso os_2.img
    )

    add_custom_target(
        bfrun
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ./bochs.sh
        USES_TERMINAL
        DEPENDS os_2.iso os_2.img
    )
endif()
