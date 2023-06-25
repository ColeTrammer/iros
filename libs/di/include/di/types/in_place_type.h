#pragma once

namespace di::types {
template<typename T>
struct InPlaceType {
    using Type = T;

    explicit InPlaceType() = default;
};

template<typename T>
constexpr inline auto in_place_type = InPlaceType<T> {};
}

namespace di {
using types::in_place_type;
using types::InPlaceType;
}
