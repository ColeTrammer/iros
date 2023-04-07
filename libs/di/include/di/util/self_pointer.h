#pragma once

namespace di::util {
template<typename T>
struct SelfPointer {
    constexpr explicit SelfPointer() : self(static_cast<T*>(this)) {}

    T* self { nullptr };
};
}
