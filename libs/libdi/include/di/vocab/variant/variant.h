#pragma once

#include <di/concepts/default_constructible.h>
#include <di/meta/list.h>
#include <di/vocab/variant/variant_impl.h>

namespace di::vocab {
template<typename... Types>
requires(sizeof...(Types) > 0)
class Variant {
private:
    using Impl = detail::VariantImpl<Types...>;

public:
    // template<size_t index, typename... Args>
    // constexpr explicit Variant(InPlaceIndex<index>, Args&&... args) {
    //     do_emplace(in_place_index<index>, util::forward<Args>(args)...);
    // }

    constexpr ~Variant() = default;

private:
    // template<size_t index, typename... Args>
    // decltype(auto) do_emplace(InPlaceIndex<index>, Args&&... args) {
    //     return this->template emplace_impl<index>(util::forward<Args>(args)...);
    // }

    Impl m_impl;
    size_t m_index { 0 };
};
}
