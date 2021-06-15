set(SOURCES
    builtin.cpp
    command.cpp
    input.cpp
    job.cpp
    main.cpp
    sh_state.cpp
)

add_os_executable(sh bin)
target_link_libraries(sh libsh libext libtinput)

if (${NATIVE_BUILD})
    target_compile_definitions(sh PRIVATE USERLAND_NATIVE)
    target_link_libraries(sh libwordexp)
endif()
