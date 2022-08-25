#pragma once

#include <di/util/types/compare_outcome.h>

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

private:
    explicit constexpr strong_ordering(char value) : m_value(value) {}
    explicit constexpr strong_ordering(di::util::types::detail::CompareOutcome value) : m_value(static_cast<char>(value)) {}

    char m_value;
};

inline constexpr strong_ordering strong_ordering::less(di::util::types::detail::CompareOutcome::Less);
inline constexpr strong_ordering strong_ordering::equivalent(di::util::types::detail::CompareOutcome::Equal);
inline constexpr strong_ordering strong_ordering::equal(di::util::types::detail::CompareOutcome::Equal);
inline constexpr strong_ordering strong_ordering::greater(di::util::types::detail::CompareOutcome::Greater);
}
#endif

namespace di::util::types {
using std::strong_ordering;
}
