#pragma once

#include <liim/forward.h>
#include <liim/vector.h>
#include <test/forward.h>

namespace Test {
class TestManager {
public:
    static TestManager& the();

    ~TestManager();

    void register_test_case(String suite_name, String case_name, Function<void()> tester);
    void test_did_fail(const char* file, int line, const char* cond);

    int do_main(int argc, char** argv);

private:
    Vector<SharedPtr<TestCase>> m_test_cases;
    TestCase* m_current_test_case { nullptr };
    int m_fail_count { 0 };
};
}
