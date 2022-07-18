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

enum class Flags { X = 1, Y = 2, Z = 4 };

LIIM_DEFINE_BITWISE_OPERATIONS(Flags)

constexpr void flags() {
    auto x = Flags::Y;
    x |= Flags::X;
    EXPECT(to_underlying(x & Flags::X));

    x &= ~Flags::X;
    EXPECT(~to_underlying(x & Flags::X));

    x ^= Flags::X | Flags::Y;
    EXPECT(x == Flags::X);
    EXPECT(!to_underlying(x ^ Flags::X));
}

TEST_CONSTEXPR(enum, flags, flags)
