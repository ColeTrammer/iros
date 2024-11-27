#pragma once

#include <di/container/string/prelude.h>
#include <di/container/vector/prelude.h>
#include <di/util/prelude.h>

namespace iris::test {
using TestCaseFunction = void (*)();

class TestCase {
public:
    constexpr explicit TestCase(di::TransparentStringView suite_name, di::TransparentStringView case_name,
                                TestCaseFunction function)
        : m_suite_name(di::move(suite_name)), m_case_name(di::move(case_name)), m_function(function) {}

    constexpr auto suite_name() const -> di::TransparentStringView { return m_suite_name; }
    constexpr auto case_name() const -> di::TransparentStringView { return m_case_name; }

    constexpr void execute() const { m_function(); }

private:
    constexpr friend auto operator==(TestCase const& a, TestCase const& b) -> bool {
        return di::make_tuple(a.suite_name(), a.case_name()) == di::make_tuple(b.suite_name(), b.case_name());
    }

    constexpr friend auto operator<=>(TestCase const& a, TestCase const& b) -> di::strong_ordering {
        return di::make_tuple(a.suite_name(), a.case_name()) <=> di::make_tuple(b.suite_name(), b.case_name());
    }

    di::TransparentStringView m_suite_name;
    di::TransparentStringView m_case_name;
    TestCaseFunction m_function;
};

class TestManager {
public:
    static auto the() -> TestManager&;

    void register_test_case(TestCase);
    void run_tests();

private:
    di::Vector<TestCase> m_test_cases;
};
}

#define IRIS_TEST(suite_name, case_name)                                                                          \
    static void suite_name##_##case_name();                                                                       \
    static void register_##suite_name##_##case_name() {                                                           \
        iris::test::TestManager::the().register_test_case(                                                        \
            iris::test::TestCase("" #suite_name ""_tsv, "" #case_name ""_tsv, suite_name##_##case_name));         \
    }                                                                                                             \
    [[maybe_unused]] [[gnu::section(".unit_test_init_array")]] void (*__unit_test_##suite_name##_##case_name)() = \
        register_##suite_name##_##case_name;                                                                      \
    static void suite_name##_##case_name() {                                                                      \
        case_name();                                                                                              \
    }

#define TEST IRIS_TEST
