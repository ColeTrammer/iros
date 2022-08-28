#pragma once

namespace di::types {
template<typename T>
struct InPlaceType {
    explicit InPlaceType() = default;
};

template<typename T>
constexpr inline auto in_place_type = InPlaceType<T> {};
}
