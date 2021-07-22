set(SOURCES
    builtins/alias.cpp
    builtins/bg.cpp
    builtins/break.cpp
    builtins/cd.cpp
    builtins/colon.cpp
    builtins/continue.cpp
    builtins/echo.cpp
    builtins/eval.cpp
    builtins/exec.cpp
    builtins/exit.cpp
    builtins/export.cpp
    builtins/false.cpp
    builtins/fg.cpp
    builtins/history.cpp
    builtins/jobs.cpp
    builtins/kill.cpp
    builtins/pushd.cpp
    builtins/read.cpp
    builtins/return.cpp
    builtins/set.cpp
    builtins/shift.cpp
    builtins/source.cpp
    builtins/test.cpp
    builtins/time.cpp
    builtins/true.cpp
    builtins/type.cpp
    builtins/umask.cpp
    builtins/unalias.cpp
    builtins/unset.cpp
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
