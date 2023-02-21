#pragma once

#include <di/function/pipeline.h>
#include <di/types/prelude.h>

namespace di::util {
namespace detail {
    struct Function : function::pipeline::EnablePipeline {
        uintptr_t operator()(auto* pointer) const { return reinterpret_cast<uintptr_t>(pointer); }
    };
}

constexpr inline auto to_uintptr = detail::Function {};
}
