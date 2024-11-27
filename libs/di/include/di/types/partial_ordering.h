#pragma once

#include <di/types/compare_outcome.h>

#ifndef DI_NO_USE_STD
#include <compare>
#else
namespace std {
class partial_ordering {
public:
    static partial_ordering const less;
    static partial_ordering const equivalent;
    static partial_ordering const greater;
    static partial_ordering const unordered;

    constexpr friend auto operator==(partial_ordering v, partial_ordering w) -> bool = default;
    constexpr friend auto operator==(partial_ordering v, int) -> bool { return v.m_value == 0; }

    constexpr friend auto operator<(partial_ordering v, int) -> bool { return v.m_value < 0; }
    constexpr friend auto operator<(int, partial_ordering v) -> bool { return v.m_value == 1; }

    constexpr friend auto operator<=(partial_ordering v, int) -> bool { return v == 0 || v < 0; }
    constexpr friend auto operator<=(int, partial_ordering v) -> bool { return v == 0 || 0 < v; }

    constexpr friend auto operator>(partial_ordering v, int) -> bool { return 0 < v; }
    constexpr friend auto operator>(int, partial_ordering v) -> bool { return v < 0; }

    constexpr friend auto operator>=(partial_ordering v, int) -> bool { return 0 <= v; }
    constexpr friend auto operator>=(int, partial_ordering v) -> bool { return v <= 0; }

    constexpr friend auto operator<=>(partial_ordering v, int) -> partial_ordering { return v; }
    constexpr friend auto operator<=>(int, partial_ordering v) -> partial_ordering {
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

namespace di {
using types::partial_ordering;
}
