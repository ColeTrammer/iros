#include <di/prelude.h>
#include <test/test.h>

constexpr void iteration() {
    auto in1 = "/abc/def/"_pv;
    auto in2 = "///abc///def///"_pv;
    ASSERT_EQ(in1, in2);

    auto r1 = in2 | di::to<di::Vector>();
    auto ex1 = di::Array { "/"_tsv, "abc"_tsv, "def"_tsv } | di::to<di::Vector>();
    ASSERT_EQ(r1, ex1);

    auto r2 = in2 | di::reverse | di::to<di::Vector>();
    auto ex2 = di::Array { "/"_tsv, "abc"_tsv, "def"_tsv } | di::reverse | di::to<di::Vector>();
    ASSERT_EQ(r2, ex2);

    auto in3 = "hello/friends"_pv;
    auto r3 = in3 | di::to<di::Vector>();
    auto ex3 = di::Array { "hello"_tsv, "friends"_tsv } | di::to<di::Vector>();
    ASSERT_EQ(r3, ex3);

    auto r4 = in3 | di::reverse | di::to<di::Vector>();
    auto ex4 = di::Array { "hello"_tsv, "friends"_tsv } | di::reverse | di::to<di::Vector>();
    ASSERT_EQ(r4, ex4);

    ASSERT(di::empty(""_pv));
}

TEST_CONSTEXPR(container_path, iteration, iteration)