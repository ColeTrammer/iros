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

TEST(format, integers) {
    EXPECT_EQ(format("{}", 0), "0");
    EXPECT_EQ(format("{}", 5), "5");
    EXPECT_EQ(format("{}", -3), "-3");
    EXPECT_EQ(format("{:+}", 5), "+5");
    EXPECT_EQ(format("{: }", 5), " 5");
    EXPECT_EQ(format("{:+#b}", 3), "+0b11");
    EXPECT_EQ(format("{:+#B}", 3), "+0B11");
    EXPECT_EQ(format("{:#o}", 0101), "0o101");
    EXPECT_EQ(format("{:#O}", 0101), "0O101");
    EXPECT_EQ(format("{:#X}", 15), "0XF");
    EXPECT_EQ(format("{:#x}", 15), "0xf");

    EXPECT_EQ(format("{: #5X}", 0x12345), " 0X12345");
    EXPECT_EQ(format("{:5}", 234), "  234");
    EXPECT_EQ(format("{:05}", 234), "00234");
    EXPECT_EQ(format("{:+#08x}", 0x234), "+0x00234");
    EXPECT_EQ(format("{:*^5}", 234), "*234*");
}

TEST(format, characters) {
    EXPECT_EQ(format("{}", 'e'), "e");
    EXPECT_EQ(format("{:*>3}", '3'), "**3");
}

TEST(format, escape_braces) {
    EXPECT_EQ(format("{{"), "{");
    EXPECT_EQ(format("{{}}"), "{}");
    EXPECT_EQ(format("xyz}}"), "xyz}");
    EXPECT_EQ(format("}}}}xyz}}"), "}}xyz}");
    EXPECT_EQ(format("{{x{{y}}z}}"), "{x{y}z}");
}
