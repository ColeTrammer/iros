#pragma once

#include "di/container/view/iota.h"
#include "di/meta/language.h"

namespace di::container::view {
namespace detail {
    struct RangeFunction : function::pipeline::EnablePipeline {
        template<concepts::Integer T>
        constexpr auto operator()(T end) const {
            return view::iota(static_cast<T>(0), end);
        }

        template<concepts::Integer T, concepts::Integer U>
        requires(concepts::SignedInteger<T> == concepts::SignedInteger<U>)
        constexpr auto operator()(T start, U end) const {
            return view::iota(start, end);
        }
    };
}

constexpr inline auto range = detail::RangeFunction {};
}

namespace di {
using view::range;
}
