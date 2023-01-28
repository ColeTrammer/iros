set(TEST_FILES
    test_alarm.cpp
    test_spawn.cpp
    test_timer.cpp
    test_waitpid.cpp
)

add_os_tests(kernel ${TEST_FILES})
target_link_libraries(test_kernel PRIVATE ${REALTIME_LIB})

set(SOURCES
    test_alarm.cpp
)
add_os_executable(test_alarm_exec_helper bin)
target_compile_definitions(test_alarm_exec_helper PRIVATE "ALARM_EXEC_HELPER=1")
target_link_libraries(test_alarm_exec_helper PRIVATE libtest ${REALTIME_LIB})

set(SOURCES
    test_spawn.cpp
)
add_os_executable(test_spawn_basic_helper bin)
target_compile_definitions(test_spawn_basic_helper PRIVATE "SPAWN_BASIC_HELPER=1")
target_link_libraries(test_spawn_basic_helper PRIVATE libtest ${REALTIME_LIB})

set(SOURCES
    test_waitpid.cpp
)
add_os_executable(test_waitpid_exec_helper bin)
target_compile_definitions(test_waitpid_exec_helper PRIVATE "WAITPID_EXEC_HELPER=1")
target_link_libraries(test_waitpid_exec_helper PRIVATE libtest ${REALTIME_LIB})
