add_custom_target(
    image
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND sudo IROS_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
            IROS_BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}"
            IROS_LIMINE_DIR="${LIMINE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/meta/make-iris-limine-image.sh"
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/iris/iris.img"
    DEPENDS iris
    USES_TERMINAL
)

add_custom_target(
    run
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND IRIS_ARCH=${CMAKE_HOST_SYSTEM_PROCESSOR}
            IRIS_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/iris/iris.img
            "${CMAKE_CURRENT_SOURCE_DIR}/meta/run-iris.sh"
    USES_TERMINAL
)

add_custom_target(
    ibr
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND IRIS_ARCH=${CMAKE_HOST_SYSTEM_PROCESSOR}
            IRIS_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/iris/iris.img
            "${CMAKE_CURRENT_SOURCE_DIR}/meta/run-iris.sh"
    DEPENDS image
    USES_TERMINAL
)