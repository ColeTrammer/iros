#pragma once

#include <di/concepts/default_constructible.h>
#include <di/concepts/trivially_copy_assignable.h>
#include <di/concepts/trivially_copy_constructible.h>
#include <di/concepts/trivially_destructible.h>
#include <di/concepts/trivially_move_assignable.h>
#include <di/concepts/trivially_move_constructible.h>
#include <di/function/index_dispatch.h>
#include <di/math/smallest_unsigned_type.h>
#include <di/meta/add_member_get.h>
#include <di/meta/list.h>
#include <di/util/forward.h>
#include <di/util/get.h>
#include <di/util/initializer_list.h>
#include <di/util/move.h>
#include <di/vocab/variant/variant_alternative.h>
#include <di/vocab/variant/variant_impl.h>
#include <di/vocab/variant/variant_index.h>
#include <di/vocab/variant/variant_size.h>
#include <di/vocab/variant/variant_types.h>

namespace di::vocab {
template<typename... Types>
requires(sizeof...(Types) > 0)
class Variant : public meta::AddMemberGet<Variant<Types...>> {
private:
    using Impl = detail::VariantImpl<Types...>;
    using List = meta::List<Types...>;

    constexpr static bool trivially_copy_constructible = concepts::Conjunction<concepts::TriviallyCopyConstructible<Types>...>;
    constexpr static bool trivially_move_constructible = concepts::Conjunction<concepts::TriviallyMoveConstructible<Types>...>;
    constexpr static bool trivially_copy_assignable = concepts::Conjunction<concepts::TriviallyCopyAssignable<Types>...>;
    constexpr static bool trivially_move_assignable = concepts::Conjunction<concepts::TriviallyMoveAssignable<Types>...>;
    constexpr static bool trivially_destructible = concepts::Conjunction<concepts::TriviallyDestructible<Types>...>;

    constexpr static bool copyable = concepts::Conjunction<concepts::CopyConstructible<Types>...>;
    constexpr static bool movable = concepts::Conjunction<concepts::CopyConstructible<Types>...>;

public:
    // conditionally trivial special member functions.
    constexpr Variant(Variant const&)
    requires(trivially_copy_constructible)
    = default;
    constexpr Variant(Variant&&)
    requires(trivially_move_constructible)
    = default;
    constexpr ~Variant()
    requires(trivially_destructible)
    = default;
    constexpr Variant& operator=(Variant const&)
    requires(trivially_copy_assignable)
    = default;
    constexpr Variant& operator=(Variant&&)
    requires(trivially_move_assignable)
    = default;

    constexpr Variant()
    requires(concepts::DefaultConstructible<meta::Front<List>>)
    {
        do_emplace(in_place_index<0>);
    }

    constexpr Variant(Variant const& other)
    requires(copyable && !trivially_copy_constructible)
    {
        function::index_dispatch<void, meta::Size<List>>(
            this->index(),
            []<size_t index>(InPlaceIndex<index>, Variant& self, Variant const& other) {
                self.do_emplace(in_place_index<index>, util::get<index>(other));
            },
            *this, other);
    }

    constexpr Variant(Variant&& other)
    requires(movable && !trivially_move_constructible)
    {
        function::index_dispatch<void, meta::Size<List>>(
            this->index(),
            []<size_t index>(InPlaceIndex<index>, Variant& self, Variant&& other) {
                self.do_emplace(in_place_index<index>, util::get<index>(util::move(other)));
            },
            *this, util::move(other()));
    }

    template<size_t index, typename... Args, typename T = meta::At<List, index>>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr explicit Variant(InPlaceIndex<index>, Args&&... args) {
        do_emplace(in_place_index<index>, util::forward<Args>(args)...);
    }

    template<size_t index, typename U, typename... Args, typename T = meta::At<List, index>>
    requires(concepts::ConstructibleFrom<T, util::InitializerList<U>, Args...>)
    constexpr explicit Variant(InPlaceIndex<index>, util::InitializerList<U> list, Args&&... args) {
        do_emplace(in_place_index<index>, list, util::forward<Args>(args)...);
    }

    template<typename T, typename... Args, auto index = meta::Lookup<T, List>>
    requires(meta::UniqueType<T, List> && concepts::ConstructibleFrom<T, Args...>)
    constexpr explicit Variant(InPlaceType<T>, Args&&... args) {
        do_emplace(in_place_index<index>, util::forward<Args>(args)...);
    }

    template<typename T, typename U, typename... Args, auto index = meta::Lookup<T, List>>
    requires(meta::UniqueType<T, List> && concepts::ConstructibleFrom<T, util::InitializerList<U>, Args...>)
    constexpr explicit Variant(InPlaceType<T>, util::InitializerList<U> list, Args&&... args) {
        do_emplace(in_place_index<index>, list, util::forward<Args>(args)...);
    }

    constexpr ~Variant() { destroy(); }

    constexpr size_t index() const { return m_index; }

    template<size_t index, typename... Args, typename T = meta::At<List, index>>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr T& emplace(Args&&... args) {
        destroy();
        return do_emplace(in_place_index<index>, util::forward<Args>(args)...);
    }

    template<size_t index, typename U, typename... Args, typename T = meta::At<List, index>>
    requires(concepts::ConstructibleFrom<T, util::InitializerList<U>, Args...>)
    constexpr T& emplace(util::InitializerList<U> list, Args&&... args) {
        destroy();
        return do_emplace(in_place_index<index>, list, util::forward<Args>(args)...);
    }

    template<typename T, typename... Args, auto index = meta::Lookup<T, List>>
    requires(meta::UniqueType<T, List> && concepts::ConstructibleFrom<T, Args...>)
    constexpr T& emplace(Args&&... args) {
        destroy();
        return do_emplace(in_place_index<index>, util::forward<Args>(args)...);
    }

    template<typename T, typename U, typename... Args, auto index = meta::Lookup<T, List>>
    requires(meta::UniqueType<T, List> && concepts::ConstructibleFrom<T, util::InitializerList<U>, Args...>)
    constexpr T& emplace(util::InitializerList<U> list, Args&&... args) {
        destroy();
        return do_emplace(in_place_index<index>, list, util::forward<Args>(args)...);
    }

private:
    template<size_t index>
    friend meta::At<List, index> tag_invoke(types::Tag<variant_alternative>, InPlaceType<Variant>, InPlaceIndex<index>);

    template<size_t index>
    friend meta::At<List, index> const tag_invoke(types::Tag<variant_alternative>, InPlaceType<Variant const>, InPlaceIndex<index>);

    constexpr friend size_t tag_invoke(types::Tag<variant_size>, InPlaceType<Variant>) { return meta::Size<List>; }

    template<concepts::RemoveCVRefSameAs<Variant> Self, size_t index>
    constexpr friend meta::Like<Self, meta::At<List, index>>&& tag_invoke(types::Tag<util::get_in_place>, InPlaceIndex<index>,
                                                                          Self&& self) {
        DI_ASSERT(index == self.m_index);
        return Impl::static_get(in_place_index<index>, util::forward<Self>(self).m_impl);
    }

    template<concepts::RemoveCVRefSameAs<Variant> Self, typename T>
    requires(meta::UniqueType<T, List>)
    constexpr friend meta::Like<Self, T>&& tag_invoke(types::Tag<util::get_in_place>, InPlaceType<T>, Self&& self) {
        constexpr auto index = meta::Lookup<T, List>;
        DI_ASSERT(index == self.m_index);
        return Impl::static_get(in_place_index<index>, util::forward<Self>(self).m_impl);
    }

    constexpr friend List tag_invoke(types::Tag<variant_types>, InPlaceType<Variant>) { return List {}; }

    constexpr void destroy()
    requires(trivially_destructible)
    {}

    constexpr void destroy() {
        function::index_dispatch<void, meta::Size<List>>(
            this->index(),
            []<size_t index>(InPlaceIndex<index>, Impl& impl) {
                impl.destroy(in_place_index<index>);
            },
            m_impl);
    }

    template<size_t index, typename... Args>
    constexpr decltype(auto) do_emplace(InPlaceIndex<index>, Args&&... args) {
        m_index = index;
        return m_impl.emplace_impl(in_place_index<index>, util::forward<Args>(args)...);
    }

    template<size_t index, typename U, typename... Args>
    constexpr decltype(auto) do_emplace(InPlaceIndex<index>, util::InitializerList<U> list, Args&&... args) {
        m_index = index;
        return m_impl.emplace_impl(in_place_index<index>, list, util::forward<Args>(args)...);
    }

    Impl m_impl;
    math::SmallestUnsignedType<meta::Size<List>> m_index { 0 };
};
}
