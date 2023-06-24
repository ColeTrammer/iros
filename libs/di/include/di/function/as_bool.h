#pragma once

#include <di/function/pipeable.h>
#include <di/function/pipeline.h>
#include <di/meta/operations.h>
#include <di/util/forward.h>

namespace di::function {
struct AsBool : function::pipeline::EnablePipeline {
    template<concepts::ConvertibleTo<bool> T>
    constexpr bool operator()(T&& value) const {
        return static_cast<bool>(util::forward<T>(value));
    }
};

constexpr inline auto as_bool = AsBool {};
}
