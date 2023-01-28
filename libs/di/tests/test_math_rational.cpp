#include <di/prelude.h>
#include <dius/test/prelude.h>

constexpr void basic() {
    auto a = di::Rational { 3, 6 };
    ASSERT_EQ(a.numerator(), 1);
    ASSERT_EQ(a.denominator(), 2);

    auto b = di::Rational { 4, 7 };
    ASSERT_EQ(b.numerator(), 4);
    ASSERT_EQ(b.denominator(), 7);
    ASSERT(!b.negative());

    ASSERT_LT(a, b);
    ASSERT_GT(b, a);

    auto c = di::Rational { 4, -7 };
    ASSERT_EQ(c.numerator(), -4);
    ASSERT_EQ(c.denominator(), 7);
    ASSERT(c.negative());

    ASSERT_LT(c, b);
    ASSERT_LT(c, a);

    ASSERT_EQ(di::Rational(0, 5), di::Rational(0, 3));

    ASSERT_EQ(di::Rational(4, 5) * di::Rational(1, 4), di::Rational(1, 5));

    ASSERT_EQ(di::Rational(11, 3) + di::Rational(15, 9), di::Rational(48, 9));

    auto x = di::Rational(3, 5);
    x++;
    ASSERT_EQ(x, di::Rational(8, 5));
}

constexpr void ratio() {
    using X = di::Ratio<4, 5>;
    using Y = di::Ratio<4, 1>;
    using Z = di::RatioDivide<X, Y>;

    static_assert(Z::rational == di::Rational(1, 5));
}

TESTC(math_rational, basic)
TESTC(math_rational, ratio)