#pragma once

#include <di/function/pipeable.h>
#include <di/function/pipeline.h>
#include <di/meta/operations.h>
#include <di/util/forward.h>

namespace di::function {
struct AsBool : function::pipeline::EnablePipeline {
    template<concepts::ConvertibleTo<bool> T>
    constexpr auto operator()(T&& value) const -> bool {
        return static_cast<bool>(util::forward<T>(value));
    }
};

constexpr inline auto as_bool = AsBool {};
}

namespace di {
using function::as_bool;
using function::AsBool;
}
