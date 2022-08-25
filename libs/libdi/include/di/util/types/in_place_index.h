#pragma once

#include <di/util/types/size_t.h>

namespace di::util::types {
template<size_t index>
struct InPlaceIndex {
    explicit InPlaceIndex() = default;
};

template<size_t index>
constexpr inline auto in_place_index = InPlaceIndex<index> {};
}
