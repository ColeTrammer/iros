#pragma once

namespace di::meta {
template<typename T>
struct TypeConstant {
    using Type = T;
};
}
