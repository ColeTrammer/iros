#include <dius/system/process.h>
#include <dius/test/prelude.h>

namespace process {
static void arg_passing() {
    auto args = di::Array { FIXTURE_PATH "test_dius_process_arg_passing_fixture"_tsv.to_owned(), "hello"_tsv.to_owned(),
                            "world"_tsv.to_owned() } |
                di::to<di::Vector>();
    auto process = dius::system::Process(di::move(args));

    auto result = di::move(process).spawn_and_wait();
    ASSERT(result);
    ASSERT(result->exited());
    ASSERT_EQ(result->exit_code(), 0);
}

TEST(process, arg_passing)
}
