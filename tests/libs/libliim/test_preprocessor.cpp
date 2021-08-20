#include <liim/preprocessor.h>
#include <test/test.h>

TEST(preprocessor, logic) {
    EXPECT_EQ(LIIM_NOT(0), 1);
    EXPECT_EQ(LIIM_NOT(x), 0);
    EXPECT_EQ(LIIM_NOT(1), 0);
    EXPECT_EQ(LIIM_NOT(5), 0);

    EXPECT_EQ(LIIM_BOOL(x), 1);
    EXPECT_EQ(LIIM_BOOL(1), 1);
    EXPECT_EQ(LIIM_BOOL(5), 1);
    EXPECT_EQ(LIIM_BOOL(0), 0);

    EXPECT_EQ(LIIM_IF(x)("true")("false"), "true");
    EXPECT_EQ(LIIM_IF(1)("true")("false"), "true");
    EXPECT_EQ(LIIM_IF(0)("true")("false"), "false");
}

TEST(preprocessor, detection) {
    EXPECT_EQ(LIIM_IS_PAREN(), 0);
    EXPECT_EQ(LIIM_IS_PAREN(x), 0);
    EXPECT_EQ(LIIM_IS_PAREN(()), 1);
    EXPECT_EQ(LIIM_IS_PAREN((1, 2)), 1);
    EXPECT_EQ(LIIM_IS_PAREN((x, y)), 1);

    EXPECT_EQ(LIIM_NOT_EMPTY(), 0);
    EXPECT_EQ(LIIM_NOT_EMPTY(1), 1);
    EXPECT_EQ(LIIM_NOT_EMPTY(1, 3), 1);
    EXPECT_EQ(LIIM_NOT_EMPTY((x, "a")), 1);
    EXPECT_EQ(LIIM_NOT_EMPTY((x, "a"), (b, c)), 1);
}

TEST(preprocessor, for_each) {
#define CONCAT(x) "|" #x

    EXPECT_EQ(LIIM_EVAL(LIIM_FOR_EACH(CONCAT, a, b, c)), "|a|b|c"s);

#define CONCAT2(x, y) "" #x "|" #y

    EXPECT_EQ(LIIM_EVAL(LIIM_FOR_EACH(CONCAT2, (a, b), (c, d))), "a|bc|d"s);

#define CONCAT3(...) LIIM_FOR_EACH(CONCAT, __VA_ARGS__)

    EXPECT_EQ(LIIM_EVAL(LIIM_FOR_EACH(CONCAT3, (a, b, c), (d, e, f))), "|a|b|c|d|e|f"s);
}
