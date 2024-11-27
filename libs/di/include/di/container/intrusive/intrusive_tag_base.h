#pragma once

#include <di/meta/operations.h>
#include <di/types/prelude.h>

namespace di::container {
template<typename NodeType>
struct IntrusiveTagBase {
    template<typename T>
    constexpr static auto is_sized(InPlaceType<T>) -> bool {
        return false;
    }

    template<typename T>
    constexpr static auto always_store_tail(InPlaceType<T>) -> bool {
        return true;
    }

    template<typename T>
    constexpr static auto node_type(InPlaceType<T>) -> NodeType;

    template<typename T>
    constexpr static auto down_cast(InPlaceType<T>, NodeType& node) -> T& {
        return static_cast<T&>(node);
    }

    constexpr static void did_insert(auto&, auto&) {}
    constexpr static void did_remove(auto&, auto&) {}
};
}
