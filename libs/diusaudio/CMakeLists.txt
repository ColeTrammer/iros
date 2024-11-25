cmake_minimum_required(VERSION 3.25.2)

project(Diusaudio CXX)

set(to_filter iros linux tests)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(REMOVE_ITEM to_filter linux)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Iros")
    list(REMOVE_ITEM to_filter iros)
endif()

function(filter input exclude_list output)
    foreach(exclude ${exclude_list})
        string(TOLOWER ${exclude} exclude)
        list(FILTER input EXCLUDE REGEX ${exclude})
    endforeach()

    set(${output}
        ${input}
        PARENT_SCOPE
    )
endfunction()

file(
    GLOB_RECURSE sources CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "*.cpp"
)
file(
    GLOB_RECURSE headers CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "include/*.h"
)

filter("${sources}" "${to_filter}" sources)
filter("${headers}" "${to_filter}" headers)

add_library(diusaudio ${sources})
add_library(Diusaudio::diusaudio ALIAS diusaudio)

target_sources(
    diusaudio
    INTERFACE FILE_SET
              HEADERS
              TYPE
              HEADERS
              BASE_DIRS
              include
              FILES
              ${headers}
)

target_include_directories(
    diusaudio PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
)

find_package(Dius REQUIRED)

target_link_libraries(diusaudio PUBLIC Dius::dius)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(IROS_UseDiusRuntime)
        message(STATUS "Dius runtime is being used. Audio suppport will be disabled.")
    else()
        find_path(PIPEWIRE_INCLUDE_DIRS pipewire-0.3/pipewire/pipewire.h)
        find_path(SPA_INCLUDE_DIRS spa-0.2/spa/param/props.h)
        find_library(PIPEWIRE_LIBRARIES NAMES pipewire-0.3)
        if(NOT PIPEWIRE_INCLUDE_DIRS
           OR NOT SPA_INCLUDE_DIRS
           OR NOT PIPEWIRE_LIBRARIES
        )
            message(STATUS "Pipewire not found. Audio support will be disabled.")
        else()
            message(STATUS "Pipewire found. Audio support will be enabled.")
            target_compile_definitions(diusaudio PRIVATE "DIUSAUDIO_HAVE_PIPEWIRE")
            target_include_directories(
                diusaudio SYSTEM PRIVATE ${PIPEWIRE_INCLUDE_DIRS}/pipewire-0.3 ${SPA_INCLUDE_DIRS}/spa-0.2
            )
            target_link_libraries(diusaudio PRIVATE ${PIPEWIRE_LIBRARIES})
        endif()
    endif()
endif()

install(
    TARGETS diusaudio
    EXPORT DiusaudioConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES
    DESTINATION include
    FILE_SET HEADERS
)
install(
    EXPORT DiusaudioConfig
    FILE DiusaudioConfig.cmake
    NAMESPACE Diusaudio::
    DESTINATION lib/cmake/Diusaudio
)

if(IROS_BuildTests)
    add_subdirectory(tests)
endif()
