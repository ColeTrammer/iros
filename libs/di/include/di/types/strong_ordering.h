#pragma once

#include "di/types/compare_outcome.h"
#include "di/types/partial_ordering.h"
#include "di/types/weak_ordering.h"

#ifndef DI_NO_USE_STD
#include <compare>
#else
namespace std {
class strong_ordering {
public:
    static strong_ordering const less;
    static strong_ordering const equivalent;
    static strong_ordering const equal;
    static strong_ordering const greater;

    constexpr friend auto operator==(strong_ordering v, strong_ordering w) -> bool { return v.m_value == w.m_value; }
    constexpr friend auto operator==(strong_ordering v, int) -> bool { return v.m_value == 0; }

    constexpr friend auto operator<(strong_ordering v, int) -> bool { return v.m_value < 0; }
    constexpr friend auto operator<(int, strong_ordering v) -> bool { return v.m_value > 0; }

    constexpr friend auto operator<=(strong_ordering v, int) -> bool { return v == 0 || v < 0; }
    constexpr friend auto operator<=(int, strong_ordering v) -> bool { return v == 0 || 0 < v; }

    constexpr friend auto operator>(strong_ordering v, int) -> bool { return 0 < v; }
    constexpr friend auto operator>(int, strong_ordering v) -> bool { return v < 0; }

    constexpr friend auto operator>=(strong_ordering v, int) -> bool { return 0 <= v; }
    constexpr friend auto operator>=(int, strong_ordering v) -> bool { return v <= 0; }

    constexpr friend auto operator<=>(strong_ordering v, int) -> strong_ordering { return v; }
    constexpr friend auto operator<=>(int, strong_ordering v) -> strong_ordering {
        return strong_ordering(char(-v.m_value));
    }

    constexpr operator partial_ordering() const { return partial_ordering(m_value); }
    constexpr operator weak_ordering() const { return weak_ordering(m_value); }

private:
    explicit constexpr strong_ordering(char value) : m_value(value) {}
    explicit constexpr strong_ordering(di::types::detail::CompareOutcome value) : m_value(static_cast<char>(value)) {}

    char m_value;
};

constexpr inline strong_ordering strong_ordering::less(di::types::detail::CompareOutcome::Less);
constexpr inline strong_ordering strong_ordering::equivalent(di::types::detail::CompareOutcome::Equal);
constexpr inline strong_ordering strong_ordering::equal(di::types::detail::CompareOutcome::Equal);
constexpr inline strong_ordering strong_ordering::greater(di::types::detail::CompareOutcome::Greater);
}
#endif

namespace di::types {
using std::strong_ordering;
}

namespace di {
using types::strong_ordering;
}
