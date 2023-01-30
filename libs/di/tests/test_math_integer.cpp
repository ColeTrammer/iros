#include <dius/test/prelude.h>

template<di::concepts::SignedIntegral T>
constexpr void do_midpoint_signed_test() {
    T a = di::NumericLimits<T>::min;
    T b = di::NumericLimits<T>::max;

    auto r1 = di::midpoint(a, b);
    auto ex1 = -1;
    ASSERT_EQ(r1, ex1);

    auto r2 = di::midpoint(b, a);
    auto ex2 = 0;
    ASSERT_EQ(r2, ex2);

    T c = -1;
    T d = 0;

    auto r3 = di::midpoint(c, d);
    auto ex3 = -1;
    ASSERT_EQ(r3, ex3);

    auto r4 = di::midpoint(d, c);
    auto ex4 = 0;
    ASSERT_EQ(r4, ex4);
}

template<di::concepts::UnsignedIntegral T>
constexpr void do_midpoint_unsigned_test() {
    T a = di::NumericLimits<T>::min;
    T b = di::NumericLimits<T>::max;

    auto r1 = di::midpoint(a, b);
    auto ex1 = b / 2;
    ASSERT_EQ(r1, ex1);

    auto r2 = di::midpoint(b, a);
    auto ex2 = b / 2 + 1;
    ASSERT_EQ(r2, ex2);
}

constexpr void midpoint() {
    do_midpoint_signed_test<signed char>();
    do_midpoint_signed_test<signed short>();
    do_midpoint_signed_test<signed int>();
    do_midpoint_signed_test<signed long>();
    do_midpoint_signed_test<signed long long>();
    do_midpoint_signed_test<imax>();

    do_midpoint_unsigned_test<unsigned char>();
    do_midpoint_unsigned_test<unsigned short>();
    do_midpoint_unsigned_test<unsigned int>();
    do_midpoint_unsigned_test<unsigned long>();
    do_midpoint_unsigned_test<unsigned long long>();
    do_midpoint_unsigned_test<umax>();
}

TESTC(math_integer, midpoint)