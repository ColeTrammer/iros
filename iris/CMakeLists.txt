cmake_minimum_required(VERSION 3.25.2)

project(Iris CXX)

add_library(iris_headers INTERFACE)
add_library(Iris::iris_headers ALIAS iris_headers)

target_include_directories(iris_headers
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

find_package(Di REQUIRED)

target_link_libraries(iris_headers
  INTERFACE
    Di::di
)

install(
    TARGETS iris_headers
    EXPORT IrisHeadersConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(
    EXPORT IrisHeadersConfig
    FILE IrisHeadersConfig.cmake
    NAMESPACE Iris::
    DESTINATION lib/cmake/IrisHeaders
)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "include"
    FILES_MATCHING PATTERN "*.h"
)


if(IROS_BuildIris)
  include(ExternalProject)

  ExternalProject_Add(limine
      GIT_REPOSITORY https://github.com/limine-bootloader/limine.git
      GIT_TAG v4.x-branch-binary
      GIT_SHALLOW TRUE
      BUILD_IN_SOURCE TRUE
      CONFIGURE_COMMAND ""
      BUILD_COMMAND "make"
      UPDATE_COMMAND ""
      INSTALL_COMMAND ""
  )
  set(LIMINE_DIR "${CMAKE_CURRENT_BINARY_DIR}/limine-prefix/src/limine" PARENT_SCOPE)
  set(LIMINE_DIR "${CMAKE_CURRENT_BINARY_DIR}/limine-prefix/src/limine")

  add_executable(iris
      boot/init.cpp
      core/global_state.cpp
      core/task_namespace.cpp
      core/task.cpp
      core/scheduler.cpp
      fs/initrd.cpp
      mm/address_space.cpp
      mm/page_frame_allocator.cpp
      mm/heap.cpp
  )

  if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
      include(arch/x86/amd64/CMakeLists.txt)
  else()
      message(FATAL_ERROR "The Iris kernel does not support the \"" ${CMAKE_HOST_SYSTEM_PROCESSOR} "\" architecture.")
  endif()

  target_compile_options(iris
    PRIVATE
      -ffreestanding
      -nostdlib
      -fno-pic
      -fno-pie
      -static
      -nostartfiles
      -fno-stack-protector
  )
  target_link_options(iris
    PRIVATE
      -ffreestanding
      -nostdlib
      -fno-pic
      -fno-pie
      -static
      -nostartfiles
  )
  target_link_libraries(iris
    PRIVATE
      di
      iris_headers
  )
  target_include_directories(iris
    PRIVATE 
      "${LIMINE_DIR}"
  )
  target_compile_definitions(iris
    PRIVATE
      "DI_CUSTOM_ASSERT_HANDLER"
      "DI_NO_ASSERT_ALLOCATION"
      "DI_CUSTOM_PLATFORM=<iris/core/platform.h>"
  )
  add_dependencies(iris limine)

  install(TARGETS iris 
      RUNTIME DESTINATION boot
  )

  add_executable(test_userspace 
      test_userspace.cpp
  )
  target_compile_options(test_userspace
    PRIVATE
      -ffreestanding
      -nostdlib
      -nostartfiles
      -fno-pic
      -fno-pie
      -static
  )
  target_link_options(test_userspace
    PRIVATE
      -ffreestanding
      -nostdlib
      -nostartfiles
      -fno-pic
      -fno-pie
      -static
  )
  install(TARGETS test_userspace
      RUNTIME DESTINATION boot
  )

  add_executable(test_create_task 
      test_create_task.cpp
  )
  target_compile_options(test_create_task
    PRIVATE
      -ffreestanding
      -nostdlib
      -nostartfiles
      -fno-pic
      -fno-pie
      -static
  )
  target_link_options(test_create_task
    PRIVATE
      -ffreestanding
      -nostdlib
      -nostartfiles
      -fno-pic
      -fno-pie
      -static
  )
  install(TARGETS test_create_task
      RUNTIME DESTINATION boot
  )

  add_executable(test_read 
    test_read.cpp
  )
  target_compile_options(test_read
    PRIVATE
    -ffreestanding
    -nostdlib
    -nostartfiles
    -fno-pic
    -fno-pie
    -static
  )
  target_link_options(test_read
    PRIVATE
    -ffreestanding
    -nostdlib
    -nostartfiles
    -fno-pic
    -fno-pie
    -static
  )
  install(TARGETS test_read
    RUNTIME DESTINATION boot
  )

  if (IROS_BuildTests)
    add_subdirectory(tests/integration)
  endif()
endif()
