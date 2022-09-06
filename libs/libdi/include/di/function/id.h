#pragma once

#include <di/function/pipeable.h>
#include <di/util/forward.h>

namespace di::function {
namespace detail {
    struct IdFunction : pipeline::EnablePipeline {
        template<typename T>
        constexpr T&& operator()(T&& value) const {
            return util::forward<T>(value);
        }
    };
}

constexpr inline auto id = detail::IdFunction {};
}
