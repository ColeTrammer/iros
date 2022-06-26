#pragma once

#include <liim/construct.h>
#include <liim/result.h>
#include <liim/tuple.h>
#include <liim/utilities.h>

namespace LIIM {
template<typename T, typename U>
struct Pair {
    T first {};
    U second {};

    using FirstType = T;
    using SecondType = U;

    constexpr Pair() : first(), second() {}
    constexpr Pair(const Pair&) = default;
    constexpr Pair(Pair&&) = default;

    constexpr Pair clone() const requires(Cloneable<T>&& Cloneable<U>) { return Pair(::clone(first), ::clone(second)); }

    template<typename... TArgs, typename... UArgs>
    constexpr static auto create(piecewise_construct_t, Tuple<TArgs...>&& first_args, Tuple<UArgs...>&& second_args) {
        return Pair::create(move(first_args), move(second_args), make_index_sequence<sizeof...(TArgs)>(),
                            make_index_sequence<sizeof...(UArgs)>());
    }

    template<typename X = T, typename Y = U>
    constexpr Pair(X&& x, Y&& y) : first(forward<X>(x)), second(forward<Y>(y)) {}

    template<typename X = T, typename Y = U>
    constexpr Pair(const Pair<X, Y>& other) : first(other.first), second(other.second) {}

    template<typename X = T, typename Y = U>
    constexpr Pair(Pair<X, Y>&& other) : first(move(other.first)), second(move(other.second)) {}

    template<typename... TArgs, typename... UArgs>
    requires(ConstructibleFrom<T, TArgs...>, ConstructibleFrom<U, UArgs...>) constexpr Pair(piecewise_construct_t,
                                                                                            Tuple<TArgs...>&& first_args,
                                                                                            Tuple<UArgs...>&& second_args)
        : Pair(move(first_args), move(second_args), make_index_sequence<sizeof...(TArgs)>(), make_index_sequence<sizeof...(UArgs)>()) {}

    constexpr Pair& operator=(const Pair&) = default;
    constexpr Pair& operator=(Pair&&) = default;

    template<typename X, typename Y>
    constexpr Pair& operator=(const Pair<X, Y>& other) {
        this->first = other.first;
        this->second = other.second;
        return *this;
    }

    template<typename X, typename Y>
    constexpr Pair& operator=(Pair<X, Y>&& other) {
        this->first = move(other.first);
        this->second = move(other.second);
        return *this;
    }

    template<size_t index>
    constexpr typename TypeList::TypeAtIndex<index, T, U>::type& get() & {
        static_assert(index < 2, "Pair get index is too high");
        if constexpr (index == 0) {
            return first;
        } else {
            return second;
        }
    }

    template<size_t index>
    constexpr typename TypeList::TypeAtIndex<index, T, U>::type&& get() && {
        static_assert(index < 2, "Pair get index is too high");
        if constexpr (index == 0) {
            return forward<T&&>(first);
        } else {
            return forward<U&&>(second);
        }
    }

    template<size_t index>
    constexpr const typename TypeList::TypeAtIndex<index, T, U>::type& get() const {
        static_assert(index < 2, "Pair get index is too high");
        if constexpr (index == 0) {
            return first;
        } else {
            return second;
        }
    }

    template<size_t index>
    constexpr const typename TypeList::TypeAtIndex<index, T, U>::type&& get() const&& {
        static_assert(index < 2, "Pair get index is too high");
        if constexpr (index == 0) {
            return forward<const T&&>(first);
        } else {
            return forward<const U&&>(second);
        }
    }

    constexpr void swap(Pair& other) {
        LIIM::swap(this->first, other.first);
        LIIM::swap(this->second, other.second);
    }

    constexpr bool operator==(const Pair& other) const { return this->first == other.first && this->second == other.second; }
    constexpr auto operator<=>(const Pair&) const = default;

private:
    template<typename... TArgs, typename... UArgs, size_t... tindices, size_t... uindices>
    constexpr static CommonResult<Pair, CreateResult<T, TArgs...>, CreateResult<U, UArgs...>>
    create(Tuple<TArgs...>&& first_args, Tuple<UArgs...>&& second_args, IndexSequence<tindices...>, IndexSequence<uindices...>) {
        return result_and_then(LIIM::create<T>(forward<TArgs>(first_args.template get<tindices>())...), [&](auto&& first) {
            return result_and_then(LIIM::create<U>(forward<UArgs>(second_args.template get<uindices>())...), [&](auto&& second) {
                return Pair(forward<T>(first), forward<U>(second));
            });
        });
    }

    template<typename... TArgs, typename... UArgs, size_t... tindices, size_t... uindices>
    constexpr Pair(Tuple<TArgs...>&& first_args, Tuple<UArgs...>&& second_args, IndexSequence<tindices...>, IndexSequence<uindices...>)
        : first(forward<TArgs>(first_args.template get<tindices>())...), second(forward<UArgs>(second_args.template get<uindices>())...) {}
};

template<class T, class U>
Pair(T, U) -> Pair<T, U>;

template<typename T, typename U>
constexpr Pair<decay_t<T>, decay_t<U>> make_pair(T&& t, U&& u) {
    return Pair<decay_t<T>, decay_t<U>>(forward<T>(t), forward<U>(u));
}
}

namespace std {
template<typename T, typename U>
struct tuple_size<LIIM::Pair<T, U>> {
    static constexpr size_t value = 2;
};

template<size_t index, typename T, typename U>
struct tuple_element<index, LIIM::Pair<T, U>> {
    using type = LIIM::TypeList::TypeAtIndex<index, T, U>::type;
};
}

using LIIM::make_pair;
using LIIM::Pair;
