if(CMAKE_CROSSCOMPILING)
    include(ExternalProject)

    ExternalProject_Add(
        native
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        BINARY_DIR "../native/tools"
        INSTALL_DIR "../native/tools-install"
        CMAKE_ARGS "--preset=tools"
        CMAKE_CACHE_ARGS "-DCMAKE_INSTALL_PREFIX:STRING=<INSTALL_DIR>"
        BUILD_ALWAYS YES
        STEP_TARGETS install
    )

    set(INITRD_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/../native/tools-install/bin/initrd")
    set(INITRD_TARGET native-install)
else()
    set(INITRD_COMMAND initrd)
    set(INITRD_TARGET initrd)
endif()

add_custom_target(
    generate-initrd
    COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/initrd"
    COMMAND cd "${CMAKE_CURRENT_BINARY_DIR}/initrd"
    COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/iris/test_userspace" .
    COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/iris/test_create_task" .
    COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/iris/test_read" .
    COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/userland/shell/sh" .
    COMMAND cp "${CMAKE_CURRENT_SOURCE_DIR}/iris/data.txt" .
    COMMAND rm -f "${CMAKE_CURRENT_BINARY_DIR}/initrd/initrd.bin"
    COMMAND ${INITRD_COMMAND}
    DEPENDS ${INITRD_TARGET} test_userspace test_create_task test_read sh
)

add_custom_target(
    image
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND
        sudo IROS_ROOT="${CMAKE_CURRENT_SOURCE_DIR}" IROS_BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}"
        IROS_LIMINE_DIR="${LIMINE_DIR}" REMOTE_CONTAINERS="$ENV{REMOTE_CONTAINERS}"
        "${CMAKE_CURRENT_SOURCE_DIR}/meta/make-iris-limine-image.sh"
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/iris/iris.img"
    DEPENDS generate-initrd iris
    USES_TERMINAL
)

add_custom_target(
    run
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND IROS_ARCH=${CMAKE_HOST_SYSTEM_PROCESSOR} IROS_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/iris/iris.img
            "${CMAKE_CURRENT_SOURCE_DIR}/meta/run-iris.sh"
    USES_TERMINAL
)

add_custom_target(
    ib
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS generate-initrd image
)

add_custom_target(
    ibr
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND IROS_ARCH=${CMAKE_HOST_SYSTEM_PROCESSOR} IROS_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/iris/iris.img
            "${CMAKE_CURRENT_SOURCE_DIR}/meta/run-iris.sh"
    DEPENDS generate-initrd image
    USES_TERMINAL
)
