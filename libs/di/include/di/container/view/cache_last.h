#pragma once

#include "di/container/meta/container_reference.h"
#include "di/container/meta/container_rvalue.h"
#include "di/container/view/all.h"
#include "di/container/view/cache_last_view.h"
#include "di/function/pipeable.h"
#include "di/util/forward.h"

namespace di::container::view {
namespace detail {
    struct CacheLastFunction : public function::pipeline::EnablePipeline {
        template<concepts::InputContainer Con>
        constexpr auto operator()(Con&& container) const -> concepts::View auto {
            return CacheLastView { di::forward<Con>(container) };
        }
    };
}

constexpr inline auto cache_last = detail::CacheLastFunction {};
}

namespace di {
using container::view::cache_last;
}
