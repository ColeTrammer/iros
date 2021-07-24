set(TEST_FILES
    test_alarm.cpp
)

add_os_tests(kernel ${TEST_FILES})

set(SOURCES
    test_alarm.cpp
)
add_os_executable(test_alarm_exec_helper bin)
target_compile_definitions(test_alarm_exec_helper PRIVATE "ALARM_EXEC_HELPER=1")
target_link_libraries(test_alarm_exec_helper PRIVATE libtest)
