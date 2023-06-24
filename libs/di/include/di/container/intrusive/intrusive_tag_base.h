#pragma once

#include <di/meta/operations.h>
#include <di/types/prelude.h>

namespace di::container {
template<typename NodeType>
struct IntrusiveTagBase {
    template<typename T>
    constexpr static bool is_sized(InPlaceType<T>) {
        return false;
    }

    template<typename T>
    constexpr static bool always_store_tail(InPlaceType<T>) {
        return true;
    }

    template<typename T>
    constexpr static NodeType node_type(InPlaceType<T>);

    template<typename T>
    constexpr static T& down_cast(InPlaceType<T>, NodeType& node) {
        return static_cast<T&>(node);
    }

    constexpr static void did_insert(auto&, auto&) {}
    constexpr static void did_remove(auto&, auto&) {}
};
}
