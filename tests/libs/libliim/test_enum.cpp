#include <liim/enum.h>
#include <test/test.h>

#define __X_ENUM(m) m(W) m(Y) m(Z)

LIIM_ENUM(X, Enum, __X_ENUM)

TEST(enum, basic) {
    auto x = X::Enum::W;
    EXPECT_EQ(format("{}", x), "W");
    EXPECT_EQ(format("{}", X::Enum::Y), "Y");
    EXPECT_EQ(format("{}", X::Enum::Z), "Z");
}
