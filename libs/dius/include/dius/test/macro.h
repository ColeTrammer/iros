#pragma once

#include <dius/test/test_manager.h>

#define DIUS_TEST(suite_name, case_name)                                                                  \
    static void suite_name##_##case_name();                                                               \
    [[gnu::constructor]] static void register_##suite_name##_##case_name() {                              \
        dius::test::TestManager::the().register_test_case(                                                \
            dius::test::TestCase("" #suite_name ""_tsv, "" #case_name ""_tsv, suite_name##_##case_name)); \
    }                                                                                                     \
    static void suite_name##_##case_name() {                                                              \
        case_name();                                                                                      \
    }

#define DIUS_TESTC(suite_name, case_name)                                                                 \
    static void suite_name##_##case_name();                                                               \
    [[gnu::constructor]] static void register_##suite_name##_##case_name() {                              \
        dius::test::TestManager::the().register_test_case(                                                \
            dius::test::TestCase("" #suite_name ""_tsv, "" #case_name ""_tsv, suite_name##_##case_name)); \
    }                                                                                                     \
    static void suite_name##_##case_name() {                                                              \
        [[maybe_unused]] constexpr int exec = [] {                                                        \
            case_name();                                                                                  \
            return 0;                                                                                     \
        }();                                                                                              \
        case_name();                                                                                      \
    }

#ifdef __clang__
#define DIUS_TESTC_CLANG DIUS_TESTC
#define DIUS_TESTC_GCC   DIUS_TEST
#else
#define DIUS_TESTC_CLANG DIUS_TEST
#define DIUS_TESTC_GCC   DIUS_TESTC
#endif
