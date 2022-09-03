#pragma once

#include <di/concepts/conjunction.h>
#include <di/concepts/decay_same_as.h>
#include <di/concepts/default_constructible.h>
#include <di/meta/index_sequence.h>
#include <di/meta/like.h>
#include <di/types/in_place_index.h>
#include <di/util/declval.h>
#include <di/util/forward_as_base.h>
#include <di/util/forward_like.h>
#include <di/vocab/tuple/tuple_like.h>

namespace di::vocab::tuple {
struct ConstructTupleImplValuewise {};

constexpr inline auto construct_tuple_impl_valuewise = ConstructTupleImplValuewise {};

struct ConstructTupleImplFromTuplelike {};

constexpr inline auto construct_tuple_impl_from_tuplelike = ConstructTupleImplFromTuplelike {};

template<typename Indices, typename... Types>
class TupleImpl;

template<types::size_t index, types::size_t... indices, typename T, typename... Rest>
class TupleImpl<meta::IndexSequence<index, indices...>, T, Rest...> : public TupleImpl<meta::IndexSequence<indices...>, Rest...> {
private:
    using Base = TupleImpl<meta::IndexSequence<indices...>, Rest...>;

public:
    constexpr TupleImpl()
    requires(concepts::Conjunction<concepts::DefaultConstructible<T>, concepts::DefaultConstructible<Rest>...>)
        : m_value() {}

    constexpr TupleImpl(TupleImpl const&) = default;
    constexpr TupleImpl(TupleImpl&&) = default;

    template<typename Arg, typename... Args>
    constexpr TupleImpl(ConstructTupleImplValuewise, Arg&& arg, Args&&... args)
        : Base(construct_tuple_impl_valuewise, util::forward<Args>(args)...), m_value(util::forward<Arg>(arg)) {}

    template<concepts::TupleLike Tup>
    requires(sizeof...(Rest) + 1 == meta::TupleSize<Tup> &&
             concepts::ConstructibleFrom<T, decltype(util::get<index>(util::declval<Tup>()))> &&
             concepts::Conjunction<concepts::ConstructibleFrom<Rest, decltype(util::get<indices>(util::declval<Tup>()))>...>)
    constexpr TupleImpl(ConstructTupleImplFromTuplelike, Tup&& tuple)
        : Base(construct_tuple_impl_valuewise, util::get<indices>(util::forward<Tup>(tuple))...)
        , m_value(util::get<index>(util::forward<Tup>(tuple))) {}

    constexpr ~TupleImpl() = default;

    constexpr TupleImpl& operator=(TupleImpl const&) = default;
    constexpr TupleImpl& operator=(TupleImpl&&) = default;

protected:
    template<types::size_t index_to_lookup, concepts::DecaySameAs<TupleImpl> Self>
    requires(index_to_lookup <= sizeof...(Rest))
    constexpr static decltype(auto) static_get(types::InPlaceIndex<index_to_lookup>, Self&& self) {
        if constexpr (index_to_lookup == 0) {
            return (util::forward_like<Self>(self.m_value));
        } else {
            return Base::static_get(types::in_place_index<index_to_lookup - 1>, util::forward_as_base<Self, Base>(self));
        }
    }

    template<concepts::DecaySameAs<TupleImpl> Self, concepts::TupleLike Tup>
    requires(
        sizeof...(Rest) + 1 == meta::TupleSize<Tup> && (concepts::ConstLValueReference<Tup> || concepts::MutableRValueReference<Tup&&>) &&
        concepts::AssignableFrom<meta::Like<Self, T>, meta::Like<Tup, meta::TupleElement<Tup, index>> &&> &&
        concepts::Conjunction<concepts::AssignableFrom<meta::Like<Self, Rest>, meta::Like<Tup, meta::TupleElement<Tup, indices>> &&>...>)
    constexpr static void static_assign(Self&& self, Tup&& other) {
        self.m_value = util::get<index>(util::forward<Tup>(other));
        Base::static_assign_unchecked(util::forward_as_base<Self, Base>(self), util::forward<Tup>(other));
    }

    template<typename Self, typename Tup>
    constexpr static void static_assign_unchecked(Self&& self, Tup&& other) {
        self.m_value = util::get<index>(util::forward<Tup>(other));
        Base::static_assign_unchecked(util::forward_as_base<Self, Base>(self), util::forward<Tup>(other));
    }

    [[no_unique_address]] T m_value;
};

template<>
class TupleImpl<meta::IndexSequence<>> {
public:
    constexpr TupleImpl() = default;
    constexpr TupleImpl(TupleImpl const&) = default;
    constexpr TupleImpl(TupleImpl&&) = default;

    constexpr TupleImpl(ConstructTupleImplValuewise) {}

    constexpr ~TupleImpl() = default;

    template<concepts::TupleLike T>
    requires(meta::TupleSize<T> == 0)
    constexpr TupleImpl(ConstructTupleImplFromTuplelike, T&&) {}

    constexpr TupleImpl& operator=(TupleImpl const&) = default;
    constexpr TupleImpl& operator=(TupleImpl&&) = default;

protected:
    template<concepts::DecaySameAs<TupleImpl> Self, concepts::TupleLike Tup>
    requires(meta::TupleSize<Tup> == 0 && (concepts::ConstLValueReference<Tup> || concepts::MutableRValueReference<Tup&&>) )
    constexpr static void static_assign(Self&&, Tup&&) {}

    template<typename Self, typename Tup>
    constexpr static void static_assign_unchecked(Self&&, Tup&&) {}
};
}
