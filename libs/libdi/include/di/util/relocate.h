#pragma once

#include <di/concepts/destructible.h>
#include <di/concepts/move_constructible.h>
#include <di/util/address_of.h>
#include <di/util/destroy_at.h>
#include <di/util/move.h>

namespace di::util {
namespace detail {
    struct RelocateFunction {
        template<concepts::Destructible T>
        requires(concepts::MoveConstructible<T>)
        constexpr T operator()(T& reference) const {
            auto result = util::move(reference);
            destroy_at(util::address_of(reference));
            return result;
        }
    };
}

constexpr inline auto relocate = detail::RelocateFunction {};
}
