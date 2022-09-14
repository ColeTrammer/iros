#pragma once

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
    constexpr VariantImpl() = default;
    constexpr VariantImpl(VariantImpl const&) = default;
    constexpr VariantImpl(VariantImpl&&) = default;

    constexpr VariantImpl& operator=(VariantImpl const&) = default;
    constexpr VariantImpl& operator=(VariantImpl&&) = default;

    template<typename... Args>
    constexpr T& emplace_impl(InPlaceIndex<0>, Args&&... args) {
        return util::construct_at(in_place, util::forward<Args>(args)...);
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
    constexpr VariantImpl() = default;
    constexpr VariantImpl(VariantImpl const&) = default;
    constexpr VariantImpl(VariantImpl&&) = default;

    constexpr VariantImpl& operator=(VariantImpl const&) = default;
    constexpr VariantImpl& operator=(VariantImpl&&) = default;

private:
    union {
        util::RebindableBox<T> m_value;
    };
};
}
