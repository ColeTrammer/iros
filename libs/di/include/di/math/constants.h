#pragma once

#include "di/math/numeric_limits.h"
#include "di/meta/language.h"
#include "di/types/floats.h"

namespace di::numbers {
// NOTE: a 128 bit float has between 33 to 36 digits of precision. So we should define each
// constant with 36 significant digits.

template<concepts::FloatingPoint T>
constexpr inline auto e_v = T(2.71828182845904523536028747135266249);

template<concepts::FloatingPoint T>
constexpr inline auto log2e_v = T(1.44269504088896340735992468100189213);

template<concepts::FloatingPoint T>
constexpr inline auto log10e_v = T(0.434294481903251827651128918916605082);

template<concepts::FloatingPoint T>
constexpr inline auto pi_v = T(3.14159265358979323846264338327950288);

template<concepts::FloatingPoint T>
constexpr inline auto inv_pi_v = T(0.318309886183790671537767526745028724);

template<concepts::FloatingPoint T>
constexpr inline auto inv_pisqrt_v = T(0.564189583547756286948079451560772585);

template<concepts::FloatingPoint T>
constexpr inline auto ln2_v = T(0.693147180559945309417232121458176568);

template<concepts::FloatingPoint T>
constexpr inline auto ln10_v = T(2.30258509299404568401799145468436420);

template<concepts::FloatingPoint T>
constexpr inline auto sqrt2_v = T(1.41421356237309504880168872420969807);

template<concepts::FloatingPoint T>
constexpr inline auto sqrt3_v = T(1.73205080756887729352744634150587236);

template<concepts::FloatingPoint T>
constexpr inline auto inv_sqrt3_v = T(0.577350269189625764509148780501957455);

template<concepts::FloatingPoint T>
constexpr inline auto egamma_v = T(0.577215664901532860606512090082402431);

template<concepts::FloatingPoint T>
constexpr inline auto phi_v = T(1.61803398874989484820458683436563811);

constexpr inline auto e = e_v<f64>;
constexpr inline auto log2e = log2e_v<f64>;
constexpr inline auto log10e = log10e_v<f64>;
constexpr inline auto pi = pi_v<f64>;
constexpr inline auto inv_pi = inv_pi_v<f64>;
constexpr inline auto inv_pisqrt = inv_pisqrt_v<f64>;
constexpr inline auto ln2 = ln2_v<f64>;
constexpr inline auto ln10 = ln10_v<f64>;
constexpr inline auto sqrt2 = sqrt2_v<f64>;
constexpr inline auto sqrt3 = sqrt3_v<f64>;
constexpr inline auto inv_sqrt3 = inv_sqrt3_v<f64>;
constexpr inline auto egamma = egamma_v<f64>;
constexpr inline auto phi = phi_v<f64>;

constexpr inline auto infinity = NumericLimits<f64>::infinity;
constexpr inline auto quiet_nan = NumericLimits<f64>::quiet_nan;
constexpr inline auto signaling_nan = NumericLimits<f64>::signaling_nan;
}
