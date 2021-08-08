#include <liim/format.h>
#include <test/test.h>

TEST(format, automatic_indexing) {
    EXPECT_EQ(format("{} has {:}", "3", "2"), "3 has 2");
}

TEST(format, manual_indexing) {
    EXPECT_EQ(format("{1:} is {0:} + {0}", "2", "4"), "4 is 2 + 2");
}

TEST(format, fill_and_align) {
    EXPECT_EQ(format("{:<5}", "x"), "x    ");
    EXPECT_EQ(format("{:*<5}", "x"), "x****");

    EXPECT_EQ(format("{:*^5}", "x"), "**x**");
    EXPECT_EQ(format("{:*^6}", "x"), "**x***");
    EXPECT_EQ(format("{:^6}", "x"), "  x   ");

    EXPECT_EQ(format("{:>5}", "x"), "    x");
    EXPECT_EQ(format("{:*>5}", "x"), "****x");
}

TEST(format, string_precision) {
    EXPECT_EQ(format("{:10.5}", "x"), "x         ");
    EXPECT_EQ(format("{:*^10.5}", "xxxyyy"), "**xxxyy***");

    EXPECT_EQ(format("{:.2}", "abcde"), "ab");
}
