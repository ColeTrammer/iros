#include <dius/test/prelude.h>

#ifndef DIUS_USE_RUNTIME
#include <signal.h>
#include <stdlib.h>
#endif

namespace dius::test {
TestManager& TestManager::the() {
    static TestManager s_the;
    return s_the;
}

void TestManager::register_test_case(TestCase test_case) {
    m_test_cases.push_back(di::move(test_case));
}

void TestManager::execute_remaining_tests() {
    while (m_current_test_index < m_test_cases.size()) {
        run_current_test();
        m_current_test_index++;
    }
    final_report();
}

void TestManager::run_current_test() {
    auto& test_case = m_test_cases[m_current_test_index];
    test_case.execute();
    print_success_message();
}

void TestManager::handle_assertion_failure() {
    print_failure_message();
    ++m_current_test_index;
    final_report();
}

void TestManager::print_failure_message() {
    auto& test_case = m_test_cases[m_current_test_index];
    dius::eprintln("\033[31;1mFAIL\033[0m: \033[1m{}\033[0m: {}"_sv, test_case.suite_name(), test_case.case_name());
    ++m_fail_count;
}

void TestManager::print_success_message() {
    auto& test_case = m_test_cases[m_current_test_index];
    dius::eprintln("\033[1;32mPASS\033[0m: \033[1m{}\033[0m: {}"_sv, test_case.suite_name(), test_case.case_name());
    ++m_success_count;
}

void TestManager::final_report() {
    auto tests_skipped = m_test_cases.size() - m_current_test_index;
    auto skipped_string = di::String {};
    if (tests_skipped) {
        skipped_string =
            *di::present(" (\033[1m{}\033[0m Failed \033[1m{}\033[0m Skipped)"_sv, m_fail_count, tests_skipped);
    }

    dius::println("\n\033[1m{}\033[0m / \033[1m{}\033[0m Tests Passed{}: {}"_sv, m_success_count, m_test_cases.size(),
                  skipped_string,
                  m_fail_count ? "\033[31;1mTests Failed\033[0m"_sv : "\033[32;1mTests Passed\033[0m"_sv);

    int result = m_fail_count > 0;
#ifdef DIUS_PLATFORM_IROS
    (void) system::system_call<int>(system::Number::shutdown, result);
#endif
    system::exit_process(result);
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
        dius::eprintln("No test cases match filter: [suite={}] [case={}]"_sv, suite_name, case_name);
        return di::Unexpected(dius::PosixError::InvalidArgument);
    }

    if (list_simple) {
        for (auto& test_case : m_test_cases) {
            dius::println("{}:{}"_sv, test_case.suite_name(), test_case.case_name());
        }
        return {};
    }

#ifndef DIUS_USE_RUNTIME
    auto handler = [](int) {
        auto& manager = TestManager::the();
        manager.handle_assertion_failure();
    };

    signal(SIGSEGV, handler);
    signal(SIGFPE, handler);
    signal(SIGABRT, handler);
#endif

    execute_remaining_tests();

    return {};
}
}
