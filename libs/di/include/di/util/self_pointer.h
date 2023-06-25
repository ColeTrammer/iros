#pragma once

namespace di::util {
template<typename T>
struct SelfPointer {
    constexpr explicit SelfPointer() : self(static_cast<T*>(this)) {}

    constexpr SelfPointer(SelfPointer const&) : SelfPointer() {}
    constexpr SelfPointer(SelfPointer&&) : SelfPointer() {}

    constexpr SelfPointer& operator=(SelfPointer const&) { return *this; }
    constexpr SelfPointer& operator=(SelfPointer&&) { return *this; }

    T* self { nullptr };
};
}

namespace di {
using util::SelfPointer;
}
