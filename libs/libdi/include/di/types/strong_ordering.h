#pragma once

#include <di/types/compare_outcome.h>
#include <di/types/partial_ordering.h>
#include <di/types/weak_ordering.h>

#ifdef DI_USE_STD
#include <compare>
#else
namespace std {
class strong_ordering {
public:
    static const strong_ordering less;
    static const strong_ordering equivalent;
    static const strong_ordering equal;
    static const strong_ordering greater;

    friend constexpr bool operator==(strong_ordering v, strong_ordering w) { return v.m_value == w.m_value; }
    friend constexpr bool operator==(strong_ordering v, int) { return v.m_value == 0; }

    friend constexpr bool operator<(strong_ordering v, int) { return v.m_value < 0; }
    friend constexpr bool operator<(int, strong_ordering v) { return v.m_value > 0; }

    friend constexpr bool operator<=(strong_ordering v, int) { return v == 0 || v < 0; }
    friend constexpr bool operator<=(int, strong_ordering v) { return v == 0 || 0 < v; }

    friend constexpr bool operator>(strong_ordering v, int) { return 0 < v; }
    friend constexpr bool operator>(int, strong_ordering v) { return v < 0; }

    friend constexpr bool operator>=(strong_ordering v, int) { return 0 <= v; }
    friend constexpr bool operator>=(int, strong_ordering v) { return v <= 0; }

    friend constexpr strong_ordering operator<=>(strong_ordering v, int) { return v; }
    friend constexpr strong_ordering operator<=>(int, strong_ordering v) { return strong_ordering(-v.m_value); }

    constexpr operator partial_ordering() const { return partial_ordering(m_value); }
    constexpr operator weak_ordering() const { return weak_ordering(m_value); }

private:
    explicit constexpr strong_ordering(char value) : m_value(value) {}
    explicit constexpr strong_ordering(di::types::detail::CompareOutcome value) : m_value(static_cast<char>(value)) {}

    char m_value;
};

inline constexpr strong_ordering strong_ordering::less(di::types::detail::CompareOutcome::Less);
inline constexpr strong_ordering strong_ordering::equivalent(di::types::detail::CompareOutcome::Equal);
inline constexpr strong_ordering strong_ordering::equal(di::types::detail::CompareOutcome::Equal);
inline constexpr strong_ordering strong_ordering::greater(di::types::detail::CompareOutcome::Greater);
}
#endif

namespace di::types {
using std::strong_ordering;
}
