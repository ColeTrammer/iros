#pragma once

#include <di/concepts/default_constructible.h>
#include <di/concepts/derived_from.h>
#include <di/concepts/instance_of.h>
#include <di/concepts/instance_of_v.h>
#include <di/concepts/trivially_copy_assignable.h>
#include <di/concepts/trivially_copy_constructible.h>
#include <di/concepts/trivially_destructible.h>
#include <di/concepts/trivially_move_assignable.h>
#include <di/concepts/trivially_move_constructible.h>
#include <di/function/index_dispatch.h>
#include <di/math/smallest_unsigned_type.h>
#include <di/meta/add_member_get.h>
#include <di/meta/constexpr.h>
#include <di/meta/list/prelude.h>
#include <di/meta/remove_cvref.h>
#include <di/util/forward.h>
#include <di/util/get.h>
#include <di/util/initializer_list.h>
#include <di/util/move.h>
#include <di/vocab/variant/variant_alternative.h>
#include <di/vocab/variant/variant_forward_declaration.h>
#include <di/vocab/variant/variant_impl.h>
#include <di/vocab/variant/variant_index.h>
#include <di/vocab/variant/variant_size.h>
#include <di/vocab/variant/variant_types.h>

namespace di::vocab {
namespace detail {
    template<typename T, typename U, typename A = T[1]>
    concept VariantValidOverload = requires { A { util::declval<U>() }; };
}

template<typename... Types>
requires(sizeof...(Types) > 0)
class Variant : public meta::AddMemberGet<Variant<Types...>> {
private:
    using Impl = detail::VariantImpl<Types...>;
    using List = meta::List<Types...>;

    constexpr static bool trivially_copy_constructible =
        concepts::Conjunction<concepts::TriviallyCopyConstructible<Types>...>;
    constexpr static bool trivially_move_constructible =
        concepts::Conjunction<concepts::TriviallyMoveConstructible<Types>...>;
    constexpr static bool trivially_copy_assignable =
        concepts::Conjunction<concepts::TriviallyCopyAssignable<Types>...>;
    constexpr static bool trivially_move_assignable =
        concepts::Conjunction<concepts::TriviallyMoveAssignable<Types>...>;
    constexpr static bool trivially_destructible = concepts::Conjunction<concepts::TriviallyDestructible<Types>...>;

    constexpr static bool copyable = concepts::Conjunction<concepts::CopyConstructible<Types>...>;
    constexpr static bool movable = concepts::Conjunction<concepts::MoveConstructible<Types>...>;

    template<typename U, typename T>
    struct SelectorImpl {
        T operator()(T) const
        requires(detail::VariantValidOverload<T, U>);
    };

    template<typename U>
    struct Selector : SelectorImpl<U, Types>... {
        using SelectorImpl<U, Types>::operator()...;
    };

public:
    // conditionally trivial special member functions.
    Variant(Variant const&)
    requires(trivially_copy_constructible)
    = default;
    Variant(Variant&&)
    requires(trivially_move_constructible)
    = default;
    ~Variant()
    requires(trivially_destructible)
    = default;
    Variant& operator=(Variant const&)
    requires(trivially_copy_assignable)
    = default;
    Variant& operator=(Variant&&)
    requires(trivially_move_assignable)
    = default;

    constexpr Variant()
    requires(concepts::DefaultConstructible<meta::Front<List>>)
    {
        do_emplace(c_<0zu>);
    }

    constexpr Variant(Variant const& other)
    requires(copyable && !trivially_copy_constructible)
    {
        function::index_dispatch<void, meta::Size<List>>(
            other.index(),
            []<size_t index>(Constexpr<index>, Variant& self, Variant const& other) {
                self.do_emplace(c_<index>, util::get<index>(other));
            },
            *this, other);
    }

    constexpr Variant(Variant&& other)
    requires(movable && !trivially_move_constructible)
    {
        function::index_dispatch<void, meta::Size<List>>(
            other.index(),
            []<size_t index>(Constexpr<index>, Variant& self, Variant&& other) {
                self.do_emplace(c_<index>, util::get<index>(util::move(other)));
            },
            *this, util::move(other));
    }

    template<typename U>
    requires(!concepts::RemoveCVRefSameAs<U, Variant> && !concepts::InstanceOf<meta::RemoveCVRef<U>, InPlaceType> &&
             !concepts::Constexpr<meta::RemoveCVRef<U>> && concepts::Invocable<Selector<U>, U> &&
             concepts::ConstructibleFrom<meta::InvokeResult<Selector<U>, U>, U>)
    constexpr Variant(U&& value)
        : Variant(in_place_type<meta::InvokeResult<Selector<U>, U>>, util::forward<U>(value)) {}

    template<size_t index, typename... Args, typename T = meta::At<List, index>>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr explicit Variant(Constexpr<index>, Args&&... args) {
        do_emplace(c_<index>, util::forward<Args>(args)...);
    }

    template<size_t index, typename U, typename... Args, typename T = meta::At<List, index>>
    requires(concepts::ConstructibleFrom<T, std::initializer_list<U>, Args...>)
    constexpr explicit Variant(Constexpr<index>, std::initializer_list<U> list, Args&&... args) {
        do_emplace(c_<index>, list, util::forward<Args>(args)...);
    }

    template<typename T, typename... Args, auto index = meta::Lookup<T, List>>
    requires(meta::UniqueType<T, List> && concepts::ConstructibleFrom<T, Args...>)
    constexpr explicit Variant(InPlaceType<T>, Args&&... args) {
        do_emplace(c_<index>, util::forward<Args>(args)...);
    }

    template<typename T, typename U, typename... Args, auto index = meta::Lookup<T, List>>
    requires(meta::UniqueType<T, List> && concepts::ConstructibleFrom<T, std::initializer_list<U>, Args...>)
    constexpr explicit Variant(InPlaceType<T>, std::initializer_list<U> list, Args&&... args) {
        do_emplace(c_<index>, list, util::forward<Args>(args)...);
    }

    template<typename... Other>
    requires(sizeof...(Types) == sizeof...(Other) &&
             requires { requires concepts::Conjunction<concepts::ConstructibleFrom<Types, Other const&>...>; })
    constexpr explicit(!concepts::Conjunction<concepts::ConvertibleTo<Other const&, Types>...>)
        Variant(Variant<Other...> const& other) {
        function::index_dispatch<void, sizeof...(Types)>(other.index(), [&]<size_t index>(Constexpr<index>) {
            do_emplace(c_<index>, util::get<index>(other));
        });
    }

    template<typename... Other>
    requires(sizeof...(Types) == sizeof...(Other) &&
             requires { requires concepts::Conjunction<concepts::ConstructibleFrom<Types, Other>...>; })
    constexpr explicit(!concepts::Conjunction<concepts::ConvertibleTo<Other, Types>...>)
        Variant(Variant<Other...>&& other) {
        function::index_dispatch<void, sizeof...(Types)>(other.index(), [&]<size_t index>(Constexpr<index>) {
            do_emplace(c_<index>, util::get<index>(util::move(other)));
        });
    }

    constexpr ~Variant() { destroy(); }

    constexpr Variant& operator=(Variant const& other)
    requires(!trivially_copy_assignable && copyable)
    {
        destroy();
        function::index_dispatch<void, sizeof...(Types)>(other.index(), [&]<size_t index>(Constexpr<index>) {
            do_emplace(c_<index>, util::get<index>(other));
        });
        return *this;
    }

    constexpr Variant& operator=(Variant&& other)
    requires(!trivially_move_assignable && movable)
    {
        destroy();
        function::index_dispatch<void, sizeof...(Types)>(other.index(), [&]<size_t index>(Constexpr<index>) {
            do_emplace(c_<index>, util::get<index>(util::move(other)));
        });
        return *this;
    }

    template<typename U>
    requires(!concepts::RemoveCVRefSameAs<U, Variant> && !concepts::InstanceOf<meta::RemoveCVRef<U>, InPlaceType> &&
             !concepts::Constexpr<meta::RemoveCVRef<U>> && concepts::Invocable<Selector<U>, U> &&
             concepts::ConstructibleFrom<meta::InvokeResult<Selector<U>, U>, U>)
    constexpr Variant& operator=(U&& value) {
        this->template emplace<meta::InvokeResult<Selector<U>, U>>(util::forward<U>(value));
        return *this;
    }

    constexpr size_t index() const { return m_index; }

    template<size_t index, typename... Args, typename T = meta::At<List, index>>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr T& emplace(Args&&... args) {
        destroy();
        return do_emplace(c_<index>, util::forward<Args>(args)...);
    }

    template<size_t index, typename U, typename... Args, typename T = meta::At<List, index>>
    requires(concepts::ConstructibleFrom<T, std::initializer_list<U>, Args...>)
    constexpr T& emplace(std::initializer_list<U> list, Args&&... args) {
        destroy();
        return do_emplace(c_<index>, list, util::forward<Args>(args)...);
    }

    template<typename T, typename... Args, auto index = meta::Lookup<T, List>>
    requires(meta::UniqueType<T, List> && concepts::ConstructibleFrom<T, Args...>)
    constexpr T& emplace(Args&&... args) {
        destroy();
        return do_emplace(c_<index>, util::forward<Args>(args)...);
    }

    template<typename T, typename U, typename... Args, auto index = meta::Lookup<T, List>>
    requires(meta::UniqueType<T, List> && concepts::ConstructibleFrom<T, std::initializer_list<U>, Args...>)
    constexpr T& emplace(std::initializer_list<U> list, Args&&... args) {
        destroy();
        return do_emplace(c_<index>, list, util::forward<Args>(args)...);
    }

private:
    template<typename... Other>
    requires(sizeof...(Types) == sizeof...(Other) &&
             requires { requires concepts::Conjunction<concepts::EqualityComparableWith<Types, Other>...>; })
    constexpr friend bool operator==(Variant const& a, Variant<Other...> const& b) {
        if (a.index() != b.index()) {
            return false;
        }
        return function::index_dispatch<bool, sizeof...(Types)>(a.index(), [&]<size_t index>(Constexpr<index>) {
            return util::get<index>(a) == util::get<index>(b);
        });
    }

    template<typename... Other>
    requires(sizeof...(Types) == sizeof...(Other) &&
             requires { requires concepts::Conjunction<concepts::ThreeWayComparableWith<Types, Other>...>; })
    constexpr friend auto operator<=>(Variant const& a, Variant<Other...> const& b) {
        using Result = meta::CommonComparisonCategory<meta::CompareThreeWayResult<Types, Other>...>;
        if (auto result = a.index() <=> b.index(); result != 0) {
            return Result(result);
        }
        return function::index_dispatch<Result, sizeof...(Types)>(
            a.index(), [&]<size_t index>(Constexpr<index>) -> Result {
                return util::get<index>(a) <=> util::get<index>(b);
            });
    }

    template<size_t index, concepts::DerivedFrom<Variant> Self = Variant>
    constexpr friend meta::At<List, index> tag_invoke(types::Tag<variant_alternative>, InPlaceType<Self>,
                                                      Constexpr<index>) {
        return {};
    }

    template<size_t index, concepts::DerivedFrom<Variant> Self = Variant>
    constexpr friend meta::At<List, index> const tag_invoke(types::Tag<variant_alternative>, InPlaceType<Self const>,
                                                            Constexpr<index>) {
        return {};
    }

    template<concepts::DerivedFrom<Variant> Self = Variant>
    constexpr friend size_t tag_invoke(types::Tag<variant_size>, InPlaceType<Self>) {
        return meta::Size<List>;
    }

    template<size_t index, typename Self = Variant>
    requires(concepts::DerivedFrom<meta::RemoveCVRef<Self>, Variant>)
    constexpr friend meta::Like<Self, meta::At<List, index>>&& tag_invoke(types::Tag<util::get_in_place>,
                                                                          Constexpr<index>, Self&& self) {
        DI_ASSERT(index == self.m_index);
        return Impl::static_get(c_<index>, util::forward<Self>(self).m_impl);
    }

    template<typename T, typename Self = Variant>
    requires(concepts::DerivedFrom<meta::RemoveCVRef<Self>, Variant> && meta::UniqueType<T, List>)
    constexpr friend meta::Like<Self, T>&& tag_invoke(types::Tag<util::get_in_place>, InPlaceType<T>, Self&& self) {
        constexpr auto index = meta::Lookup<T, List>;
        DI_ASSERT(index == self.m_index);
        return Impl::static_get(c_<index>, util::forward<Self>(self).m_impl);
    }

    template<concepts::DerivedFrom<Variant> Self = Variant>
    constexpr friend List tag_invoke(types::Tag<variant_types>, InPlaceType<Self>) {
        return List {};
    }

    constexpr void destroy()
    requires(trivially_destructible)
    {}

    constexpr void destroy() {
        function::index_dispatch<void, meta::Size<List>>(
            this->index(),
            []<size_t index>(Constexpr<index>, Impl& impl) {
                impl.destroy_impl(c_<index>);
            },
            m_impl);
    }

    template<size_t index, typename... Args>
    constexpr decltype(auto) do_emplace(Constexpr<index>, Args&&... args) {
        m_index = index;
        return m_impl.emplace_impl(c_<index>, util::forward<Args>(args)...);
    }

    template<size_t index, typename U, typename... Args>
    constexpr decltype(auto) do_emplace(Constexpr<index>, std::initializer_list<U> list, Args&&... args) {
        m_index = index;
        return m_impl.emplace_impl(c_<index>, list, util::forward<Args>(args)...);
    }

    Impl m_impl;
    math::SmallestUnsignedType<meta::Size<List>> m_index { 0 };
};
}
