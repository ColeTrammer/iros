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

function(add_os_tests name)
    set(test_name "test_${name}")
    set(test_executable "${CMAKE_CURRENT_BINARY_DIR}/${test_name}")
    set(test_include_file "${CMAKE_CURRENT_BINARY_DIR}/${test_name}.cmake")
    set(test_generate_script "${CMAKE_SOURCE_DIR}/scripts/generate-cmake-tests.sh")

    set(SOURCES ${TEST_FILES} ${CMAKE_SOURCE_DIR}/libs/libtest/include/test/main.cpp)
    add_os_executable("${test_name}" bin)
    target_link_libraries("${test_name}" libtest)

    if (${NATIVE_BUILD})
        target_compile_definitions("${test_name}" PRIVATE "BINARY_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\"")
        add_custom_command(
            TARGET "test_${name}"
            POST_BUILD
            COMMAND "${test_generate_script}" "${test_executable}" "${test_name}" "${test_include_file}"
            DEPENDS "${test_generate_script}"
        )
        set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "${test_include_file}")
    else()
        target_compile_definitions("${test_name}" PRIVATE "BINARY_DIR=\"/bin\"")
    endif()
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
        add_dependencies(${name} bootstrap-core-libs)
        target_compile_options(${name} PRIVATE "-rdynamic")
        target_link_options(${name} PRIVATE "-rdynamic" "-Wl,-rpath-link" "${ROOT}/base/usr/lib") # Make the linker stop complaining about libgcc_s.so
    endif()
    install(TARGETS ${name} RUNTIME DESTINATION ${dest_dir})
endfunction()

function(add_os_static_executable name dest_dir)
    add_executable(${name} ${SOURCES})
    if (NOT ${NATIVE_BUILD})
        add_dependencies(${name} bootstrap-core-libs)
        target_link_libraries(${name} PRIVATE libc_static)
    endif()
    target_link_options(${name} PRIVATE "-static")
    install(TARGETS ${name} RUNTIME DESTINATION ${dest_dir})
endfunction()
