#pragma once

#include "di/container/string/prelude.h"
#include "di/util/prelude.h"

namespace dius::test {
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
}
