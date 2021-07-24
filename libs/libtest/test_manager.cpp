#include <stdio.h>
#include <test/test_case.h>
#include <test/test_manager.h>
#include <unistd.h>

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
    printf("\033[31;1mFAIL\033[0m: \033[1m%s\033[0m: %s: %s:%d: %s\n", m_current_test_case->suite_name().string(),
           m_current_test_case->case_name().string(), file, line, cond);

    m_fail_count++;
}

static void print_usage(const char* name) {
    fprintf(stderr, "Usage: %s [-L]\n", name);
}

int TestManager::do_main(int argc, char** argv) {
    bool list_simple = false;

    int opt;
    while ((opt = getopt(argc, argv, ":L")) != -1) {
        switch (opt) {
            case 'L':
                list_simple = true;
                break;
            case ':':
            case '?':
                print_usage(*argv);
                return 2;
        }
    }

    if (list_simple) {
        for (auto& test_case : m_test_cases) {
            printf("%s:%s\n", test_case.suite_name().string(), test_case.case_name().string());
        }
        return 0;
    }

    for (auto& test_case : m_test_cases) {
        auto start_fail_count = m_fail_count;
        m_current_test_case = &test_case;
        test_case.execute();

        if (m_fail_count == start_fail_count) {
            printf("\033[1;32mPASS\033[0m: \033[1m%s\033[0m: %s\n", test_case.suite_name().string(), test_case.case_name().string());
        }
    }

    printf("\n\033[1m%d\033[0m / \033[1m%d\033[0m Tests Passed: %s\n", m_test_cases.size() - m_fail_count, m_test_cases.size(),
           m_fail_count ? "\033[31;1mTests Failed\033[0m" : "\033[32;1mTests Passed\033[0m");

    return m_fail_count ? 1 : 0;
}
}
