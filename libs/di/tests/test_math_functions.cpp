#include <di/math/abs.h>
#include <di/math/functions.h>
#include <dius/test/prelude.h>

namespace math_functions {
constexpr static void abs() {
    ASSERT_EQ(di::abs(-1.0), 1.0);
    ASSERT_EQ(di::abs(1.0), 1.0);

    ASSERT_EQ(di::abs(0.0), 0.0);
    ASSERT_EQ(di::abs(-0.0), 0.0);

    ASSERT_EQ(di::abs(di::numbers::infinity), di::numbers::infinity);
    ASSERT_EQ(di::abs(-di::numbers::infinity), di::numbers::infinity);
}

constexpr static void signbit() {
    ASSERT(!di::signbit(1.0));
    ASSERT(di::signbit(-1.0));

    ASSERT(!di::signbit(0.0));
    ASSERT(di::signbit(-0.0));

    ASSERT(!di::signbit(di::numbers::infinity));
    ASSERT(di::signbit(-di::numbers::infinity));

    ASSERT(!di::signbit(di::numbers::quiet_nan));
    ASSERT(di::signbit(-di::numbers::quiet_nan));
}

constexpr static void copysign() {
    ASSERT_EQ(di::copysign(2.0, -1.0), -2.0);
    ASSERT_EQ(di::copysign(2.0, 1.0), 2.0);
    ASSERT_EQ(di::copysign(-2.0, 1.0), 2.0);
    ASSERT_EQ(di::copysign(-2.0, -1.0), -2.0);
}

constexpr static void round() {
    ASSERT_EQ(di::round(0.3), 0.0);
    ASSERT_EQ(di::round(0.5), 1.0);
    ASSERT_EQ(di::round(-0.5), -1.0);
    ASSERT_EQ(di::round(3.9), 4.0);
}

constexpr static void remainder() {
    ASSERT_APPROX_EQ(di::remainder(5.1, 3.0), -0.9);
    ASSERT_APPROX_EQ(di::remainder(-5.1, 3.0), 0.9);
    ASSERT_APPROX_EQ(di::remainder(5.1, -3.0), -0.9);
    ASSERT_APPROX_EQ(di::remainder(-5.1, -3.0), 0.9);
}

constexpr static void fmod() {
    ASSERT_APPROX_EQ(di::fmod(5.1, 3.0), 2.1);
    ASSERT_APPROX_EQ(di::fmod(-5.1, 3.0), -2.1);
    ASSERT_APPROX_EQ(di::fmod(5.1, -3.0), 2.1);
    ASSERT_APPROX_EQ(di::fmod(-5.1, -3.0), -2.1);
}

constexpr static void cos() {
    ASSERT_EQ(di::cos(0), 1.0);
    ASSERT_APPROX_EQ(di::cos(di::numbers::pi), -1.0);
    ASSERT_APPROX_EQ(di::cos(-di::numbers::pi), -1.0);
    ASSERT_APPROX_EQ(di::cos(120 * di::numbers::pi), 1.0);

    ASSERT_APPROX_EQ(di::cos(di::numbers::pi / 2), 0.0);
    ASSERT_APPROX_EQ(di::cos(di::numbers::pi / 4), 0.7071);
    ASSERT_APPROX_EQ(di::cos(-di::numbers::pi / 4), 0.7071);
    ASSERT_APPROX_EQ(di::cos(3 * di::numbers::pi / 4), -0.7071);
    ASSERT_APPROX_EQ(di::cos(di::numbers::pi / 4 + 2 * di::numbers::pi), 0.7071);
    ASSERT_APPROX_EQ(di::cos(di::numbers::pi / 4 - 2 * di::numbers::pi), 0.7071);
}

constexpr static void sin() {
    ASSERT_EQ(di::sin(0), 0.0);
    ASSERT_EQ(di::sin(di::numbers::pi), 0.0);
    ASSERT_EQ(di::sin(-di::numbers::pi), 0.0);
    ASSERT_EQ(di::sin(120 * di::numbers::pi), 0.0);

    ASSERT_APPROX_EQ(di::sin(di::numbers::pi / 2), 1.0);
    ASSERT_APPROX_EQ(di::sin(di::numbers::pi / 4), 0.7071);
    ASSERT_APPROX_EQ(di::sin(-di::numbers::pi / 4), -0.7071);
    ASSERT_APPROX_EQ(di::sin(3 * di::numbers::pi / 4), 0.7071);
    ASSERT_APPROX_EQ(di::sin(di::numbers::pi / 4 + 2 * di::numbers::pi), 0.7071);
    ASSERT_APPROX_EQ(di::sin(di::numbers::pi / 4 - 2 * di::numbers::pi), 0.7071);
}

TESTC(math_functions, abs)
TESTC(math_functions, signbit)
TESTC(math_functions, copysign)
TESTC(math_functions, round)
TESTC(math_functions, remainder)
TESTC(math_functions, fmod)
TESTC(math_functions, cos)
TESTC(math_functions, sin)
}
