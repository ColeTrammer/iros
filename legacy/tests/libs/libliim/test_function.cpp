#include <liim/function.h>
#include <test/test.h>

#if 0
TEST(function, constexpr) {
    constexpr int x = [] {
        int y = 12;
        Function<int(int)> add = [y](int x) {
            return x + 12;
        };
        return add(30);
    }();

    EXPECT_EQ(x, 42);
}
#endif
