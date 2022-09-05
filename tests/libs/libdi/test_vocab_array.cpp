#include <di/prelude.h>
#include <test/test.h>

constexpr void to_array() {
    {
        int array[] = { 1, 2, 3, 4, 5 };
        auto x = di::to_array(array);
        EXPECT_EQ(x[0], 1);
        EXPECT_EQ(x[4], 5);
    }

    {
        auto x = di::to_array({ 1, 2, 3, 4, 5 });
        EXPECT_EQ(x[0], 1);
        EXPECT_EQ(x[4], 5);
    }
}

constexpr void span() {
    {
        auto x = di::to_array({ 1, 2, 3, 4 });
        auto y = x.first<2>();
        EXPECT_EQ(y[0], 1);
        EXPECT_EQ(y[1], 2);
    }
}

TEST_CONSTEXPR(vocab_array, to_array, to_array)
TEST_CONSTEXPR(vocab_array, span, span)
