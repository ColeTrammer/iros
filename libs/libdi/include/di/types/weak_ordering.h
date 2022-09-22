#pragma once

#include <di/types/compare_outcome.h>
#include <di/types/partial_ordering.h>

#ifdef DI_USE_STD
#include <compare>
#else
namespace std {
class weak_ordering {
public:
    static const weak_ordering less;
    static const weak_ordering equivalent;
    static const weak_ordering greater;

    friend constexpr bool operator==(weak_ordering v, weak_ordering w) { return v.m_value == w.m_value; }
    friend constexpr bool operator==(weak_ordering v, int) { return v.m_value == 0; }

    friend constexpr bool operator<(weak_ordering v, int) { return v.m_value < 0; }
    friend constexpr bool operator<(int, weak_ordering v) { return v.m_value > 0; }

    friend constexpr bool operator<=(weak_ordering v, int) { return v == 0 || v < 0; }
    friend constexpr bool operator<=(int, weak_ordering v) { return v == 0 || 0 < v; }

    friend constexpr bool operator>(weak_ordering v, int) { return 0 < v; }
    friend constexpr bool operator>(int, weak_ordering v) { return v < 0; }

    friend constexpr bool operator>=(weak_ordering v, int) { return 0 <= v; }
    friend constexpr bool operator>=(int, weak_ordering v) { return v <= 0; }

    friend constexpr weak_ordering operator<=>(weak_ordering v, int) { return v; }
    friend constexpr weak_ordering operator<=>(int, weak_ordering v) { return weak_ordering(-v.m_value); }

    constexpr operator partial_ordering() const { return partial_ordering(m_value); }

private:
    friend class strong_ordering;

    explicit constexpr weak_ordering(char value) : m_value(value) {}
    explicit constexpr weak_ordering(di::types::detail::CompareOutcome value) : m_value(static_cast<char>(value)) {}

    char m_value;
};

inline constexpr weak_ordering weak_ordering::less(di::types::detail::CompareOutcome::Less);
inline constexpr weak_ordering weak_ordering::equivalent(di::types::detail::CompareOutcome::Equal);
inline constexpr weak_ordering weak_ordering::greater(di::types::detail::CompareOutcome::Greater);
}
#endif

namespace di::types {
using std::weak_ordering;
}
