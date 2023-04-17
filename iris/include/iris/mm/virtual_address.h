#pragma once

#include <di/util/prelude.h>

namespace iris::mm {
struct VirtualAddressTag {
    using Type = uptr;

    struct Mixin {
        using Self = di::StrongInt<VirtualAddressTag>;

        template<di::concepts::ImplicitLifetime T>
        T* typed_pointer() const {
            return reinterpret_cast<T*>(static_cast<Self const&>(*this).raw_value());
        }

        void* void_pointer() const { return reinterpret_cast<void*>(static_cast<Self const&>(*this).raw_value()); }
    };

    constexpr static bool format_as_pointer = true;
};

using VirtualAddress = di::StrongInt<VirtualAddressTag>;
}
