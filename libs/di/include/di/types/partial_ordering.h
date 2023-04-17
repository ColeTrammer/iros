#pragma once

#include <di/types/compare_outcome.h>

#ifndef DI_NO_USE_STD
#include <compare>
#else
namespace std {
class partial_ordering {
public:
    static const partial_ordering less;
    static const partial_ordering equivalent;
    static const partial_ordering greater;
    static const partial_ordering unordered;

    constexpr friend bool operator==(partial_ordering v, partial_ordering w) = default;
    constexpr friend bool operator==(partial_ordering v, int) { return v.m_value == 0; }

    constexpr friend bool operator<(partial_ordering v, int) { return v.m_value < 0; }
    constexpr friend bool operator<(int, partial_ordering v) { return v.m_value == 1; }

    constexpr friend bool operator<=(partial_ordering v, int) { return v == 0 || v < 0; }
    constexpr friend bool operator<=(int, partial_ordering v) { return v == 0 || 0 < v; }

    constexpr friend bool operator>(partial_ordering v, int) { return 0 < v; }
    constexpr friend bool operator>(int, partial_ordering v) { return v < 0; }

    constexpr friend bool operator>=(partial_ordering v, int) { return 0 <= v; }
    constexpr friend bool operator>=(int, partial_ordering v) { return v <= 0; }

    constexpr friend partial_ordering operator<=>(partial_ordering v, int) { return v; }
    constexpr friend partial_ordering operator<=>(int, partial_ordering v) {
        return v.m_value == 2 ? v : partial_ordering(-v.m_value);
    }

private:
    friend class strong_ordering;
    friend class weak_ordering;

    explicit constexpr partial_ordering(char value) : m_value(value) {}
    explicit constexpr partial_ordering(di::types::detail::CompareOutcome value) : m_value(static_cast<char>(value)) {}

    char m_value;
};

constexpr inline partial_ordering partial_ordering::less(di::types::detail::CompareOutcome::Less);
constexpr inline partial_ordering partial_ordering::equivalent(di::types::detail::CompareOutcome::Equal);
constexpr inline partial_ordering partial_ordering::greater(di::types::detail::CompareOutcome::Greater);
constexpr inline partial_ordering partial_ordering::unordered(di::types::detail::CompareOutcome::Unordered);
}
#endif

namespace di::types {
using std::partial_ordering;
}
