#pragma once

#include <di/function/pipeable.h>
#include <di/function/pipeline.h>
#include <di/util/forward.h>

namespace di::function {
struct Dereference : function::pipeline::EnablePipeline {
    template<typename T>
    requires(requires(T&& a) { *util::forward<T>(a); })
    constexpr auto operator()(T&& a) const -> decltype(auto) {
        return *util::forward<T>(a);
    }
};

constexpr inline auto dereference = Dereference {};
}

namespace di {
using function::dereference;
using function::Dereference;
}
