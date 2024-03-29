#pragma once

#include <di/function/unpack.h>
#include <di/function/ycombinator.h>
#include <di/meta/algorithm.h>
#include <di/meta/common.h>
#include <di/meta/compare.h>
#include <di/meta/constexpr.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/list.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/util/add_member_get.h>
#include <di/util/forward_as_base.h>
#include <di/util/get_in_place.h>
#include <di/util/swap.h>
#include <di/vocab/tuple/tuple_element.h>
#include <di/vocab/tuple/tuple_elements.h>
#include <di/vocab/tuple/tuple_impl.h>
#include <di/vocab/tuple/tuple_size.h>

namespace di::vocab {
template<typename... Types>
class Tuple
    : public TupleImpl<meta::IndexSequenceFor<Types...>, Types...>
    , public util::AddMemberGet<Tuple<Types...>> {
private:
    using Base = TupleImpl<meta::IndexSequenceFor<Types...>, Types...>;

public:
    constexpr Tuple()
    requires(concepts::DefaultConstructible<Types> && ...)
    {}

    constexpr Tuple(Tuple const&) = default;
    constexpr Tuple(Tuple&&) = default;

    constexpr Tuple(Types const&... args)
    requires(sizeof...(Types) > 0 && (concepts::CopyConstructible<Types> && ...))
        : Base(construct_tuple_impl_valuewise, args...) {}

    template<typename... Args>
    requires(sizeof...(Types) == sizeof...(Args) && sizeof...(Types) > 0 &&
             (concepts::ConstructibleFrom<Types, Args> && ...))
    constexpr Tuple(Args&&... args) : Base(construct_tuple_impl_valuewise, util::forward<Args>(args)...) {}

    template<typename Tup>
    requires(!concepts::DecaySameAs<Tuple, Tup> &&
             concepts::ConstructibleFrom<Base, ConstructTupleImplFromTuplelike, Tup>)
    constexpr Tuple(Tup&& value) : Base(construct_tuple_impl_from_tuplelike, util::forward<Tup>(value)) {}

    constexpr ~Tuple() = default;

    constexpr Tuple& operator=(Tuple const& other)
    requires(concepts::CopyAssignable<Types> && ...)
    {
        Base::static_assign(util::forward_as_base<Tuple&, Base>(*this), other);
        return *this;
    }

    constexpr Tuple const& operator=(Tuple const& other) const
    requires(concepts::CopyAssignable<Types const> && ...)
    {
        Base::static_assign(util::forward_as_base<Tuple const&, Base>(*this), other);
        return *this;
    }

    constexpr Tuple& operator=(Tuple&& other)
    requires(concepts::MoveAssignable<Types> && ...)
    {
        Base::static_assign(util::forward_as_base<Tuple&, Base>(*this), util::move(other));
        return *this;
    }

    constexpr Tuple const& operator=(Tuple&& other) const
    requires(concepts::AssignableFrom<Types const&, Types> && ...)
    {
        Base::static_assign(util::forward_as_base<Tuple const&, Base>(*this), util::move(other));
        return *this;
    }

    template<concepts::TupleLike Tup>
    requires(!concepts::DecaySameAs<Tup, Tuple> &&
             requires(Base& self, Tup&& other) { Base::static_assign(self, util::forward<Tup>(other)); })
    constexpr Tuple& operator=(Tup&& other) {
        Base::static_assign(util::forward_as_base<Tuple&, Base>(*this), util::forward<Tup>(other));
        return *this;
    }

    template<concepts::TupleLike Tup>
    requires(!concepts::DecaySameAs<Tup, Tuple> &&
             requires(Base const& self, Tup&& other) { Base::static_assign(self, util::forward<Tup>(other)); })
    constexpr Tuple const& operator=(Tup&& other) const {
        Base::static_assign(util::forward_as_base<Tuple const&, Base>(*this), util::forward<Tup>(other));
        return *this;
    }

private:
    template<typename... Other>
    requires(sizeof...(Types) == sizeof...(Other) &&
             requires { requires(concepts::EqualityComparableWith<Types, Other> && ...); })
    constexpr friend bool operator==(Tuple const& a, Tuple<Other...> const& b) {
        return function::unpack<meta::MakeIndexSequence<sizeof...(Types)>>(
            [&]<size_t... indices>(meta::ListV<indices...>) {
                return ((util::get<indices>(a) == util::get<indices>(b)) && ...);
            });
    }

    template<typename... Other>
    requires(sizeof...(Types) == sizeof...(Other) &&
             requires { requires(concepts::ThreeWayComparableWith<Types, Other> && ...); })
    constexpr friend meta::CommonComparisonCategory<meta::CompareThreeWayResult<Types, Other>...>
    operator<=>(Tuple const& a, Tuple<Other...> const& b) {
        if constexpr (sizeof...(Types) == 0) {
            return di::strong_ordering::equal;
        } else {
            auto process = function::ycombinator([&]<size_t index>(auto& self, Constexpr<index>) {
                if (auto result = util::get<index>(a) <=> util::get<index>(b); result != 0) {
                    return result;
                }
                if constexpr (index == sizeof...(Types) - 1) {
                    return di::strong_ordering::equal;
                } else {
                    return self(c_<index + 1>);
                }
            });
            return process(c_<0zu>);
        }
    }

    template<concepts::DerivedFrom<Tuple> Self = Tuple>
    constexpr friend types::size_t tag_invoke(types::Tag<tuple_size>, types::InPlaceType<Self>) {
        return sizeof...(Types);
    }

    using TypeList = meta::List<Types...>;

    template<types::size_t index, concepts::DerivedFrom<Tuple> Self = Tuple>
    constexpr friend InPlaceType<meta::At<TypeList, index>> tag_invoke(types::Tag<tuple_element>,
                                                                       types::InPlaceType<Self>, Constexpr<index>) {
        return {};
    }

    template<types::size_t index, concepts::DerivedFrom<Tuple> Self = Tuple>
    constexpr friend InPlaceType<meta::At<TypeList, index> const>
    tag_invoke(types::Tag<tuple_element>, types::InPlaceType<Self const>, Constexpr<index>) {
        return {};
    }

    template<types::size_t index, typename Self>
    requires(index < sizeof...(Types) && concepts::DerivedFrom<meta::Decay<Self>, Tuple>)
    constexpr friend meta::Like<Self, meta::TupleElement<Self, index>>&& tag_invoke(types::Tag<util::get_in_place>,
                                                                                    Constexpr<index>, Self&& self) {
        using Impl = detail::TupleImplBase<index, meta::IndexSequenceFor<Types...>, Types...>::Type;
        return static_cast<meta::Like<Self, meta::TupleElement<Self, index>>&&>(
            Impl::static_get(util::forward_as_base<Self, Impl>(self)));
    }

    template<typename T, typename Self = Tuple>
    requires(concepts::DerivedFrom<meta::RemoveCVRef<Self>, Tuple> && meta::UniqueType<T, meta::List<Types...>>)
    constexpr friend meta::Like<Self, T>&& tag_invoke(types::Tag<util::get_in_place>, InPlaceType<T>, Self&& self) {
        constexpr auto index = meta::Lookup<T, meta::List<Types...>>;
        using Impl = detail::TupleImplBase<index, meta::IndexSequenceFor<Types...>, Types...>::Type;
        return static_cast<meta::Like<Self, meta::TupleElement<Self, index>>&&>(
            Impl::static_get(util::forward_as_base<Self, Impl>(self)));
    }
};

template<typename... Types>
Tuple(Types...) -> Tuple<Types...>;
}

namespace di {
namespace detail {
    template<typename...>
    constexpr inline bool has_common_type_helper = false;

    template<typename T, concepts::CommonWith<T> U>
    constexpr inline bool has_common_type_helper<T, U> = true;

    struct HasCommonType {
        template<typename... Types>
        using Invoke = Constexpr<has_common_type_helper<Types...>>;
    };

    template<typename...>
    constexpr inline bool has_common_reference_helper = false;

    template<typename T, concepts::CommonWith<T> U>
    constexpr inline bool has_common_reference_helper<T, U> = true;

    struct HasCommonReference {
        template<typename... Types>
        using Invoke = Constexpr<has_common_reference_helper<Types...>>;
    };
}

template<concepts::TupleLike A, concepts::TupleLike B>
requires((concepts::InstanceOf<A, vocab::Tuple> || concepts::InstanceOf<B, vocab::Tuple>) &&
         (meta::TupleSize<A> == meta::TupleSize<B>) &&
         meta::All<meta::Zip<meta::TupleElements<A>, meta::TupleElements<B>>, meta::Uncurry<detail::HasCommonType>>)
struct meta::CustomCommonType<A, B> {
    using Type = meta::AsTuple<meta::Transform<meta::Zip<meta::TupleElements<A>, meta::TupleElements<B>>,
                                               meta::Uncurry<meta::Quote<meta::CommonType>>>>;
};

template<concepts::TupleLike A, concepts::TupleLike B, template<typename> typename AQual,
         template<typename> typename BQual>
requires((concepts::InstanceOf<A, vocab::Tuple> || concepts::InstanceOf<B, vocab::Tuple>) &&
         (meta::TupleSize<A> == meta::TupleSize<B>) &&
         meta::All<meta::Zip<meta::Transform<meta::TupleElements<A>, meta::Quote<AQual>>,
                             meta::Transform<meta::TupleElements<B>, meta::Quote<BQual>>>,
                   meta::Uncurry<detail::HasCommonType>>)
struct meta::CustomCommonReference<A, B, AQual, BQual> {
    using Type = meta::AsTuple<meta::Transform<meta::Zip<meta::Transform<meta::TupleElements<A>, meta::Quote<AQual>>,
                                                         meta::Transform<meta::TupleElements<B>, meta::Quote<BQual>>>,
                                               meta::Uncurry<meta::Quote<meta::CommonReference>>>>;
};
}
