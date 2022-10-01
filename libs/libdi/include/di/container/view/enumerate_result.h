#pragma once

#include <di/vocab/tuple/prelude.h>

namespace di::container {
template<typename Idx, typename Val>
struct EnumerateResult {
    Idx index;
    Val value;

    constexpr friend bool tag_invoke(types::Tag<vocab::enable_generate_structed_bindings>, InPlaceType<EnumerateResult>) { return true; }

    constexpr friend Idx tag_invoke(types::Tag<vocab::tuple_element>, types::InPlaceType<EnumerateResult>, types::InPlaceIndex<0>) {}
    constexpr friend Val tag_invoke(types::Tag<vocab::tuple_element>, types::InPlaceType<EnumerateResult>, types::InPlaceIndex<1>) {}

    constexpr friend Idx const tag_invoke(types::Tag<vocab::tuple_element>, types::InPlaceType<EnumerateResult const>,
                                          types::InPlaceIndex<0>) {}
    constexpr friend Val const tag_invoke(types::Tag<vocab::tuple_element>, types::InPlaceType<EnumerateResult const>,
                                          types::InPlaceIndex<1>) {}

    constexpr friend types::size_t tag_invoke(types::Tag<vocab::tuple_size>, types::InPlaceType<EnumerateResult>) { return 2; }

    template<concepts::DecaySameAs<EnumerateResult> Self>
    constexpr friend meta::Like<Self, Idx> tag_invoke(types::Tag<util::get_in_place>, types::InPlaceIndex<0>, Self&& self) {
        return util::forward_like<Self>(self.index);
    }

    template<concepts::DecaySameAs<EnumerateResult> Self>
    constexpr friend meta::Like<Self, Val> tag_invoke(types::Tag<util::get_in_place>, types::InPlaceIndex<1>, Self&& self) {
        return util::forward_like<Self>(self.value);
    }

    template<concepts::DecaySameAs<EnumerateResult> Self, typename T>
    requires(!concepts::SameAs<Idx, Val> && (concepts::SameAs<T, Idx> || concepts::SameAs<T, Val>) )
    constexpr friend decltype(auto) tag_invoke(types::Tag<util::get_in_place>, types::InPlaceType<T>, Self&& self) {
        if constexpr (concepts::SameAs<T, Idx>) {
            return util::get<0>(util::forward<Self>(self));
        } else {
            return util::get<1>(util::forward<Self>(self));
        }
    }
};
}