#pragma once

#include <di/container/interface/enable_borrowed_container.h>

namespace di::meta {
template<typename Self, bool should_enable = true>
class EnableBorrowedContainer {
private:
    constexpr friend bool tag_invoke(types::Tag<container::enable_borrowed_container>, types::InPlaceType<Self>) {
        return should_enable;
    }
};
}
