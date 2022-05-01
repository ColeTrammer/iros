#pragma once

#include <liim/function.h>
#include <liim/string.h>

namespace Test {
class TestCase {
public:
    TestCase(String suite_name, String case_name, Function<void()> tester, bool skipped)
        : m_suite_name(move(suite_name)), m_case_name(move(case_name)), m_tester(move(tester)), m_skipped(skipped) {}

    const String& suite_name() const { return m_suite_name; }
    const String& case_name() const { return m_case_name; }
    bool skipped() const { return m_skipped; }

    void execute() {
        if (!skipped()) {
            m_tester();
        }
    }

private:
    String m_suite_name;
    String m_case_name;
    Function<void()> m_tester;
    bool m_skipped { false };
};
}
