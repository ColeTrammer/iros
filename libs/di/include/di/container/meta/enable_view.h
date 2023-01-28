#pragma once

#include <di/container/interface/enable_view.h>

namespace di::meta {
template<typename Self, bool should_enable = true>
class EnableView {
private:
    constexpr friend bool tag_invoke(types::Tag<container::enable_view>, types::InPlaceType<Self>) {
        return should_enable;
    }
};
}
