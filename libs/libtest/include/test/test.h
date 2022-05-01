#pragma once

#include <liim/format.h>
#include <test/test_case.h>
#include <test/test_manager.h>

#define TEST(suite_name, case_name)                                                                           \
    static void suite_name##_##case_name();                                                                   \
    static __attribute__((constructor)) void __register_##suite_name##_##case_name() {                        \
        Test::TestManager::the().register_test_case("" #suite_name, "" #case_name, suite_name##_##case_name); \
    }                                                                                                         \
    static void suite_name##_##case_name()

#define TEST_SKIP(suite_name, case_name)                                                                              \
    static void suite_name##_##case_name();                                                                           \
    static __attribute__((constructor)) void __register_##suite_name##_##case_name() {                                \
        Test::TestManager::the().register_skipped_test_case("" #suite_name, "" #case_name, suite_name##_##case_name); \
    }                                                                                                                 \
    static void suite_name##_##case_name()

#define EXPECT(...)                                                                                                                        \
    do {                                                                                                                                   \
        if (!(__VA_ARGS__)) {                                                                                                              \
            error_log("\033[31;1mFAIL\033[0m: \033[1m{}\033[0m: {}: {}:{}: {}", Test::TestManager::the().current_test_case().suite_name(), \
                      Test::TestManager::the().current_test_case().case_name(), __FILE__, String::format("%d", __LINE__),                  \
                      "" #__VA_ARGS__);                                                                                                    \
            Test::TestManager::the().test_did_fail();                                                                                      \
            return;                                                                                                                        \
        }                                                                                                                                  \
    } while (0)

#define EXPECT_EQ(a, b)                                                                                                                    \
    do {                                                                                                                                   \
        auto a_result = (a);                                                                                                               \
        auto b_result = (b);                                                                                                               \
        if (a_result != b_result) {                                                                                                        \
            error_log("\033[31;1mFAIL\033[0m: \033[1m{}\033[0m: {}: {}:{}: {} != {} => '{}' != '{}'",                                      \
                      Test::TestManager::the().current_test_case().suite_name(), Test::TestManager::the().current_test_case().case_name(), \
                      __FILE__, String::format("%d", __LINE__), "" #a, "" #b, a_result, b_result);                                         \
            Test::TestManager::the().test_did_fail();                                                                                      \
            return;                                                                                                                        \
        }                                                                                                                                  \
    } while (0)

#define EXPECT_NOT_EQ(a, b)                                                                                                                \
    do {                                                                                                                                   \
        auto a_result = (a);                                                                                                               \
        auto b_result = (b);                                                                                                               \
        if (a_result == b_result) {                                                                                                        \
            error_log("\033[31;1mFAIL\033[0m: \033[1m{}\033[0m: {}: {}:{}: {} != {} => '{}' == '{}'",                                      \
                      Test::TestManager::the().current_test_case().suite_name(), Test::TestManager::the().current_test_case().case_name(), \
                      __FILE__, String::format("%d", __LINE__), "" #a, "" #b, a_result, b_result);                                         \
            Test::TestManager::the().test_did_fail();                                                                                      \
            return;                                                                                                                        \
        }                                                                                                                                  \
    } while (0)
