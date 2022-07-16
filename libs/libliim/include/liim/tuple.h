#pragma once

#include <liim/compare.h>
#include <liim/construct.h>
#include <liim/result.h>
#include <liim/type_list.h>

namespace LIIM {
template<typename T>
concept TupleLike = requires {
    std::tuple_size<decay_t<T>>::value;
};

template<size_t index, TupleLike T>
using TupleElement = typename std::tuple_element<index, decay_t<T>>::type;

template<size_t index, TupleLike T>
using DecayTupleElement = decay_t<TupleElement<index, T>>;

template<TupleLike T>
constexpr inline auto tuple_size = std::tuple_size<decay_t<T>>::value;

template<typename T>
concept PairLike = TupleLike<T> &&(tuple_size<T> == 2);

template<typename... Types>
class Tuple;

template<size_t index, typename Tuple>
constexpr Like<Tuple, TupleElement<index, Tuple>> tuple_get(Tuple&& tuple) {
    return forward<Tuple>(tuple).template get<index>();
}

template<typename C, TupleLike Tuple>
constexpr void tuple_visit(Tuple&& tuple, C&& visitor) {
    auto helper = [&]<size_t... indices>(IndexSequence<indices...>) {
        (invoke(forward<C>(visitor), tuple_get<indices>(forward<Tuple>(tuple))), ...);
    };
    helper(make_index_sequence<tuple_size<Tuple>>());
}

template<typename C, TupleLike Tuple>
constexpr decltype(auto) tuple_apply(C&& callable, Tuple&& tuple) {
    auto helper = [&]<size_t... indices>(IndexSequence<indices...>)->decltype(auto) {
        return invoke(forward<C>(callable), tuple_get<indices>(forward<Tuple>(tuple))...);
    };
    return helper(make_index_sequence<tuple_size<Tuple>>());
}

template<typename T, TupleLike Tuple>
constexpr auto make_from_tuple(Tuple&& tuple) {
    auto helper = [&]<size_t... indices>(IndexSequence<indices...>)->decltype(auto) {
        return create<T>(tuple_get<indices>(forward<Tuple>(tuple))...);
    };
    return helper(make_index_sequence<tuple_size<Tuple>>());
}

namespace Detail {
    template<typename T, TupleLike Tuple>
    constexpr inline bool constructible_from_tuple_args = ([]<size_t... indices>(IndexSequence<indices...>) {
        return ConstructibleFrom<T, Like<Tuple, TupleElement<indices, Tuple>>...>;
    })(make_index_sequence<tuple_size<Tuple>>());

    template<typename T, TupleLike Tuple>
    constexpr inline bool createable_from_tuple_args = ([]<size_t... indices>(IndexSequence<indices...>) {
        return CreateableFrom<T, Like<Tuple, TupleElement<indices, Tuple>>...> ||
               FalliblyCreateableFrom<T, Like<Tuple, TupleElement<indices, Tuple>>...>;
    })(make_index_sequence<tuple_size<Tuple>>());

    enum class TupleSatisfiesOp {
        Equal,
        Compare,
        Constructible,
        InPlaceConstructible,
        Createable,
        InPlaceCreateable,
    };

    template<TupleSatisfiesOp op, TupleLike TupleA, TupleLike TupleB>
    constexpr inline bool tuple_pairwise_satisified =
        tuple_size<TupleA> == tuple_size<TupleB> && ([]<size_t... indices>(IndexSequence<indices...>) {
            if constexpr (op == TupleSatisfiesOp::Equal) {
                return ((EqualComparableWith<TupleElement<indices, TupleA>, TupleElement<indices, TupleB>>) &&...);
            } else if constexpr (op == TupleSatisfiesOp::Compare) {
                return ((ComparableWith<TupleElement<indices, TupleA>, TupleElement<indices, TupleB>>) &&...);
            } else if constexpr (op == TupleSatisfiesOp::Constructible) {
                return (
                    (ConstructibleFrom<Like<TupleA, TupleElement<indices, TupleA>>, Like<TupleB, TupleElement<indices, TupleB>>>) &&...);
            } else if constexpr (op == TupleSatisfiesOp::InPlaceConstructible) {
                if constexpr (((TupleLike<TupleElement<indices, TupleB>>) &&...)) {
                    return ((constructible_from_tuple_args<TupleElement<indices, TupleA>, TupleElement<indices, TupleB>>) &&...);
                } else {
                    return false;
                }
            } else if constexpr (op == TupleSatisfiesOp::Createable) {
                return ((CreateableFrom<Like<TupleA, TupleElement<indices, TupleA>>, Like<TupleB, TupleElement<indices, TupleB>>> ||
                         FalliblyCreateableFrom<Like<TupleA, TupleElement<indices, TupleA>>,
                                                Like<TupleB, TupleElement<indices, TupleB>>>) &&...);
            } else if constexpr (op == TupleSatisfiesOp::InPlaceCreateable) {
                if constexpr (((TupleLike<TupleElement<indices, TupleB>>) &&...)) {
                    return ((createable_from_tuple_args<TupleElement<indices, TupleA>, TupleElement<indices, TupleB>>) &&...);
                } else {
                    return false;
                }
            }
        })(make_index_sequence<tuple_size<TupleA>>());
}

#define __LIIM_TUPLE_FORWARD_GET             \
    template<size_t index>                   \
    constexpr decltype(auto) get()& {        \
        return get_impl<index>(*this);       \
    }                                        \
    template<size_t index>                   \
    constexpr decltype(auto) get() const& {  \
        return get_impl<index>(*this);       \
    }                                        \
    template<size_t index>                   \
    constexpr decltype(auto) get()&& {       \
        return get_impl<index>(move(*this)); \
    }                                        \
    template<size_t index>                   \
    constexpr decltype(auto) get() const&& { \
        return get_impl<index>(move(*this)); \
    }

template<typename T, typename... Rest>
class Tuple<T, Rest...> {
public:
    template<TupleLike Tup>
    requires(Detail::tuple_pairwise_satisified<Detail::TupleSatisfiesOp::Createable, Tuple, Tup>) constexpr static auto create(
        Tup&& tuple) {
        auto helper = [&]<size_t... indices>(IndexSequence<indices...>)->decltype(auto) {
            return Tuple::create(tuple_get<indices>(forward<Tup>(tuple))...);
        };
        return helper(make_index_sequence<tuple_size<Tup>>());
    }

    template<typename Th, typename... Ts>
    requires(Detail::tuple_pairwise_satisified<Detail::TupleSatisfiesOp::Createable, Tuple, Tuple<Th, Ts...>>) constexpr static auto create(
        Th&& value, Ts&&... rest) {
        return result_and_then(LIIM::create<T>(forward<Th>(value)), [&](T&& val) {
            return result_and_then(Tuple<Rest...>::create(forward<Ts>(rest)...), [&](Tuple<Rest...>&& rs) {
                return Tuple(move(val), move(rs));
            });
        });
    }

    template<TupleLike Tup, TupleLike... Tuples>
    requires(Detail::tuple_pairwise_satisified<Detail::TupleSatisfiesOp::InPlaceCreateable, Tuple,
                                               Tuple<Tup, Tuples...>>) constexpr static auto create(piecewise_construct_t, Tup tuple,
                                                                                                    Tuples... rest) {
        return result_and_then(make_from_tuple<T>(move(tuple)), [&](T&& val) {
            return result_and_then(Tuple<Rest...>::create(piecewise_construct, move(rest)...), [&](Tuple<Rest...>&& rs) {
                return Tuple(move(val), move(rs));
            });
        });
    }

    constexpr Tuple() = default;
    constexpr Tuple(const Tuple&) = default;
    constexpr Tuple(Tuple&&) = default;

    template<typename Th, typename... Ts>
    requires(Detail::tuple_pairwise_satisified<Detail::TupleSatisfiesOp::Constructible, Tuple, Tuple<Th, Ts...>>) constexpr Tuple(
        Th&& value, Ts&&... rest)
        : m_value(forward<Th>(value)), m_rest(forward<Ts>(rest)...) {}

    template<TupleLike Tup>
    requires(Detail::tuple_pairwise_satisified<Detail::TupleSatisfiesOp::Constructible, Tuple, Tup>) constexpr Tuple(Tup&& values)
        : Tuple(make_index_sequence<tuple_size<Tup>>(), forward<Tup>(values)) {}

    template<TupleLike Tup, TupleLike... Tuples>
    requires(Detail::tuple_pairwise_satisified<Detail::TupleSatisfiesOp::InPlaceConstructible, Tuple,
                                               Tuple<Tup, Tuples...>>) constexpr Tuple(piecewise_construct_t, Tup tuple, Tuples... rest)
        : Tuple(piecewise_construct, make_index_sequence<tuple_size<Tup>>(), move(tuple), move(rest)...) {}

    constexpr Tuple& operator=(const Tuple&) = default;
    constexpr Tuple& operator=(Tuple&&) = default;

    __LIIM_TUPLE_FORWARD_GET

    template<TupleLike Tup>
    requires(Detail::tuple_pairwise_satisified<Detail::TupleSatisfiesOp::Equal, Tuple, Tup>) constexpr bool
    operator==(const Tup& other) const {
        auto helper = [&]<size_t... indices>(IndexSequence<indices...>)->bool {
            return ((tuple_get<indices>(*this) == tuple_get<indices>(other)) && ...);
        };
        return helper(make_index_sequence<tuple_size<Tup>>());
    }

private:
    template<TupleLike Tup, size_t... indices>
    constexpr Tuple(IndexSequence<indices...>, Tup&& value) : Tuple(tuple_get<indices>(forward<Tup>(value))...) {}

    template<TupleLike Tup, TupleLike... Tuples, size_t... indices>
    constexpr Tuple(piecewise_construct_t, IndexSequence<indices...>, Tup tuple, Tuples... rest)
        : m_value(tuple_get<indices>(move(tuple))...), m_rest(piecewise_construct, move(rest)...) {}

    constexpr Tuple(T&& value, Tuple<Rest...>&& rest) : m_value(move(value)), m_rest(move(rest)) {}

    template<size_t index, typename Tup>
    constexpr static Like<Tup, TupleElement<index, Tuple>> get_impl(Tup&& tuple) {
        static_assert(index <= sizeof...(Rest), "Tuple index too high.");
        using R = Like<Tup, TupleElement<index, Tuple>>;
        if constexpr (index == 0) {
            return static_cast<R>(forward<Tup>(tuple).m_value);
        } else {
            return static_cast<R>(forward<Tup>(tuple).m_rest.template get<index - 1>());
        }
    }

    T m_value {};
    Tuple<Rest...> m_rest {};
};

template<>
class Tuple<> {
public:
    constexpr static Tuple create() { return Tuple(); }

    template<TupleLike Tup>
    requires(tuple_size<Tup> == 0) constexpr static Tuple create(Tup&&) { return Tuple(); }

    constexpr static Tuple create(piecewise_construct_t) { return Tuple(); }

    constexpr Tuple() = default;

    template<TupleLike Tup>
    requires(tuple_size<Tup> == 0) constexpr Tuple(Tup&&) {}

    constexpr Tuple(piecewise_construct_t) {}

    __LIIM_TUPLE_FORWARD_GET

    template<TupleLike Tup>
    requires(tuple_size<Tup> == 0) constexpr bool operator==(const Tup&) const { return true; }

private:
    template<size_t index, typename Tup>
    constexpr static void get_impl(Tup&&) {
        static_assert(always_false<index>, "Tuple index too high.");
    }
};

#undef __LIIM_TUPLE_FORWARD_GET

template<class... Types>
Tuple(Types...) -> Tuple<Types...>;

template<typename... Args>
constexpr Tuple<decay_t<Args>...> make_tuple(Args&&... args) {
    return Tuple<decay_t<Args>...>(forward<Args>(args)...);
}

template<typename... Args>
constexpr Tuple<Args&&...> forward_as_tuple(Args&&... args) {
    return Tuple<Args&&...>(forward<Args>(args)...);
}

template<typename C, typename Tuple>
constexpr auto tuple_map(Tuple&& tuple, C&& mapper) {
    auto helper = [&]<size_t... indices>(IndexSequence<indices...>)->decltype(auto) {
        using ReturnTuple = LIIM::Tuple<decltype(mapper(forward<Tuple>(tuple).template get<indices>()))...>;
        return ReturnTuple(mapper(forward<Tuple>(tuple).template get<indices>())...);
    };
    return helper(make_index_sequence<tuple_size<Tuple>>());
}
};

namespace std {
template<typename... Types>
struct tuple_size<LIIM::Tuple<Types...>> {
    static constexpr size_t value = LIIM::TypeList::Count<Types...>::value;
};

template<size_t index, typename... Types>
struct tuple_element<index, LIIM::Tuple<Types...>> {
    using type = LIIM::TypeList::TypeAtIndex<index, Types...>::type;
};
}

using LIIM::DecayTupleElement;
using LIIM::forward_as_tuple;
using LIIM::make_from_tuple;
using LIIM::make_tuple;
using LIIM::Tuple;
using LIIM::tuple_apply;
using LIIM::tuple_map;
using LIIM::tuple_size;
using LIIM::TupleElement;
