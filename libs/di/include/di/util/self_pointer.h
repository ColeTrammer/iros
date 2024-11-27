#pragma once

namespace di::util {
template<typename T>
struct SelfPointer {
    constexpr explicit SelfPointer() : self(static_cast<T*>(this)) {}

    constexpr SelfPointer(SelfPointer const&) : SelfPointer() {}
    constexpr SelfPointer(SelfPointer&&) : SelfPointer() {}

    constexpr auto operator=(SelfPointer const&) -> SelfPointer& { return *this; }
    constexpr auto operator=(SelfPointer&&) -> SelfPointer& { return *this; }

    T* self { nullptr };
};
}

namespace di {
using util::SelfPointer;
}
