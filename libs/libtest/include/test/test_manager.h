#pragma once

#include <liim/forward.h>
#include <liim/vector.h>
#include <test/forward.h>

namespace Test {
class TestManager {
public:
    static TestManager& the();

    ~TestManager();

    void register_skipped_test_case(String suite_name, String case_name, Function<void()> tester);
    void register_test_case(String suite_name, String case_name, Function<void()> tester);
    void test_did_fail();

    int spawn_process_and_block(Function<void()> before_exec, String path);
    void* spawn_thread_and_block(Function<void(pthread_t)> after_spawn_before_join, Function<void()> thread_body);
    void spawn_threads_and_block(int thread_count, Function<void()>);

    int do_main(int argc, char** argv);

    TestCase& current_test_case() { return *m_current_test_case; }

private:
    Vector<SharedPtr<TestCase>> m_test_cases;
    TestCase* m_current_test_case { nullptr };
    int m_fail_count { 0 };
};
}
