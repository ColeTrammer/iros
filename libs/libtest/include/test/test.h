#pragma once

#include <test/test_case.h>
#include <test/test_manager.h>

#define TEST(suite_name, case_name)                                                                           \
    static void suite_name##_##case_name();                                                                   \
    static __attribute__((constructor)) void __register_##suite_name##_##case_name() {                        \
        Test::TestManager::the().register_test_case("" #suite_name, "" #case_name, suite_name##_##case_name); \
    }                                                                                                         \
    static void suite_name##_##case_name()

#define EXPECT(cond)                                                              \
    do {                                                                          \
        if (!(cond)) {                                                            \
            Test::TestManager::the().test_did_fail(__FILE__, __LINE__, "" #cond); \
            return;                                                               \
        }                                                                         \
    } while (0)
