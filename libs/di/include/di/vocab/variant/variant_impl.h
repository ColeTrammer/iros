#pragma once

#include <di/meta/constexpr.h>
#include <di/meta/util.h>
#include <di/types/prelude.h>
#include <di/util/forward.h>
#include <di/util/rebindable_box.h>

namespace di::vocab::detail {
template<typename... Types>
requires(sizeof...(Types) > 0)
class VariantImpl;

template<typename T, typename... Rest>
class VariantImpl<T, Rest...> {
public:
    constexpr VariantImpl() {}
    constexpr VariantImpl(VariantImpl const&) = default;
    constexpr VariantImpl(VariantImpl&&) = default;

    constexpr auto operator=(VariantImpl const&) -> VariantImpl& = default;
    constexpr auto operator=(VariantImpl&&) -> VariantImpl& = default;

    ~VariantImpl() = default;

    constexpr ~VariantImpl()
    requires(!concepts::TriviallyDestructible<T> || (!concepts::TriviallyDestructible<Rest> || ...))
    {}

    template<concepts::RemoveCVRefSameAs<VariantImpl> Self>
    constexpr static auto static_get(Constexpr<0ZU>, Self&& self) -> meta::Like<Self, T>&& {
        return util::forward<Self>(self).m_value.value();
    }

    template<concepts::RemoveCVRefSameAs<VariantImpl> Self, size_t index>
    requires(index != 0)
    constexpr static auto static_get(Constexpr<index>, Self&& self) -> decltype(auto) {
        return VariantImpl<Rest...>::static_get(c_<index - 1>, util::forward<Self>(self).m_rest);
    }

    constexpr void destroy_impl(Constexpr<0ZU>) { util::destroy_at(util::addressof(m_value)); }

    template<size_t index>
    constexpr void destroy_impl(Constexpr<index>) {
        return m_rest.destroy_impl(c_<index - 1>);
    }

    template<typename... Args>
    constexpr auto emplace_impl(Constexpr<0ZU>, Args&&... args) -> T& {
        util::construct_at(util::addressof(m_value), in_place, util::forward<Args>(args)...);
        return m_value.value();
    }

    template<size_t index, typename... Args>
    requires(index != 0)
    constexpr auto emplace_impl(Constexpr<index>, Args&&... args) -> decltype(auto) {
        util::construct_at(util::addressof(m_rest));
        return m_rest.emplace_impl(c_<index - 1>, util::forward<Args>(args)...);
    }

private:
    union {
        util::RebindableBox<T> m_value;
        VariantImpl<Rest...> m_rest;
    };
};

template<typename T>
class VariantImpl<T> {
public:
    constexpr VariantImpl() {}
    constexpr VariantImpl(VariantImpl const&) = default;
    constexpr VariantImpl(VariantImpl&&) = default;

    constexpr auto operator=(VariantImpl const&) -> VariantImpl& = default;
    constexpr auto operator=(VariantImpl&&) -> VariantImpl& = default;

    ~VariantImpl() = default;

    constexpr ~VariantImpl()
    requires(!concepts::TriviallyDestructible<T>)
    {}

    template<concepts::RemoveCVRefSameAs<VariantImpl> Self>
    constexpr static auto static_get(Constexpr<0ZU>, Self&& self) -> meta::Like<Self, T>&& {
        return util::forward<Self>(self).m_value.value();
    }

    constexpr void destroy_impl(Constexpr<0ZU>) { util::destroy_at(util::addressof(m_value)); }

    template<typename... Args>
    constexpr auto emplace_impl(Constexpr<0ZU>, Args&&... args) -> T& {
        util::construct_at(util::addressof(m_value), in_place, util::forward<Args>(args)...);
        return m_value.value();
    }

private:
    union {
        util::RebindableBox<T> m_value;
    };
};
}
