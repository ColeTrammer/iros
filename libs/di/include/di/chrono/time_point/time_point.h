#pragma once

#include <di/chrono/duration/prelude.h>
#include <di/chrono/time_point/time_point_common_type.h>
#include <di/concepts/equality_comparable_with.h>
#include <di/concepts/three_way_comparable_with.h>

namespace di::chrono {
template<typename C, concepts::InstanceOf<Duration> Dur>
class TimePoint {
public:
    using Clock = C;
    using Duration = Dur;
    using Representation = typename Duration::Representation;
    using Period = typename Duration::Period;

    constexpr static TimePoint min = TimePoint(Duration::min());
    constexpr static TimePoint max = TimePoint(Duration::max());

    TimePoint() = default;

    constexpr explicit TimePoint(Duration const& duration) : m_duration(duration) {}

    template<concepts::ImplicitlyConvertibleTo<Duration> D>
    constexpr TimePoint(TimePoint<Clock, D> const& other) : m_duration(other.time_since_epoch()) {}

    constexpr Duration time_since_epoch() const { return m_duration; }

    constexpr TimePoint& operator+=(Duration const& other) { return m_duration += other, *this; }
    constexpr TimePoint& operator-=(Duration const& other) { return m_duration -= other, *this; }

    constexpr TimePoint& operator++() { return ++m_duration, *this; }
    constexpr TimePoint operator++(int) { return TimePoint(m_duration++); }

    constexpr TimePoint& operator--() { return --m_duration, *this; }
    constexpr TimePoint operator--(int) { return TimePoint(m_duration--); }

private:
    Duration m_duration {};
};

template<typename C, typename D1, typename R2, typename P2,
         typename CT = TimePoint<C, meta::CommonType<D1, Duration<R2, P2>>>>
constexpr CT operator+(TimePoint<C, D1> const& a, Duration<R2, P2> const& b) {
    return CT(a.time_since_epoch() + b);
}

template<typename R1, typename P1, typename C, typename D2,
         typename CT = TimePoint<C, meta::CommonType<Duration<R1, P1>, D2>>>
constexpr CT operator+(Duration<R1, P1> const& a, TimePoint<C, D2> const& b) {
    return CT(b.time_since_epoch() + a);
}

template<typename C, typename D1, typename R2, typename P2,
         typename CT = TimePoint<C, meta::CommonType<D1, Duration<R2, P2>>>>
constexpr CT operator-(TimePoint<C, D1> const& a, Duration<R2, P2> const& b) {
    return CT(a.time_since_epoch() - b);
}

template<typename C, typename D1, typename D2, typename CT = meta::CommonType<D1, D2>>
constexpr CT operator-(TimePoint<C, D1> const& a, TimePoint<C, D2> const& b) {
    return CT(a.time_since_epoch() - b.time_since_epoch());
}

template<typename C, typename D1, concepts::EqualityComparableWith<D1> D2>
constexpr bool operator==(TimePoint<C, D1> const& a, TimePoint<C, D2> const& b) {
    return a.time_since_epoch() == b.time_since_epoch();
}

template<typename C, typename D1, concepts::ThreeWayComparableWith<D1> D2>
constexpr auto operator<=>(TimePoint<C, D1> const& a, TimePoint<C, D2> const& b) {
    return a.time_since_epoch() <=> b.time_since_epoch();
}
}
