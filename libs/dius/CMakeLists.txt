cmake_minimum_required(VERSION 3.25.2)

project(Dius CXX)

add_library(dius
    di_assert_impl.cpp
    error.cpp
    sync_file.cpp
    test/test_manager.cpp
)
add_library(Dius::dius ALIAS dius)

target_include_directories(dius
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

find_package(Di REQUIRED)

target_link_libraries(dius
  PUBLIC
    Di::di
)
target_compile_definitions(dius PUBLIC DI_CUSTOM_ASSERT_HANDLER DI_CUSTOM_PLATFORM=<dius/platform.h>)

if(LINUX)
    target_sources(dius
      PRIVATE
        linux/filesystem/directory_iterator.cpp
        linux/filesystem/query.cpp
        linux/system/process.cpp
        linux/io_uring.cpp
        linux/sync_file.cpp
        memory_region.cpp
)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Iros")
    if(CMAKE_CROSSCOMPILING)
        find_package(Iris REQUIRED)    

        target_link_libraries(dius
          PUBLIC
            Iris::iris_headers
        )
    endif()

    target_sources(dius
      PRIVATE
        iros/sync_file.cpp
        iros/system/process.cpp
    )
endif()

install(
    TARGETS dius
    EXPORT DiusConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(
    EXPORT DiusConfig
    FILE DiusConfig.cmake
    NAMESPACE Dius::
    DESTINATION lib/cmake/Dius
)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "include"
    FILES_MATCHING PATTERN "*.h"
)

if (IROS_UseDiusRuntime)
    target_compile_definitions(dius
      PUBLIC
        DIUS_USE_RUNTIME
    )

    add_library(dius_runtime STATIC
        runtime/compilerabi.cpp
        runtime/cxxabi.cpp
        runtime/cxxheap.cpp
        runtime/entry.cpp
        runtime/gccints.cpp
        runtime/stack_smash_protector.cpp
    )
    add_library(Dius::dius_runtime ALIAS dius_runtime)
    target_link_libraries(dius_runtime
      PUBLIC
        dius
    )
    target_compile_options(dius_runtime
      PUBLIC
        "-nostdlib"
        "-static"
      PRIVATE
        "-fno-stack-protector"
        "-ffreestanding"
    )
    target_link_options(dius_runtime
      PUBLIC
        "-nostdlib"
        "-static"
      PRIVATE
        "-fno-stack-protector"
        "-ffreestanding"
    )

    target_compile_options(dius
      PRIVATE
        "-fno-stack-protector"
    )
    target_link_options(dius
      PRIVATE
        "-fno-stack-protector"
    )

    install(
        TARGETS dius_runtime
        EXPORT DiusRuntimeConfig
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
    )
    install(
        EXPORT DiusRuntimeConfig
        FILE DiusRuntimeConfig.cmake
        NAMESPACE Dius::
        DESTINATION lib/cmake/DiusRuntime
    )
endif()

add_library(dius_test_main STATIC
    test/test_main.cpp
)
add_library(Dius::dius_test_main ALIAS dius_test_main)
target_link_libraries(dius_test_main
  PRIVATE
    dius
)

install(
    TARGETS dius_test_main
    EXPORT DiusTestMainConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(
    EXPORT DiusTestMainConfig
    FILE DiusTestMainConfig.cmake
    NAMESPACE Dius::
    DESTINATION lib/cmake/DiusTestMain
)

if (IROS_UseDiusRuntime)
  target_link_libraries(dius_test_main
    PUBLIC
      dius_runtime
  )
endif()
