#pragma once

#include <di/meta/operations.h>
#include <di/util/addressof.h>
#include <di/util/destroy_at.h>
#include <di/util/move.h>

namespace di::util {
namespace detail {
    struct RelocateFunction {
        template<concepts::Destructible T>
        requires(concepts::MoveConstructible<T>)
        constexpr T operator()(T& reference) const {
            auto result = util::move(reference);
            destroy_at(util::addressof(reference));
            return result;
        }
    };
}

constexpr inline auto relocate = detail::RelocateFunction {};
}
