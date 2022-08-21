#pragma once

namespace di::util::meta {
template<typename T>
struct TypeConstant {
    using Type = T;
};
}
