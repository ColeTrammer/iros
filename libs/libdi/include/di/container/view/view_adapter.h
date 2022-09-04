#pragma once

#include <di/container/view/view_closure.h>
#include <di/util/forward.h>

namespace di::container::view_adapter {
template<typename Self>
struct ViewAdapter {
    template<typename... Args>
    constexpr auto operator()(Args&&... args) const {
        return view_closure(static_cast<const Self&>(*this), util::forward<Args>(args)...);
    }
};
}
