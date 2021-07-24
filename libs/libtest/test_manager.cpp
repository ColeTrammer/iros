#include <stdio.h>
#include <test/test_case.h>
#include <test/test_manager.h>

namespace Test {
TestManager& TestManager::the() {
    static TestManager s_the;
    return s_the;
}

TestManager::~TestManager() {}

void TestManager::register_test_case(String suite_name, String case_name, Function<void()> tester) {
    m_test_cases.add({ move(suite_name), move(case_name), move(tester) });
}

void TestManager::test_did_fail(const char* file, int line, const char* cond) {
    fprintf(stderr, "FAIL: %s:%d: %s\n", file, line, cond);
}

int TestManager::do_main(int, char**) {
    for (auto& test_case : m_test_cases) {
        test_case.execute();
    }
    return 0;
}
}
