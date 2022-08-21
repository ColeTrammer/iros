#pragma once

namespace di::util::meta {
template<bool val>
struct BoolConstant {
    constexpr static bool value = val;
};
}
