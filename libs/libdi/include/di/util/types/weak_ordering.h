#pragma once

#include <di/util/types/compare_outcome.h>

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

private:
    explicit constexpr weak_ordering(char value) : m_value(value) {}
    explicit constexpr weak_ordering(di::util::types::detail::CompareOutcome value) : m_value(static_cast<char>(value)) {}

    char m_value;
};

inline constexpr weak_ordering weak_ordering::less(di::util::types::detail::CompareOutcome::Less);
inline constexpr weak_ordering weak_ordering::equivalent(di::util::types::detail::CompareOutcome::Equal);
inline constexpr weak_ordering weak_ordering::greater(di::util::types::detail::CompareOutcome::Greater);
}
#endif

namespace di::util::types {
using std::weak_ordering;
}
