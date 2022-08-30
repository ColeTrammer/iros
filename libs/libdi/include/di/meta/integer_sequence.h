#pragma once

namespace di::meta {
template<typename T, T... values>
struct IntegerSequence {
    using Value = T;

    constexpr static auto size() { return sizeof...(values); }
};
}
