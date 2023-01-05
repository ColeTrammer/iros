#pragma once

#include <di/concepts/disjunction.h>
#include <di/concepts/remove_cvref_same_as.h>
#include <di/meta/like.h>
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

    constexpr VariantImpl& operator=(VariantImpl const&) = default;
    constexpr VariantImpl& operator=(VariantImpl&&) = default;

    ~VariantImpl() = default;

    constexpr ~VariantImpl()
    requires(concepts::Disjunction<!concepts::TriviallyDestructible<T>, !concepts::TriviallyDestructible<Rest>...>)
    {}

    template<concepts::RemoveCVRefSameAs<VariantImpl> Self>
    constexpr static meta::Like<Self, T>&& static_get(InPlaceIndex<0>, Self&& self) {
        return util::forward<Self>(self).m_value.value();
    }

    template<concepts::RemoveCVRefSameAs<VariantImpl> Self, size_t index>
    requires(index != 0)
    constexpr static decltype(auto) static_get(InPlaceIndex<index>, Self&& self) {
        return VariantImpl<Rest...>::static_get(in_place_index<index - 1>, util::forward<Self>(self).m_rest);
    }

    constexpr void destroy_impl(InPlaceIndex<0>) { util::destroy_at(util::address_of(m_value)); }

    template<size_t index>
    constexpr void destroy_impl(InPlaceIndex<index>) {
        return m_rest.destroy_impl(in_place_index<index - 1>);
    }

    template<typename... Args>
    constexpr T& emplace_impl(InPlaceIndex<0>, Args&&... args) {
        util::construct_at(util::address_of(m_value), in_place, util::forward<Args>(args)...);
        return m_value.value();
    }

    template<size_t index, typename... Args>
    requires(index != 0)
    constexpr decltype(auto) emplace_impl(InPlaceIndex<index>, Args&&... args) {
        return m_rest.emplace_impl(in_place_index<index - 1>, util::forward<Args>(args)...);
    }

    template<typename U>
    constexpr T& rebind_impl(InPlaceIndex<0>, U&& value) {
        m_value = util::forward<U>(value);
        return m_value.value();
    }

    template<size_t index, typename U>
    requires(index != 0)
    constexpr decltype(auto) rebind_impl(InPlaceIndex<index>, U&& value) {
        return m_rest.rebind_impl(in_place_index<index - 1>, util::forward<U>(value));
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

    constexpr VariantImpl& operator=(VariantImpl const&) = default;
    constexpr VariantImpl& operator=(VariantImpl&&) = default;

    ~VariantImpl() = default;

    constexpr ~VariantImpl()
    requires(!concepts::TriviallyDestructible<T>)
    {}

    template<concepts::RemoveCVRefSameAs<VariantImpl> Self>
    constexpr static meta::Like<Self, T>&& static_get(InPlaceIndex<0>, Self&& self) {
        return util::forward<Self>(self).m_value.value();
    }

    constexpr void destroy_impl(InPlaceIndex<0>) { util::destroy_at(util::address_of(m_value)); }

    template<typename... Args>
    constexpr T& emplace_impl(InPlaceIndex<0>, Args&&... args) {
        util::construct_at(util::address_of(m_value), in_place, util::forward<Args>(args)...);
        return m_value.value();
    }

private:
    union {
        util::RebindableBox<T> m_value;
    };
};
}
