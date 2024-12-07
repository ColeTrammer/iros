#include "di/math/prelude.h"
#include "dius/test/prelude.h"

namespace math_integer {
template<di::concepts::SignedIntegral T>
constexpr static void do_midpoint_signed_test() {
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
constexpr static void do_midpoint_unsigned_test() {
    T a = di::NumericLimits<T>::min;
    T b = di::NumericLimits<T>::max;

    auto r1 = di::midpoint(a, b);
    auto ex1 = b / 2;
    ASSERT_EQ(r1, ex1);

    auto r2 = di::midpoint(b, a);
    auto ex2 = b / 2 + 1;
    ASSERT_EQ(r2, ex2);
}

constexpr static void midpoint() {
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

template<di::concepts::Integral T>
constexpr static void do_abs_diff_test() {
    using U = di::meta::MakeUnsigned<T>;

    auto a = di::NumericLimits<T>::min;
    auto b = di::NumericLimits<T>::max;

    auto r1 = di::abs_diff(a, b);
    auto ex1 = di::NumericLimits<U>::max;

    ASSERT_EQ(r1, ex1);

    auto r2 = di::abs_diff(b, a);
    auto ex2 = di::NumericLimits<U>::max;

    ASSERT_EQ(r2, ex2);
}

constexpr static void abs_diff() {
    do_abs_diff_test<signed char>();
    do_abs_diff_test<signed short>();
    do_abs_diff_test<signed int>();
    do_abs_diff_test<signed long>();
    do_abs_diff_test<signed long long>();
    do_abs_diff_test<imax>();

    do_abs_diff_test<unsigned char>();
    do_abs_diff_test<unsigned short>();
    do_abs_diff_test<unsigned int>();
    do_abs_diff_test<unsigned long>();
    do_abs_diff_test<unsigned long long>();
    do_abs_diff_test<umax>();
}

TESTC(math_integer, midpoint)
TESTC(math_integer, abs_diff)
}
