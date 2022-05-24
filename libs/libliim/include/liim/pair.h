#pragma once

#include <liim/utilities.h>

namespace LIIM {
template<typename T, typename U>
struct Pair {
    T first {};
    U second {};

    constexpr Pair() : first(), second() {}
    constexpr Pair(const Pair&) = default;
    constexpr Pair(Pair&&) = default;

    constexpr Pair(const T& a, const U& b) : first(a), second(b) {}
    constexpr Pair(T&& a, U&& b) : first(move(a)), second(move(b)) {}

    template<typename X = T, typename Y = U>
    constexpr Pair(X&& x, Y&& y) : first(forward<X>(x)), second(forward<Y>(y)) {}

    template<typename X = T, typename Y = U>
    constexpr Pair(const Pair<X, Y>& other) : first(other.first), second(other.second) {}

    template<typename X = T, typename Y = U>
    constexpr Pair(Pair<X, Y>&& other) : first(move(other.first)), second(move(other.second)) {}

    constexpr Pair& operator=(const Pair&) = default;
    constexpr Pair& operator=(Pair&&) = default;

    template<typename X, typename Y>
    constexpr Pair& operator=(const Pair<X, Y>& other) {
        this->first = other.first;
        this->second = other.second;
    }

    template<typename X, typename Y>
    constexpr Pair& operator=(Pair<X, Y>&& other) {
        this->first = move(other.first);
        this->second = move(other.second);
    }

    constexpr void swap(Pair& other) {
        LIIM::swap(this->first, other.first);
        LIIM::swap(this->second, other.second);
    }

    constexpr bool operator==(const Pair&) const = default;
    constexpr auto operator<=>(const Pair&) const = default;
};

template<class T, class U>
Pair(T, U) -> Pair<T, U>;

template<typename T, typename U>
constexpr Pair<decay_t<T>, decay_t<U>> make_pair(T&& t, U&& u) {
    return Pair<decay_t<T>, decay_t<U>>(forward<T>(t), forward<U>(u));
}
}

using LIIM::make_pair;
using LIIM::Pair;
