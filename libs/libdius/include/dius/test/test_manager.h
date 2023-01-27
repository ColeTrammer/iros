#pragma once

#include <dius/test/test_case.h>

namespace dius::test {
class TestManager {
public:
    static TestManager& the();

    void register_test_case(TestCase);

    struct Args {
        bool list_simple { false };
        di::Optional<di::TransparentStringView> suite_name;
        di::Optional<di::TransparentStringView> case_name;

        constexpr static auto get_cli_parser() {
            return di::cli_parser<Args>("dius_test"_sv, "Dius Test Runner"_sv)
                .flag<&Args::list_simple>('L', "list-simple"_tsv, "Output a simple machine readable list of test cases"_sv)
                .flag<&Args::suite_name>('s', "suite"_tsv, "Specifc test suite to run"_sv)
                .flag<&Args::case_name>('t', "test-case"_tsv, "Specific case to run in the format ([suite:]case)"_sv);
        }
    };

    di::Result<void> run_tests(Args& args);

private:
    TestManager() {}

    di::Vector<TestCase> m_test_cases;
    int m_fail_count { 0 };
};
}