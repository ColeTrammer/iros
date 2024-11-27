#pragma once

#include <di/container/interface/enable_borrowed_container.h>

namespace di::meta {
template<typename Self, bool should_enable = true>
class EnableBorrowedContainer {
private:
    constexpr friend auto tag_invoke(types::Tag<container::enable_borrowed_container>, types::InPlaceType<Self>)
        -> bool {
        return should_enable;
    }
};
}
