#pragma once

#include "di/function/pipeable.h"
#include "di/util/forward.h"

namespace di::function {
struct Identity : pipeline::EnablePipeline {
    template<typename T>
    constexpr auto operator()(T&& value) const -> T&& {
        return util::forward<T>(value);
    }

    constexpr void operator()() const {}
};

constexpr inline auto identity = Identity {};
}

namespace di {
using function::identity;
using function::Identity;
}
