#pragma once

#include <di/cli/prelude.h>
#include <dius/test/test_case.h>

namespace dius::test {
class TestManager {
public:
    static auto the() -> TestManager&;

    void register_test_case(TestCase);

    struct Args {
        bool list_simple { false };
        di::Optional<di::TransparentStringView> suite_name;
        di::Optional<di::TransparentStringView> case_name;
        bool help { false };

        constexpr static auto get_cli_parser() {
            return di::cli_parser<Args>("dius_test"_sv, "Dius Test Runner"_sv)
                .help()
                .option<&Args::list_simple>('L', "list-simple"_tsv,
                                            "Output a simple machine readable list of test cases"_sv)
                .option<&Args::suite_name>('s', "suite"_tsv, "Specifc test suite to run"_sv)
                .option<&Args::case_name>('t', "test-case"_tsv, "Specific case to run in the format ([suite:]case)"_sv);
        }
    };

    auto run_tests(Args& args) -> di::Result<void>;

    auto is_test_application() const -> bool { return !m_test_cases.empty(); }
    void handle_assertion_failure();

private:
    TestManager() {}

    void print_failure_message();
    void print_success_message();

    void run_current_test();
    void execute_remaining_tests();
    void final_report();

    di::Vector<TestCase> m_test_cases;
    usize m_current_test_index { 0 };
    usize m_fail_count { 0 };
    usize m_success_count { 0 };
};
}
