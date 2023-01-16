include(ExternalProject)

set(SOURCES
    di_assert_impl.cpp
    sync_file.cpp
)

add_os_library(libdius dius TRUE)
target_link_libraries(libdius PUBLIC libdi)
target_compile_definitions(libdius PUBLIC DI_CUSTOM_ASSERT_HANDLER)
target_compile_definitions(libdius PUBLIC DI_CUSTOM_PLATFORM=<dius/platform.h>)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND ${NATIVE_BUILD} AND NOT ${MINIMAL_BUILD})
    ExternalProject_Add(liburing_project
        PREFIX third_party
        GIT_REPOSITORY https://github.com/axboe/liburing.git
        GIT_TAG liburing-2.3
        GIT_SHALLOW TRUE
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND "./configure"
        BUILD_COMMAND "make"
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
    )

    SET(LIBURING_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/third_party/src/liburing_project/src/include)
    SET(LIBURING_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/third_party/src/liburing_project/src)

    execute_process(
        COMMAND mkdir -p "${LIBURING_LIB_DIR}"
        COMMAND mkdir -p "${LIBURING_INCLUDE_DIR}"
        COMMAND touch "${LIBURING_LIB_DIR}/liburing.so.2.3"
    )

    add_library(liburing SHARED IMPORTED)
    add_dependencies(liburing liburing_project)
    set_target_properties(liburing PROPERTIES IMPORTED_LOCATION ${LIBURING_LIB_DIR}/liburing.so.2.3)
    target_include_directories(liburing INTERFACE "${LIBURING_INCLUDE_DIR}")

    target_compile_definitions(libdius PUBLIC DIUS_HAVE_LIBURING DI_USE_STD)
    target_link_libraries(libdius PUBLIC liburing)
    add_dependencies(libdius liburing_project)
endif()