#include <di/math/intcmp/prelude.h>
#include <di/math/prelude.h>
#include <dius/test/prelude.h>

namespace math_checked {
constexpr static void add_sub() {
    auto a = di::Checked(1);
    auto b = di::Checked(2);

    auto r1 = a + b;
    ASSERT(r1.valid());
    ASSERT_EQ(r1.value(), 3);

    a += di::NumericLimits<int>::max;
    ASSERT(a.invalid());

    a = -1;
    a -= di::NumericLimits<int>::max;
    ASSERT(a.valid());

    a--;
    ASSERT(a.invalid());

    auto c = di::Checked(0U);
    ASSERT(c.valid());
    c--;
    ASSERT(c.invalid());

    c = di::NumericLimits<unsigned int>::max;
    ASSERT(c.valid());
    c++;
    ASSERT(c.invalid());
}

constexpr static void mul_div_mod() {
    auto a = di::Checked(1);
    auto b = di::Checked(2);

    auto r1 = a * b;
    ASSERT(r1.valid());
    ASSERT_EQ(r1.value(), 2);

    b *= di::NumericLimits<int>::max;
    ASSERT(b.invalid());

    b = di::Checked(2);
    b /= 0;
    ASSERT(b.invalid());

    b = di::Checked(2);
    b %= 0;
    ASSERT(b.invalid());

    b = di::Checked(2);
    b %= 1;
    ASSERT(b.valid());

    b = di::Checked(2);
    b %= -1;
    ASSERT(b.valid());

    b = di::Checked(di::NumericLimits<int>::min);
    b /= -1;
    ASSERT(b.invalid());

    b = di::Checked(di::NumericLimits<int>::min);
    b %= -1;
    ASSERT(b.invalid());
}

constexpr static void bit_ops() {
    auto a = di::Checked(1);
    auto b = di::Checked(2);

    auto r1 = a & b;
    ASSERT(r1.valid());
    ASSERT_EQ(r1.value(), 0);

    r1 = a | b;
    ASSERT(r1.valid());
    ASSERT_EQ(r1.value(), 3);

    r1 = a ^ b;
    ASSERT(r1.valid());
    ASSERT_EQ(r1.value(), 3);

    r1 = ~a;
    ASSERT(r1.valid());
    ASSERT_EQ(r1.value(), -2);

    r1 = a << 1;
    ASSERT(r1.valid());
    ASSERT_EQ(r1.value(), 2);

    r1 = a >> 1;
    ASSERT(r1.valid());
    ASSERT_EQ(r1.value(), 0);

    r1 = a << 32;
    ASSERT(r1.invalid());

    r1 = a >> 32;
    ASSERT(r1.invalid());

    r1 = a << -1;
    ASSERT(r1.invalid());

    r1 = a >> -1;
    ASSERT(r1.invalid());

    r1 = a >> 31;
    ASSERT(r1.valid());

    r1 = a << 31;
    ASSERT(r1.valid());
}

TESTC(math_checked, add_sub)
TESTC(math_checked, mul_div_mod)
TESTC(math_checked, bit_ops)
}
