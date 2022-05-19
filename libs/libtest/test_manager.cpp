#include <cli/cli.h>
#include <liim/format.h>
#include <pthread.h>
#include <sys/wait.h>
#include <test/test.h>
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
    m_test_cases.add(make_shared<TestCase>(move(suite_name), move(case_name), move(tester), false));
}

void TestManager::register_skipped_test_case(String suite_name, String case_name, Function<void()> tester) {
    m_test_cases.add(make_shared<TestCase>(move(suite_name), move(case_name), move(tester), true));
}

int TestManager::spawn_process_and_block(Function<void()> before_exec, String path) {
    pid_t child = fork();
    assert(child >= 0);

    if (child == 0) {
        before_exec();

        char* args[] = { path.string(), nullptr };
        assert(execvp(path.string(), args) == 0);
    }

    int status;
    assert(waitpid(child, &status, 0) == child);

    if (WIFSIGNALED(status)) {
        return WTERMSIG(status);
    }
    return WEXITSTATUS(status);
}

void* TestManager::spawn_thread_and_block(Function<void(pthread_t)> after_spawn_before_join, Function<void()> thread_body) {
    pthread_t id;
    assert(pthread_create(
               &id, nullptr,
               [](void* arg) -> void* {
                   (*(Function<void()>*) arg)();
                   return nullptr;
               },
               &thread_body) == 0);

    after_spawn_before_join.safe_call(id);

    void* result;
    assert(pthread_join(id, &result) == 0);
    return result;
}

void TestManager::spawn_threads_and_block(int thread_count, Function<void()> thread_body) {
    Vector<pthread_t> threads;
    threads.resize(thread_count);
    for (int i = 0; i < thread_count; i++) {
        assert(pthread_create(
                   &threads[i], nullptr,
                   [](void* arg) -> void* {
                       (*(Function<void()>*) arg)();
                       return nullptr;
                   },
                   &thread_body) == 0);
    }
    for (int i = 0; i < thread_count; i++) {
        assert(pthread_join(threads[i], nullptr) == 0);
    }
}

void TestManager::test_did_fail() {
    m_fail_count++;
}

Result<Monostate, Ext::StringError> TestManager::do_main(Arguments arguments) {
    auto [list_simple, suite_name, case_name] = arguments;

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
        return Err(Ext::StringError(format("No test cases match filter: [suite={}] [case={}]", suite_name, case_name)));
    }

    if (list_simple) {
        for (auto& test_case : test_cases) {
            out_log("{}:{}", test_case->suite_name(), test_case->case_name());
        }
        return Ok(Monostate {});
    }

    for (auto& test_case : test_cases) {
        auto start_fail_count = m_fail_count;
        m_current_test_case = test_case.get();
        test_case->execute();

        if (m_fail_count == start_fail_count) {
            if (test_case->skipped()) {
                error_log("\033[1;33mSKIP\033[0m: \033[1m{}\033[0m: {}", test_case->suite_name(), test_case->case_name());
            } else {
                error_log("\033[1;32mPASS\033[0m: \033[1m{}\033[0m: {}", test_case->suite_name(), test_case->case_name());
            }
        }
    }

    printf("\n\033[1m%d\033[0m / \033[1m%d\033[0m Tests Passed: %s\n", test_cases.size() - m_fail_count, test_cases.size(),
           m_fail_count ? "\033[31;1mTests Failed\033[0m" : "\033[32;1mTests Passed\033[0m");

    exit(m_fail_count ? 1 : 0);
}
}
