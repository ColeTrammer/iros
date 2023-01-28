#pragma once

#include <stdint.h>
#if !defined(__is_libc) && !defined(__is_libk)
#include <compare>
#else
namespace LIIM::Compare::Detail {
// NOTE: this is the same layout libstdc++ uses, so don't change this. The compiler might rely on it.
enum class CompareOutcome : char { Less = -1, Equal = 0, Greater = 1, Unordered = 2 };
}

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
    friend constexpr partial_ordering operator<=>(int, partial_ordering v) { return v.m_value == 2 ? v : partial_ordering(-v.m_value); }

private:
    explicit constexpr partial_ordering(char value) : m_value(value) {}
    explicit constexpr partial_ordering(LIIM::Compare::Detail::CompareOutcome value) : m_value(static_cast<char>(value)) {}

    char m_value;
};

inline constexpr partial_ordering partial_ordering::less(LIIM::Compare::Detail::CompareOutcome::Less);
inline constexpr partial_ordering partial_ordering::equivalent(LIIM::Compare::Detail::CompareOutcome::Equal);
inline constexpr partial_ordering partial_ordering::greater(LIIM::Compare::Detail::CompareOutcome::Greater);
inline constexpr partial_ordering partial_ordering::unordered(LIIM::Compare::Detail::CompareOutcome::Unordered);

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
    explicit constexpr weak_ordering(LIIM::Compare::Detail::CompareOutcome value) : m_value(static_cast<char>(value)) {}

    char m_value;
};

inline constexpr weak_ordering weak_ordering::less(LIIM::Compare::Detail::CompareOutcome::Less);
inline constexpr weak_ordering weak_ordering::equivalent(LIIM::Compare::Detail::CompareOutcome::Equal);
inline constexpr weak_ordering weak_ordering::greater(LIIM::Compare::Detail::CompareOutcome::Greater);

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
    explicit constexpr strong_ordering(LIIM::Compare::Detail::CompareOutcome value) : m_value(static_cast<char>(value)) {}

    char m_value;
};

inline constexpr strong_ordering strong_ordering::less(LIIM::Compare::Detail::CompareOutcome::Less);
inline constexpr strong_ordering strong_ordering::equivalent(LIIM::Compare::Detail::CompareOutcome::Equal);
inline constexpr strong_ordering strong_ordering::equal(LIIM::Compare::Detail::CompareOutcome::Equal);
inline constexpr strong_ordering strong_ordering::greater(LIIM::Compare::Detail::CompareOutcome::Greater);
}
#endif
#include <liim/utilities.h>

namespace LIIM {
namespace Detail {
    template<typename T, typename U>
    struct ThreeWayCompareResult {
        using Type = decltype(declval<T>() <=> declval<U>());
    };
}

template<typename T, typename U = T>
using ThreeWayCompareResult = Detail::ThreeWayCompareResult<T, U>::Type;

template<typename T>
concept EqualComparable = requires(const T& t) {
    { t == t } -> SameAs<bool>;
};

template<typename U, typename T>
concept EqualComparableWith = requires(const T& t, const U& u) {
    { t == u } -> SameAs<bool>;
};

template<typename T>
concept Comparable = requires(const T& t) {
    t <=> t;
};

template<typename U, typename T>
concept ComparableWith = requires(const T& t, const U& u) {
    t <=> u;
};

template<typename Comp, typename T, typename U = T>
concept ComparatorFor = requires(const Comp& comparator, const T& a, const U& b) {
    { comparator(a, b) } -> SameAs<bool>;
};

template<typename T>
concept ComparisonCategory = OneOf<T, std::strong_ordering, std::weak_ordering, std::partial_ordering>;

template<typename Comp, typename T, typename U = T>
concept ThreeWayComparatorFor = requires(const Comp& comparator, const T& a, const U& b) {
    { comparator(a, b) } -> ComparisonCategory;
};

namespace Detail {
    template<ComparisonCategory A, ComparisonCategory B>
    struct CommonComparisonCategoryImpl {
        using type = Conditional<SameAs<A, std::partial_ordering> || SameAs<B, std::partial_ordering>, std::partial_ordering,
                                 typename Conditional<SameAs<A, std::weak_ordering> || SameAs<B, std::weak_ordering>, std::weak_ordering,
                                                      std::strong_ordering>::type>::type;
    };

    template<ComparisonCategory... Categories>
    struct CommonComparisonCategoryHelper;

    template<ComparisonCategory Acc, ComparisonCategory... Rest>
    struct CommonComparisonCategoryHelper<Acc, Rest...> {
        using type = CommonComparisonCategoryImpl<Acc, typename CommonComparisonCategoryHelper<Rest...>::type>::type;
    };

    template<ComparisonCategory Acc>
    struct CommonComparisonCategoryHelper<Acc> {
        using type = Acc;
    };
}

template<ComparisonCategory... Categories>
using CommonComparisonCategory = Detail::CommonComparisonCategoryHelper<std::strong_ordering, Categories...>::type;

struct Less {
    template<typename T, ComparableWith<T> U>
    constexpr bool operator()(const T& a, const U& b) const {
        return a < b;
    }
};

struct Greater {
    template<typename T, ComparableWith<T> U>
    constexpr bool operator()(const T& a, const U& b) const {
        return a > b;
    }
};

struct Equal {
    template<typename T, EqualComparableWith<T> U>
    constexpr bool operator()(const T& a, const U& b) const {
        return a == b;
    }
};

struct CompareThreeWay {
    template<typename T, ComparableWith<T> U>
    constexpr auto operator()(const T& a, const U& b) const {
        return a <=> b;
    }
};

struct CompareThreeWayBackwards {
    template<typename T, ComparableWith<T> U>
    constexpr auto operator()(const T& a, const U& b) const {
        return b <=> a;
    }
};
}

using LIIM::CommonComparisonCategory;
using LIIM::Comparable;
using LIIM::ComparableWith;
using LIIM::ComparatorFor;
using LIIM::CompareThreeWay;
using LIIM::CompareThreeWayBackwards;
using LIIM::ComparisonCategory;
using LIIM::Equal;
using LIIM::EqualComparable;
using LIIM::EqualComparableWith;
using LIIM::Greater;
using LIIM::Less;
using LIIM::ThreeWayComparatorFor;
using LIIM::ThreeWayCompareResult;
