#pragma once

#include <di/function/index_dispatch.h>
#include <di/math/smallest_unsigned_type.h>
#include <di/meta/algorithm.h>
#include <di/meta/constexpr.h>
#include <di/meta/core.h>
#include <di/meta/operations.h>
#include <di/meta/trivial.h>
#include <di/util/add_member_get.h>
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
class Variant : public util::AddMemberGet<Variant<Types...>> {
private:
    using Impl = detail::VariantImpl<Types...>;
    using List = meta::List<Types...>;

    constexpr static bool trivially_copy_constructible = (concepts::TriviallyCopyConstructible<Types> && ...);
    constexpr static bool trivially_move_constructible = (concepts::TriviallyMoveConstructible<Types> && ...);
    constexpr static bool trivially_copy_assignable = (concepts::TriviallyCopyAssignable<Types> && ...);
    constexpr static bool trivially_move_assignable = (concepts::TriviallyMoveAssignable<Types> && ...);
    constexpr static bool trivially_destructible = (concepts::TriviallyDestructible<Types> && ...);

    constexpr static bool copyable = (concepts::CopyConstructible<Types> && ...);
    constexpr static bool movable = (concepts::MoveConstructible<Types> && ...);

    template<typename U, typename T>
    struct SelectorImpl {
        auto operator()(T) const -> T
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
    auto operator=(Variant const&) -> Variant& requires(trivially_copy_assignable) = default;
    auto operator=(Variant&&) -> Variant& requires(trivially_move_assignable) = default;

    constexpr Variant()
    requires(concepts::DefaultConstructible<meta::Front<List>>)
    {
        do_emplace(c_<0ZU>);
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
             requires { requires(concepts::ConstructibleFrom<Types, Other const&> && ...); })
    constexpr explicit((!concepts::ConvertibleTo<Other const&, Types> || ...)) Variant(Variant<Other...> const& other) {
        function::index_dispatch<void, sizeof...(Types)>(other.index(), [&]<size_t index>(Constexpr<index>) {
            do_emplace(c_<index>, util::get<index>(other));
        });
    }

    template<typename... Other>
    requires(sizeof...(Types) == sizeof...(Other) &&
             requires { requires(concepts::ConstructibleFrom<Types, Other> && ...); })
    constexpr explicit((!concepts::ConvertibleTo<Other, Types> || ...)) Variant(Variant<Other...>&& other) {
        function::index_dispatch<void, sizeof...(Types)>(other.index(), [&]<size_t index>(Constexpr<index>) {
            do_emplace(c_<index>, util::get<index>(util::move(other)));
        });
    }

    constexpr ~Variant() { destroy(); }

    constexpr auto operator=(Variant const& other) -> Variant& requires(!trivially_copy_assignable && copyable) {
        destroy();
        function::index_dispatch<void, sizeof...(Types)>(other.index(), [&]<size_t index>(Constexpr<index>) {
            do_emplace(c_<index>, util::get<index>(other));
        });
        return *this;
    }

    constexpr auto operator=(Variant&& other) -> Variant& requires(!trivially_move_assignable && movable) {
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
    constexpr auto operator=(U&& value) -> Variant& {
        this->template emplace<meta::InvokeResult<Selector<U>, U>>(util::forward<U>(value));
        return *this;
    }

    constexpr auto index() const -> size_t { return m_index; }

    template<size_t index, typename... Args, typename T = meta::At<List, index>>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr auto emplace(Args&&... args) -> T& {
        destroy();
        return do_emplace(c_<index>, util::forward<Args>(args)...);
    }

    template<size_t index, typename U, typename... Args, typename T = meta::At<List, index>>
    requires(concepts::ConstructibleFrom<T, std::initializer_list<U>, Args...>)
    constexpr auto emplace(std::initializer_list<U> list, Args&&... args) -> T& {
        destroy();
        return do_emplace(c_<index>, list, util::forward<Args>(args)...);
    }

    template<typename T, typename... Args, auto index = meta::Lookup<T, List>>
    requires(meta::UniqueType<T, List> && concepts::ConstructibleFrom<T, Args...>)
    constexpr auto emplace(Args&&... args) -> T& {
        destroy();
        return do_emplace(c_<index>, util::forward<Args>(args)...);
    }

    template<typename T, typename U, typename... Args, auto index = meta::Lookup<T, List>>
    requires(meta::UniqueType<T, List> && concepts::ConstructibleFrom<T, std::initializer_list<U>, Args...>)
    constexpr auto emplace(std::initializer_list<U> list, Args&&... args) -> T& {
        destroy();
        return do_emplace(c_<index>, list, util::forward<Args>(args)...);
    }

private:
    template<typename... Other>
    requires(sizeof...(Types) == sizeof...(Other) &&
             requires { requires(concepts::EqualityComparableWith<Types, Other> && ...); })
    constexpr friend auto operator==(Variant const& a, Variant<Other...> const& b) -> bool {
        if (a.index() != b.index()) {
            return false;
        }
        return function::index_dispatch<bool, sizeof...(Types)>(a.index(), [&]<size_t index>(Constexpr<index>) {
            return util::get<index>(a) == util::get<index>(b);
        });
    }

    template<typename... Other>
    requires(sizeof...(Types) == sizeof...(Other) &&
             requires { requires(concepts::ThreeWayComparableWith<Types, Other> && ...); })
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
    constexpr friend auto tag_invoke(types::Tag<variant_alternative>, InPlaceType<Self>, Constexpr<index>)
        -> meta::At<List, index> {
        return {};
    }

    template<size_t index, concepts::DerivedFrom<Variant> Self = Variant>
    // NOLINTNEXTLINE(readability-const-return-type)
    constexpr friend auto tag_invoke(types::Tag<variant_alternative>, InPlaceType<Self const>, Constexpr<index>)
        -> meta::At<List, index> const {
        return {};
    }

    template<concepts::DerivedFrom<Variant> Self = Variant>
    constexpr friend auto tag_invoke(types::Tag<variant_size>, InPlaceType<Self>) -> size_t {
        return meta::Size<List>;
    }

    template<size_t index, typename Self = Variant>
    requires(concepts::DerivedFrom<meta::RemoveCVRef<Self>, Variant>)
    constexpr friend auto tag_invoke(types::Tag<util::get_in_place>, Constexpr<index>, Self&& self)
        -> meta::Like<Self, meta::At<List, index>>&& {
        DI_ASSERT(index == self.m_index);
        return Impl::static_get(c_<index>, util::forward<Self>(self).m_impl);
    }

    template<typename T, typename Self = Variant>
    requires(concepts::DerivedFrom<meta::RemoveCVRef<Self>, Variant> && meta::UniqueType<T, List>)
    constexpr friend auto tag_invoke(types::Tag<util::get_in_place>, InPlaceType<T>, Self&& self)
        -> meta::Like<Self, T>&& {
        constexpr auto index = meta::Lookup<T, List>;
        DI_ASSERT(index == self.m_index);
        return Impl::static_get(c_<index>, util::forward<Self>(self).m_impl);
    }

    template<concepts::DerivedFrom<Variant> Self = Variant>
    constexpr friend auto tag_invoke(types::Tag<variant_types>, InPlaceType<Self>) -> List {
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
    constexpr auto do_emplace(Constexpr<index>, Args&&... args) -> decltype(auto) {
        m_index = index;
        return m_impl.emplace_impl(c_<index>, util::forward<Args>(args)...);
    }

    template<size_t index, typename U, typename... Args>
    constexpr auto do_emplace(Constexpr<index>, std::initializer_list<U> list, Args&&... args) -> decltype(auto) {
        m_index = index;
        return m_impl.emplace_impl(c_<index>, list, util::forward<Args>(args)...);
    }

    Impl m_impl;
    math::SmallestUnsignedType<meta::Size<List>> m_index { 0 };
};
}
