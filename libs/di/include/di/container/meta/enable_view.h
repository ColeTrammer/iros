#pragma once

#include <di/container/interface/enable_view.h>

namespace di::meta {
template<typename Self, bool should_enable = true>
class EnableView {
private:
    constexpr friend auto tag_invoke(types::Tag<container::enable_view>, types::InPlaceType<Self>) -> bool {
        return should_enable;
    }
};
}
