#include <dius/test/prelude.h>

#include <setjmp.h>
#include <signal.h>

namespace dius::test {
TestManager& TestManager::the() {
    static TestManager s_the;
    return s_the;
}

void TestManager::register_test_case(TestCase test_case) {
    m_test_cases.push_back(di::move(test_case));
}

static jmp_buf s_jmpbuf;

static void signal_handler(int) {
    longjmp(s_jmpbuf, 1);
}

di::Result<void> TestManager::run_tests(Args& args) {
    auto [list_simple, suite_name, case_name] = args;

    auto [first_to_remove, last_to_remove] = di::container::remove_if(m_test_cases, [&](auto&& test_case) {
        if (suite_name && *suite_name != test_case.suite_name()) {
            return true;
        }
        if (case_name) {
            auto [colon_it, colon_it_end] = case_name->find(':');
            if (colon_it != colon_it_end) {
                return test_case.suite_name() != case_name->substr(case_name->begin(), colon_it) ||
                       test_case.case_name() != case_name->substr(colon_it_end);
            } else {
                return test_case.case_name() != *case_name;
            }
        }
        return false;
    });
    m_test_cases.erase(first_to_remove, last_to_remove);

    if (m_test_cases.empty() && (suite_name || case_name)) {
        dius::error_log("No test cases match filter: [suite={}] [case={}]"_sv, suite_name, case_name);
        return di::Unexpected(dius::PosixError::InvalidArgument);
    }

    if (list_simple) {
        for (auto& test_case : m_test_cases) {
            dius::out_log("{}:{}"_sv, test_case.suite_name(), test_case.case_name());
        }
        return {};
    }

    signal(SIGSEGV, signal_handler);
    signal(SIGFPE, signal_handler);
    signal(SIGABRT, signal_handler);

    for (auto& test_case : m_test_cases) {
        auto start_fail_count = m_fail_count;

        if (setjmp(s_jmpbuf) == 1) {
            dius::error_log("\033[31;1mFAIL\033[0m: \033[1m{}\033[0m: {}"_sv, test_case.suite_name(), test_case.case_name());
            m_fail_count++;
            continue;
        }

        test_case.execute();

        if (m_fail_count == start_fail_count) {
            dius::error_log("\033[1;32mPASS\033[0m: \033[1m{}\033[0m: {}"_sv, test_case.suite_name(), test_case.case_name());
        }
    }

    dius::out_log("\n\033[1m{}\033[0m / \033[1m{}\033[0m Tests Passed: {}"_sv, m_test_cases.size() - m_fail_count, m_test_cases.size(),
                  m_fail_count ? "\033[31;1mTests Failed\033[0m"_sv : "\033[32;1mTests Passed\033[0m"_sv);

    if (m_fail_count) {
        return di::Unexpected(dius::PosixError::InvalidArgument);
    }
    return {};
}
}