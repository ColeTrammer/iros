#pragma once

#include <di/concepts/const_lvalue_refernece.h>
#include <di/concepts/constructible_from.h>
#include <di/concepts/copy_assignable.h>
#include <di/concepts/copy_constructible.h>
#include <di/concepts/decay_same_as.h>
#include <di/concepts/move_assignable.h>
#include <di/concepts/move_constructible.h>
#include <di/concepts/mutable_rvalue_reference.h>
#include <di/meta/add_member_get.h>
#include <di/meta/index_sequence_for.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/type_list.h>
#include <di/util/forward_as_base.h>
#include <di/util/get_in_place.h>
#include <di/util/swap.h>
#include <di/vocab/tuple/tuple_element.h>
#include <di/vocab/tuple/tuple_impl.h>
#include <di/vocab/tuple/tuple_size.h>

namespace di::vocab::tuple {
template<typename... Types>
class Tuple
    : public TupleImpl<meta::IndexSequenceFor<Types...>, Types...>
    , public meta::AddMemberGet<Tuple<Types...>> {
private:
    using Base = TupleImpl<meta::IndexSequenceFor<Types...>, Types...>;

public:
    constexpr Tuple()
    requires(concepts::Conjunction<concepts::DefaultConstructible<Types>...>)
    {}

    constexpr Tuple(Tuple const&) = default;
    constexpr Tuple(Tuple&&) = default;

    constexpr Tuple(Types const&... args)
    requires(sizeof...(Types) > 0 && concepts::Conjunction<concepts::CopyConstructible<Types>...>)
        : Base(construct_tuple_impl_valuewise, args...) {}

    template<typename... Args>
    requires(sizeof...(Types) == sizeof...(Args) && sizeof...(Types) > 0 &&
             concepts::Conjunction<concepts::ConstructibleFrom<Types, Args>...>)
    constexpr Tuple(Args&&... args) : Base(construct_tuple_impl_valuewise, util::forward<Args>(args)...) {}

    template<typename Tup>
    requires(concepts::ConstructibleFrom<Base, ConstructTupleImplFromTuplelike, Tup> && !concepts::DecaySameAs<Tuple, Tup>)
    constexpr Tuple(Tup&& value) : Base(construct_tuple_impl_from_tuplelike, util::forward<Tup>(value)) {}

    constexpr ~Tuple() = default;

    constexpr Tuple& operator=(Tuple const& other)
    requires(concepts::Conjunction<concepts::CopyAssignable<Types>...>)
    {
        Base::static_assign(util::forward_as_base<Tuple&, Base>(*this), other);
        return *this;
    }

    constexpr Tuple const& operator=(Tuple const& other) const
    requires(concepts::Conjunction<concepts::CopyAssignable<Types const>...>)
    {
        Base::static_assign(util::forward_as_base<Tuple const&, Base>(*this), other);
        return *this;
    }

    constexpr Tuple& operator=(Tuple&& other)
    requires(concepts::Conjunction<concepts::MoveAssignable<Types>...>)
    {
        Base::static_assign(util::forward_as_base<Tuple&, Base>(*this), util::move(other));
        return *this;
    }

    constexpr Tuple const& operator=(Tuple&& other) const
    requires(concepts::Conjunction<concepts::AssignableFrom<Types const&, Types>...>)
    {
        Base::static_assign(util::forward_as_base<Tuple const&, Base>(*this), util::move(other));
        return *this;
    }

    template<concepts::TupleLike Tup>
    requires(!concepts::DecaySameAs<Tup, Tuple> &&
             requires(Base & self, Tup&& other) { Base::static_assign(self, util::forward<Tup>(other)); })
    constexpr Tuple& operator=(Tup&& other) {
        Base::static_assign(util::forward_as_base<Tuple&, Base>(*this), util::forward<Tup>(other));
        return *this;
    }

    template<concepts::TupleLike Tup>
    requires(!concepts::DecaySameAs<Tup, Tuple> &&
             requires(Base const& self, Tup&& other) { Base::static_assign(self, util::forward<Tup>(other)); })
    constexpr Tuple const& operator=(Tup&& other) {
        Base::static_assign(util::forward_as_base<Tuple const&, Base>(*this), util::forward<Tup>(other));
        return *this;
    }

private:
    constexpr friend types::size_t tag_invoke(types::Tag<tuple_size>, types::InPlaceType<Tuple>) { return sizeof...(Types); }

    template<types::size_t index>
    constexpr friend meta::TypeList<Types...>::TypeAtIndex<index> tag_invoke(types::Tag<tuple_element>, types::InPlaceType<Tuple>,
                                                                             types::InPlaceIndex<index>);

    template<types::size_t index, concepts::DecaySameAs<Tuple> Self>
    requires(index < sizeof...(Types))
    constexpr friend meta::Like<Self, meta::TupleElement<Self, index>>&& tag_invoke(types::Tag<util::get_in_place>,
                                                                                    types::InPlaceIndex<index>, Self&& self) {
        using Impl = detail::TupleImplBase<index, meta::IndexSequenceFor<Types...>, Types...>::Type;
        return static_cast<meta::Like<Self, meta::TupleElement<Self, index>>&&>(Impl::static_get(util::forward_as_base<Self, Impl>(self)));
    }
};

template<typename... Types>
Tuple(Types...) -> Tuple<Types...>;
}
