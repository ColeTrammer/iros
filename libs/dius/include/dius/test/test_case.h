#pragma once

#include <di/prelude.h>

namespace dius::test {
using TestCaseFunction = void (*)();

class TestCase {
public:
    constexpr explicit TestCase(di::TransparentStringView suite_name, di::TransparentStringView case_name,
                                TestCaseFunction function)
        : m_suite_name(di::move(suite_name)), m_case_name(di::move(case_name)), m_function(function) {}

    constexpr di::TransparentStringView suite_name() const { return m_suite_name; }
    constexpr di::TransparentStringView case_name() const { return m_case_name; }

    constexpr void execute() const { m_function(); }

private:
    constexpr friend bool operator==(TestCase const& a, TestCase const& b) {
        return di::make_tuple(a.suite_name(), a.case_name()) == di::make_tuple(b.suite_name(), b.case_name());
    }

    constexpr friend di::strong_ordering operator<=>(TestCase const& a, TestCase const& b) {
        return di::make_tuple(a.suite_name(), a.case_name()) <=> di::make_tuple(b.suite_name(), b.case_name());
    }

    di::TransparentStringView m_suite_name;
    di::TransparentStringView m_case_name;
    TestCaseFunction m_function;
};
}
