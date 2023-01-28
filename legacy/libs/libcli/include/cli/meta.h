#pragma once

#include <cli/forward.h>

namespace Cli {
template<typename T>
struct IsOption {
    using Type = T;
    static constexpr bool value = false;
};

template<typename T>
struct IsOption<Option<T>> {
    using Type = T;
    static constexpr bool value = true;
};

template<typename T>
struct IsVector {
    using Type = T;
    static constexpr bool value = false;
};

template<typename T>
struct IsVector<Vector<T>> {
    using Type = T;
    static constexpr bool value = true;
};
}
