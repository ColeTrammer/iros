#pragma once

#include <di/function/pipeline.h>
#include <di/types/prelude.h>

namespace di::util {
namespace detail {
    struct Function : function::pipeline::EnablePipeline {
        auto operator()(auto* pointer) const -> uintptr_t { return reinterpret_cast<uintptr_t>(pointer); }
    };
}

constexpr inline auto to_uintptr = detail::Function {};
}

namespace di {
using util::to_uintptr;
}
