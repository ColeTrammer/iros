#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/hw/power.h>

namespace iris::test {
TestManager& TestManager::the() {
    return global_state_in_boot().unit_test_manager;
}

void TestManager::register_test_case(TestCase test_case) {
    ASSERT(m_test_cases.push_back(di::move(test_case)));
}

extern "C" {
typedef void (*init_function_t)(void);

extern init_function_t __iris_unit_test_init_array_start[];
extern init_function_t __iris_unit_test_init_array_end[];
}

static void call_unit_test_case_init_functions() {
    iptr init_size = __iris_unit_test_init_array_end - __iris_unit_test_init_array_start;
    for (iptr i = 0; i < init_size; i++) {
        (*__iris_unit_test_init_array_start[i])();
    }
}

void TestManager::run_tests() {
    call_unit_test_case_init_functions();

    for (auto& test_case : m_test_cases) {
        println("Running test {}:{}"_sv, test_case.suite_name(), test_case.case_name());
        test_case.execute();
    }

    println("All tests passed."_sv);
    hard_shutdown(ShutdownStatus::Intended);
}
}
