#pragma once

#include <di/types/compare_outcome.h>
#include <di/types/partial_ordering.h>
#include <di/types/weak_ordering.h>

#ifndef DI_NO_USE_STD
#include <compare>
#else
namespace std {
class strong_ordering {
public:
    static const strong_ordering less;
    static const strong_ordering equivalent;
    static const strong_ordering equal;
    static const strong_ordering greater;

    constexpr friend bool operator==(strong_ordering v, strong_ordering w) { return v.m_value == w.m_value; }
    constexpr friend bool operator==(strong_ordering v, int) { return v.m_value == 0; }

    constexpr friend bool operator<(strong_ordering v, int) { return v.m_value < 0; }
    constexpr friend bool operator<(int, strong_ordering v) { return v.m_value > 0; }

    constexpr friend bool operator<=(strong_ordering v, int) { return v == 0 || v < 0; }
    constexpr friend bool operator<=(int, strong_ordering v) { return v == 0 || 0 < v; }

    constexpr friend bool operator>(strong_ordering v, int) { return 0 < v; }
    constexpr friend bool operator>(int, strong_ordering v) { return v < 0; }

    constexpr friend bool operator>=(strong_ordering v, int) { return 0 <= v; }
    constexpr friend bool operator>=(int, strong_ordering v) { return v <= 0; }

    constexpr friend strong_ordering operator<=>(strong_ordering v, int) { return v; }
    constexpr friend strong_ordering operator<=>(int, strong_ordering v) { return strong_ordering(-v.m_value); }

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
