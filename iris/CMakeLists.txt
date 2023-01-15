include(ExternalProject)

ExternalProject_Add(limine
    GIT_REPOSITORY https://github.com/limine-bootloader/limine.git
    GIT_TAG v4.x-branch-binary
    GIT_SHALLOW TRUE
    BUILD_IN_SOURCE TRUE
    CONFIGURE_COMMAND ""
    BUILD_COMMAND "make"
    INSTALL_COMMAND ""
)
set(LIMINE_DIR "${CMAKE_CURRENT_BINARY_DIR}/limine-prefix/src/limine")

set(IRIS_ARCH "${ARCH}")

if(IRIS_ARCH STREQUAL "x86_64")
    set(IRIS_ARCH_DIR "arch/x86/amd64")
elseif(IRIS_ARCH STREQUAL "i686")
    set(IRIS_ARCH_DIR "arch/x86/i686")
else()
    message(FATAL_ERROR "The Iris kernel does not support the \"" ${IRIS_ARCH} "\" architecture.")
endif()

include("${IRIS_ARCH_DIR}/CMakeLists.txt")

set(SOURCES
    ${IRIS_ARCH_SOURCES}
    boot/init.cpp
    mm/page_frame_allocator.cpp
    mm/heap.cpp
)

add_custom_target(
    iris_image
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND sudo ROOT="${ROOT}"
            BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}"
            LIMINE_DIR="${LIMINE_DIR}"
            "${ROOT}/scripts/make-iris-limine-img.sh"
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/iris.img"
    DEPENDS iris install-all
    USES_TERMINAL
)

add_custom_target(
    iris_run
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND IRIS_ARCH=${IRIS_ARCH}
            IRIS_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/iris.img
            ${ROOT}/scripts/run-iris.sh
    USES_TERMINAL
)

add_custom_target(
    ibr
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND IRIS_ARCH=${IRIS_ARCH}
            IRIS_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/iris.img
            ${ROOT}/scripts/run-iris.sh
    DEPENDS iris_image
    USES_TERMINAL
)

add_os_headers(iris_headers)
add_executable(iris ${SOURCES})

target_compile_options(iris PRIVATE -ffreestanding ${IRIS_ARCH_CXXFLAGS})
target_link_options(iris PRIVATE -nostdlib ${IRIS_ARCH_LDFLAGS})
target_link_libraries(iris PRIVATE libdi iris_headers)
target_include_directories(iris PRIVATE "${LIMINE_DIR}")
target_compile_definitions(iris PRIVATE "DI_CUSTOM_ASSERT_HANDLER" "DI_NO_ASSERT_ALLOCATION" "DI_CUSTOM_PLATFORM=<iris/core/platform.h>")
add_dependencies(iris limine)

set_target_properties(iris PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${IRIS_ARCH_DIR}/link.ld)

install(TARGETS iris RUNTIME DESTINATION boot)
