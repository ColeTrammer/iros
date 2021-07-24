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
    m_test_cases.add(make_shared<TestCase>(move(suite_name), move(case_name), move(tester)));
}

void TestManager::test_did_fail(const char* file, int line, const char* cond) {
    printf("\033[31;1mFAIL\033[0m: \033[1m%s\033[0m: %s: %s:%d: %s\n", m_current_test_case->suite_name().string(),
           m_current_test_case->case_name().string(), file, line, cond);

    m_fail_count++;
}

static void print_usage(const char* name) {
    fprintf(stderr, "Usage: %s [-L] [-s SUITE] [-t TESTCASE]\n", name);
}

int TestManager::do_main(int argc, char** argv) {
    auto list_simple = false;
    auto suite_name = Maybe<String> {};
    auto case_name = Maybe<String> {};

    int opt;
    while ((opt = getopt(argc, argv, ":Ls:t:")) != -1) {
        switch (opt) {
            case 'L':
                list_simple = true;
                break;
            case 's':
                suite_name = { optarg };
                break;
            case 't':
                case_name = { optarg };
                break;
            case ':':
            case '?':
                print_usage(*argv);
                return 2;
        }
    }

    if (optind != argc) {
        print_usage(*argv);
        return 2;
    }

    auto test_cases = m_test_cases;
    test_cases.remove_if([&](auto& test_case) {
        if (suite_name && *suite_name != test_case->suite_name()) {
            return true;
        }
        if (case_name) {
            auto colon_index = case_name->index_of(':');
            if (colon_index) {
                return test_case->suite_name() != case_name->first(*colon_index) ||
                       test_case->case_name() != case_name->substring(*colon_index + 1);
            } else {
                return test_case->case_name() != *case_name;
            }
        }
        return false;
    });

    if (test_cases.empty() && (suite_name || case_name)) {
        fprintf(stderr, "%s: No test cases match filter: [suite=%s] [case=%s]\n", *argv, suite_name ? suite_name->string() : "",
                case_name ? case_name->string() : "");
        return 1;
    }

    if (list_simple) {
        for (auto& test_case : test_cases) {
            printf("%s:%s\n", test_case->suite_name().string(), test_case->case_name().string());
        }
        return 0;
    }

    for (auto& test_case : test_cases) {
        auto start_fail_count = m_fail_count;
        m_current_test_case = test_case.get();
        test_case->execute();

        if (m_fail_count == start_fail_count) {
            printf("\033[1;32mPASS\033[0m: \033[1m%s\033[0m: %s\n", test_case->suite_name().string(), test_case->case_name().string());
        }
    }

    printf("\n\033[1m%d\033[0m / \033[1m%d\033[0m Tests Passed: %s\n", test_cases.size() - m_fail_count, test_cases.size(),
           m_fail_count ? "\033[31;1mTests Failed\033[0m" : "\033[32;1mTests Passed\033[0m");

    return m_fail_count ? 1 : 0;
}
}
