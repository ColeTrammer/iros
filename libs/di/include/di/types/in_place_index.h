#pragma once

#include <di/types/size_t.h>

namespace di::types {
template<size_t index>
struct InPlaceIndex {
    explicit InPlaceIndex() = default;
};

template<size_t index>
constexpr inline auto in_place_index = InPlaceIndex<index> {};
}
