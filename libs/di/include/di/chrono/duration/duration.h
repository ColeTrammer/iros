#pragma once

#include <di/chrono/duration/duration_common_type.h>
#include <di/concepts/common_with.h>
#include <di/concepts/three_way_comparable.h>
#include <di/math/numeric_limits.h>
#include <di/math/rational/prelude.h>
#include <di/meta/common_type.h>

namespace di::chrono {
template<typename Rep, math::detail::IsRatio Per>
class Duration {
public:
    using Representation = Rep;
    using Period = Per::Type;

    constexpr static auto zero() { return Duration(Representation(0)); }
    constexpr static auto min() { return Duration(math::NumericLimits<Representation>::min); }
    constexpr static auto max() { return Duration(math::NumericLimits<Representation>::max); }

    Duration() = default;
    Duration(Duration const&) = default;

    template<typename Val>
    requires(concepts::ConvertibleTo<Val const&, Representation>)
    constexpr explicit Duration(Val const& count) : m_count(count) {}

    template<typename Rep2, typename Period2>
    requires(math::RatioDivide<Period2, Period>::den == 1)
    constexpr Duration(Duration<Rep2, Period2> const& other) {
        constexpr auto conversion_factor = math::RatioDivide<Period2, Period>::rational;
        m_count = Representation(other.count()) * Representation(conversion_factor.numerator());
    }

    Duration& operator=(Duration const&) = default;

    constexpr Representation count() const { return m_count; }

    constexpr auto operator+() const { return meta::CommonType<Duration> { count() }; }
    constexpr auto operator-() const { return meta::CommonType<Duration> { -count() }; }

    constexpr Duration& operator++() { return ++m_count, *this; }
    constexpr Duration operator++(int) { return Duration(m_count++); }

    constexpr Duration& operator--() { return --m_count, *this; }
    constexpr Duration operator--(int) { return Duration(m_count--); }

    constexpr Duration& operator+=(Duration const& other) { return m_count += other.count(), *this; }
    constexpr Duration& operator-=(Duration const& other) { return m_count -= other.count(), *this; }
    constexpr Duration& operator*=(Representation const& other) { return m_count *= other, *this; }
    constexpr Duration& operator/=(Representation const& other) { return m_count /= other, *this; }
    constexpr Duration& operator%=(Representation const& other) { return m_count %= other, *this; }
    constexpr Duration& operator%=(Duration const& other) { return m_count %= other.count(), *this; }

private:
    Representation m_count;
};

template<typename Rep1, math::detail::IsRatio Period1, concepts::CommonWith<Rep1> Rep2, math::detail::IsRatio Period2>
constexpr auto operator+(Duration<Rep1, Period1> const& a, Duration<Rep2, Period2> const& b) {
    using R = meta::CommonType<Duration<Rep1, Period1>, Duration<Rep2, Period2>>;
    return R(R(a).count() + R(b).count());
}

template<typename Rep1, math::detail::IsRatio Period1, concepts::CommonWith<Rep1> Rep2, math::detail::IsRatio Period2>
constexpr auto operator-(Duration<Rep1, Period1> const& a, Duration<Rep2, Period2> const& b) {
    using R = meta::CommonType<Duration<Rep1, Period1>, Duration<Rep2, Period2>>;
    return R(R(a).count() - R(b).count());
}

template<typename Rep, math::detail::IsRatio Period, concepts::CommonWith<Rep> Value>
constexpr auto operator*(Duration<Rep, Period> const& a, Value const& b) {
    using R = meta::CommonType<Duration<Rep, Period>, Duration<Value, Period>>;
    return R(R(a).count() * b);
}

template<typename Rep, math::detail::IsRatio Period, concepts::CommonWith<Rep> Value>
constexpr auto operator*(Value const& a, Duration<Rep, Period> const& b) {
    using R = meta::CommonType<Duration<Rep, Period>, Duration<Value, Period>>;
    return R(a * R(b).count());
}

template<typename Rep, math::detail::IsRatio Period, concepts::CommonWith<Rep> Value>
constexpr auto operator/(Duration<Rep, Period> const& a, Value const& b) {
    using R = meta::CommonType<Duration<Rep, Period>, Duration<Value, Period>>;
    return R(R(a).count() / b);
}

template<typename Rep1, math::detail::IsRatio Period1, concepts::CommonWith<Rep1> Rep2, math::detail::IsRatio Period2>
constexpr auto operator/(Duration<Rep1, Period1> const& a, Duration<Rep2, Period2> const& b) {
    using D = meta::CommonType<Duration<Rep1, Period1>, Duration<Rep2, Period2>>;
    using R = meta::CommonType<Rep1, Rep2>;
    return R(D(a).count() / D(b).count());
}

template<typename Rep, math::detail::IsRatio Period, concepts::CommonWith<Rep> Value>
constexpr auto operator%(Duration<Rep, Period> const& a, Value const& b) {
    using R = meta::CommonType<Duration<Rep, Period>, Duration<Value, Period>>;
    return R(R(a).count() % b);
}

template<typename Rep1, math::detail::IsRatio Period1, concepts::CommonWith<Rep1> Rep2, math::detail::IsRatio Period2>
constexpr auto operator%(Duration<Rep1, Period1> const& a, Duration<Rep2, Period2> const& b) {
    using R = meta::CommonType<Duration<Rep1, Period1>, Duration<Rep2, Period2>>;
    return R(R(a).count() % R(b).count());
}

template<typename Rep1, math::detail::IsRatio Period1, concepts::CommonWith<Rep1> Rep2, math::detail::IsRatio Period2>
constexpr bool operator==(Duration<Rep1, Period1> const& a, Duration<Rep2, Period2> const& b) {
    using D = meta::CommonType<Duration<Rep1, Period1>, Duration<Rep2, Period2>>;
    return D(a).count() == D(b).count();
}

template<typename Rep1, math::detail::IsRatio Period1, concepts::CommonWith<Rep1> Rep2, math::detail::IsRatio Period2>
requires(concepts::ThreeWayComparable<meta::CommonType<Rep1, Rep2>>)
constexpr auto operator<=>(Duration<Rep1, Period1> const& a, Duration<Rep2, Period2> const& b) {
    using D = meta::CommonType<Duration<Rep1, Period1>, Duration<Rep2, Period2>>;
    return D(a).count() <=> D(b).count();
}
}
