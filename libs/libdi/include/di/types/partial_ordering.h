#pragma once

#include <di/types/compare_outcome.h>

#ifdef DI_USE_STD
#include <compare>
#else
namespace std {
class partial_ordering {
public:
    static const partial_ordering less;
    static const partial_ordering equivalent;
    static const partial_ordering greater;
    static const partial_ordering unordered;

    friend constexpr bool operator==(partial_ordering v, partial_ordering w) = default;
    friend constexpr bool operator==(partial_ordering v, int) { return v.m_value == 0; }

    friend constexpr bool operator<(partial_ordering v, int) { return v.m_value < 0; }
    friend constexpr bool operator<(int, partial_ordering v) { return v.m_value == 1; }

    friend constexpr bool operator<=(partial_ordering v, int) { return v == 0 || v < 0; }
    friend constexpr bool operator<=(int, partial_ordering v) { return v == 0 || 0 < v; }

    friend constexpr bool operator>(partial_ordering v, int) { return 0 < v; }
    friend constexpr bool operator>(int, partial_ordering v) { return v < 0; }

    friend constexpr bool operator>=(partial_ordering v, int) { return 0 <= v; }
    friend constexpr bool operator>=(int, partial_ordering v) { return v <= 0; }

    friend constexpr partial_ordering operator<=>(partial_ordering v, int) { return v; }
    friend constexpr partial_ordering operator<=>(int, partial_ordering v) {
        return v.m_value == 2 ? v : partial_ordering(-v.m_value);
    }

private:
    friend class strong_ordering;
    friend class weak_ordering;

    explicit constexpr partial_ordering(char value) : m_value(value) {}
    explicit constexpr partial_ordering(di::types::detail::CompareOutcome value) : m_value(static_cast<char>(value)) {}

    char m_value;
};

inline constexpr partial_ordering partial_ordering::less(di::types::detail::CompareOutcome::Less);
inline constexpr partial_ordering partial_ordering::equivalent(di::types::detail::CompareOutcome::Equal);
inline constexpr partial_ordering partial_ordering::greater(di::types::detail::CompareOutcome::Greater);
inline constexpr partial_ordering partial_ordering::unordered(di::types::detail::CompareOutcome::Unordered);
}
#endif

namespace di::types {
using std::partial_ordering;
}
